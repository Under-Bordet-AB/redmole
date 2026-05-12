#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "app_gui_bindings.h"
#include "task_scheduler.h"
#include "gui_module.h"
#include "local_sensor_service.h"
#include "nac.h"
#include "rm_nvs.h"
#include "sensor_data.h"
#include "http_client.h"
#include "sdcard.h"
#include "sdcard_log.h"

static const char* TAG = "MAIN";
static gui_ctx_t s_gui = {0};

static esp_err_t init_single_instance_modules(void) {
    esp_err_t rv = rm_nvs_init("app");
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "rm_nvs_init failed: %s", esp_err_to_name(rv));
        return rv;
    }

    if (task_scheduler_init() != ESP_OK) {
        ESP_LOGE(TAG, "task_scheduler_init failed:", esp_err_to_name(rv));
        return rv;
    }

    if (nac_init() != ESP_OK) {
        ESP_LOGE(TAG, "nac_init failed: %s", esp_err_to_name(rv));
        return rv;
    }

    if (http_client_init(HTTP_CLIENT_TLS_NONE, NULL) != ESP_OK) {
        ESP_LOGE(TAG, "http_client_init failed: %s", esp_err_to_name(rv));
        return rv;
    }

    rv = sensor_data_init();
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "sensor_data_init failed: %s", esp_err_to_name(rv));
        return rv;
    }
    
    rv = sdcard_init();
    if (rv != ESP_OK){
        ESP_LOGE(TAG, "sdcard_init failed: %s", esp_err_to_name(rv));
        return rv;
    }
    
    return ESP_OK;
}

static esp_err_t init_runtime_modules(void) {
    if (task_scheduler_init() != ESP_OK) {
        ESP_LOGE(TAG, "task_scheduler_init failed");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "task_scheduler_init started");

    esp_err_t rv = local_sensor_service_init();
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "local_sensor_service_init failed: %s", esp_err_to_name(rv));
        return rv;
    }

    // Load previously saved GUI settings
    gui_init_config_t gui_init_config = {0};
    (void)app_gui_bindings_load_saved_appearance(&gui_init_config);

    // Initialize the GUI
    gui_init(&s_gui, &gui_init_config);

    // Initialize the GUI bindings
    rv = app_gui_bindings_init(&s_gui);
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "app_gui_bindings_init failed: %s", esp_err_to_name(rv));
        return rv;
    }

    rv = sdcard_log_init("/sdcard/logfile.txt"); // We don't capture startup logs this way - init this in a pre-init?
    if (rv != ESP_OK){
        ESP_LOGE(TAG, "sdcard_log_init failed: %s", esp_err_to_name(rv));
    }

    return ESP_OK;
}

static esp_err_t start_runtime_modules() {
    esp_err_t rv = local_sensor_service_start();
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "local_sensor_service_start failed: %s", esp_err_to_name(rv));
        return rv;
    }

    if (!s_gui.is_ready) {
        ESP_LOGW(TAG, "GUI not ready after initialization");
    }

    return ESP_OK;
}

void app_main(void) {
    ESP_LOGI(TAG, "app_main entered");

    ESP_LOGI(TAG, "Initializing single-instance modules");
    if (init_single_instance_modules() != ESP_OK) {
        goto fatal_error;
    }

    ESP_LOGI(TAG, "Initializing runtime modules");
    if (init_runtime_modules() != ESP_OK) {
        goto fatal_error;
    }

    if (start_runtime_modules() != ESP_OK) {
        goto fatal_error;
    }

    // vTaskDelay(pdMS_TO_TICKS(5000));
    ESP_LOGI(TAG, "Startup complete");

    /* Pump the task scheduler to start tasks, needed to connect wifi in this mock code
     * GUI would trigger scheduler work in real project
     */
    while (1) {
        // Synchronize the GUI with the backend
        app_gui_bindings_sync(&s_gui);

        // Scheduler hard labor
        task_scheduler_work();
        vTaskDelay(pdMS_TO_TICKS(30));
    }
    return;

fatal_error:
    ESP_LOGE(TAG, "System halted during startup");
}
