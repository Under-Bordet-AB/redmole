#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "app_gui_bindings.h"
#include "board_i2c.h"
#include "environment_measurements.h"
#include "task_scheduler.h"
#include "gui_module.h"
#include "nac.h"
#include "rm_nvs.h"
#include "http_client.h"
#include "blufi_main.h"
#include "sdcard.h"
#include "sdcard_log.h"
#include "uart_mole.h"

static const char* TAG = "MAIN";
static gui_ctx_t s_gui = {0};
static EventGroupHandle_t s_event_group = NULL;
static char s_ssid[32];
static char s_password[64];
static size_t s_ssid_len = 32;
static size_t s_password_len = 64;

/*
 * Temporary coexistence diagnostics for issue #95. Remove after the final
 * WiFi/BLE lifecycle and memory-budget implementation is complete.
 */
static void log_radio_heap(const char *stage) {
    ESP_LOGI("RADIO_HEAP",
             "%s: internal free=%u largest=%u minimum=%u; DMA free=%u largest=%u; PSRAM free=%u largest=%u",
             stage,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_DMA),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_DMA),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
}

/*
 * Temporary issue #95 diagnostic. FreeRTOS can enumerate every task when the
 * trace facility is enabled, so individual components do not need wrappers.
 * Allocate the snapshot in PSRAM to preserve scarce internal/DMA memory.
 */
static void log_all_task_stack_watermarks(const char *stage) {
    UBaseType_t capacity = uxTaskGetNumberOfTasks() + 4;
    TaskStatus_t *tasks = heap_caps_calloc(capacity, sizeof(*tasks), MALLOC_CAP_SPIRAM);

    if (!tasks) {
        ESP_LOGE("STACK_HWM", "%s: failed to allocate snapshot for %u tasks",
                 stage, (unsigned)capacity);
        return;
    }

    UBaseType_t count = uxTaskGetSystemState(tasks, capacity, NULL);
    if (count == 0) {
        ESP_LOGE("STACK_HWM", "%s: task snapshot capacity %u was insufficient",
                 stage, (unsigned)capacity);
        heap_caps_free(tasks);
        return;
    }

    ESP_LOGI("STACK_HWM", "%s: %u tasks; high-water values are minimum-ever free stack bytes",
             stage, (unsigned)count);

    for (UBaseType_t i = 0; i < count; i++) {
        ESP_LOGI("STACK_HWM", "task=%-16s state=%d priority=%u free_min=%u",
                 tasks[i].pcTaskName,
                 (int)tasks[i].eCurrentState,
                 (unsigned)tasks[i].uxCurrentPriority,
                 (unsigned)tasks[i].usStackHighWaterMark);
    }

    heap_caps_free(tasks);
}

static esp_err_t init_single_instance_modules(EventGroupHandle_t* event_group) {
    esp_err_t rv = rm_nvs_init("app");
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "rm_nvs_init failed: %s", esp_err_to_name(rv));
        return rv;
    }

    if (task_scheduler_init() != ESP_OK) {
        ESP_LOGE(TAG, "task_scheduler_init failed:", esp_err_to_name(rv));
        return rv;
    }

    /* Initialize network interface and event loop
     *
     * esp_netif_init: Initializes the network interface TCP/IP stack
     * esp_event_loop_create_default: Creates the default event loop for handling system events
     *      the user must register event handlers to receive events from this loop, such as WiFi events
     * Belongs in main rather than in NAC init, makes NAC init testable
     */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    /* NAC init relies on network interface and event loop being initialized first */
    log_radio_heap("before NAC init");
    rv = nac_init(event_group);
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "nac_init failed: %s", esp_err_to_name(rv));
        return rv;
    }
    log_radio_heap("after NAC init");
    ESP_LOGI(TAG, "NAC module successfully initialized.");

    if (http_client_init(HTTP_CLIENT_TLS_BUNDLE, NULL) != ESP_OK) {
        ESP_LOGE(TAG, "http_client_init failed: %s", esp_err_to_name(rv));
        return rv;
    }

    rv = board_i2c_init();
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "board_i2c_init failed: %s", esp_err_to_name(rv));
        return rv;
    }

    rv = environment_measurements_init();
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "environment_measurements_init failed: %s", esp_err_to_name(rv));
        return rv;
    }

    rv = sdcard_init();
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "sdcard_init failed: %s", esp_err_to_name(rv));
    }

    rv = uart_mole_init(event_group);
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "uart_mole_init failed: %s", esp_err_to_name(rv));
        return rv;
    }

    log_radio_heap("before blufi_main");
    blufi_main();
    log_radio_heap("after blufi_main");

    return ESP_OK;
}

static esp_err_t init_runtime_modules(void) {
    log_radio_heap("before runtime modules");

    // Load previously saved GUI settings
    gui_init_config_t gui_init_config = {0};
    (void)app_gui_bindings_load_saved_appearance(&gui_init_config);

    // Initialize the GUI
    gui_init(&s_gui, &gui_init_config);

    // Initialize the GUI bindings
    esp_err_t rv = app_gui_bindings_init(&s_gui, &s_event_group);
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "app_gui_bindings_init failed: %s", esp_err_to_name(rv));
        return rv;
    }

    rv = sdcard_log_init("logs");
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "sdcard_log_init failed: %s", esp_err_to_name(rv));
    }

    log_radio_heap("after runtime modules");
    return ESP_OK;
}

static esp_err_t start_runtime_modules() {
    esp_err_t rv = environment_measurements_start();
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "environment_measurements_start failed: %s", esp_err_to_name(rv));
        return rv;
    }

    if (!s_gui.is_ready) {
        ESP_LOGW(TAG, "GUI not ready after initialization");
    }

    return ESP_OK;
}

void app_main(void) {
    ESP_LOGI(TAG, "app_main entered");

    s_event_group = xEventGroupCreate();
    if (!s_event_group) {
        ESP_LOGE(TAG, "xEventGroupCreate failed");
        goto fatal_error;
    }

    ESP_LOGI(TAG, "Initializing single-instance modules");
    if (init_single_instance_modules(&s_event_group) != ESP_OK) {
        goto fatal_error;
    }

    ESP_LOGI(TAG, "Initializing runtime modules");
    if (init_runtime_modules() != ESP_OK) {
        goto fatal_error;
    }

    if (start_runtime_modules() != ESP_OK) {
        goto fatal_error;
    }

    ESP_LOGI(TAG, "Startup complete");
    log_radio_heap("startup complete before saved WiFi connect");

    /*
    For testing destroy NVS records
    if (rm_nvs_erase_key("wifi_ssid") != ESP_OK) {
        ESP_LOGW(TAG, "Failed to erase wifi_ssid NVS key");
    }
    if (rm_nvs_erase_key("wifi_pass") != ESP_OK) {
        ESP_LOGW(TAG, "Failed to erase wifi_pass NVS key");
    }
    */

    /* Queue a connection attempt using saved WiFi credentials. */
    rm_nvs_get_str("wifi_ssid", s_ssid, &s_ssid_len);
    rm_nvs_get_str("wifi_pass", s_password, &s_password_len);
    if (s_ssid[0] != '\0' && s_password[0] != '\0') {
        nac_connect_to_saved_wifi(s_ssid, s_password);
        log_radio_heap("after saved WiFi connect request");
    }

    log_all_task_stack_watermarks("startup");
    TickType_t last_stack_report = xTaskGetTickCount();

    while (1) {
        // Synchronize the GUI with the backend
        app_gui_bindings_sync(&s_gui);

        // Scheduler hard labor
        task_scheduler_work();

        TickType_t now = xTaskGetTickCount();
        if ((now - last_stack_report) >= pdMS_TO_TICKS(60000)) {
            log_all_task_stack_watermarks("periodic");
            last_stack_report = now;
        }

        vTaskDelay(pdMS_TO_TICKS(30));
    }
    return;

fatal_error:
    ESP_LOGE(TAG, "System halted during startup");
}
