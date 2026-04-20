/* NAC - Network and Communications
 * Handles WiFi and Bluetooth.
 *
 * The WiFi state machine lives entirely in wifi_connect(), the single
 * task_scheduler callback for all WiFi work. Async events (WIFI_EVENT /
 * IP_EVENT) update state and re-add the task node when more work is needed.
 */

#include "nac.h"
#include "esp_netif_types.h"
#include "rm_nvs.h"
#include <string.h>
#include <inttypes.h>
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#define REDMOLE_WIFI_SSID   CONFIG_REDMOLE_WIFI_SSID
#define REDMOLE_WIFI_PASS   CONFIG_REDMOLE_WIFI_PASSWORD
#define REDMOLE_MAX_RETRY   CONFIG_REDMOLE_MAXIMUM_RETRY

#if CONFIG_REDMOLE_WPA3_SAE_PWE_HUNT_AND_PECK
    #define REDMOLE_WPA3_SAE_MODE  WPA3_SAE_PWE_HUNT_AND_PECK
    #define REDMOLE_H2E_IDENTIFIER ""
#elif CONFIG_REDMOLE_WPA3_SAE_PWE_HASH_TO_ELEMENT
    #define REDMOLE_WPA3_SAE_MODE  WPA3_SAE_PWE_HASH_TO_ELEMENT
    #define REDMOLE_H2E_IDENTIFIER CONFIG_REDMOLE_WIFI_PW_ID
#elif CONFIG_REDMOLE_WPA3_SAE_PWE_BOTH
    #define REDMOLE_WPA3_SAE_MODE  WPA3_SAE_PWE_BOTH
    #define REDMOLE_H2E_IDENTIFIER CONFIG_REDMOLE_WIFI_PW_ID
#else
    #define REDMOLE_WPA3_SAE_MODE  WPA3_SAE_PWE_HUNT_AND_PECK
    #define REDMOLE_H2E_IDENTIFIER ""
#endif

#if CONFIG_REDMOLE_AUTH_OPEN
    #define REDMOLE_AUTH_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_REDMOLE_AUTH_WEP
    #define REDMOLE_AUTH_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_REDMOLE_AUTH_WPA_PSK
    #define REDMOLE_AUTH_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_REDMOLE_AUTH_WPA2_PSK
    #define REDMOLE_AUTH_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_REDMOLE_AUTH_WPA_WPA2_PSK
    #define REDMOLE_AUTH_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_REDMOLE_AUTH_WPA3_PSK
    #define REDMOLE_AUTH_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_REDMOLE_AUTH_WPA2_WPA3_PSK
    #define REDMOLE_AUTH_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_REDMOLE_AUTH_WAPI_PSK
    #define REDMOLE_AUTH_THRESHOLD WIFI_AUTH_WAPI_PSK
#else
    #define REDMOLE_AUTH_THRESHOLD WIFI_AUTH_WPA2_PSK
#endif

typedef struct
{
    wifi_ctx_t      wifi;
    bluetooth_ctx_t bluetooth;
} nac_ctx_t;

static nac_ctx_t s_nac;

/*  Internal forward declarations */

static void   wifi_event_handler(void *arg, esp_event_base_t event_base,
                                  int32_t event_id, void *event_data);
static int8_t wifi_bring_hw_online(wifi_ctx_t *self);
static int8_t wifi_bring_hw_offline(wifi_ctx_t *self);
static void   wifi_disconnect(wifi_ctx_t *self);
static void   wifi_reconnect(wifi_ctx_t *self);
static void   wifi_scan_done(wifi_ctx_t *self);
static int8_t wifi_init(wifi_ctx_t *self);
static void   wifi_dispose(wifi_ctx_t *self);
task_status_t wifi_connect(task_node_t *node);
static int8_t bluetooth_init(bluetooth_ctx_t *self);
static void   bluetooth_dispose(bluetooth_ctx_t *self);

/*  Public API */

esp_err_t nac_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    memset(&s_nac, 0, sizeof(s_nac));

    if (wifi_init(&s_nac.wifi) != 0)
    {
        ESP_LOGE("NAC", "wifi_init failed");
        return ESP_FAIL;
    }

    if (bluetooth_init(&s_nac.bluetooth) != 0)
    {
        ESP_LOGE("NAC", "bluetooth_init failed");
        wifi_dispose(&s_nac.wifi);
        return ESP_FAIL;
    }

    return ESP_OK;
}

void nac_dispose(void)
{
    wifi_dispose(&s_nac.wifi);
    bluetooth_dispose(&s_nac.bluetooth);
    memset(&s_nac, 0, sizeof(s_nac));
}

nac_wifi_status_t nac_get_wifi_status(void)
{
    switch (s_nac.wifi.state)
    {
        case WIFI_STATE_CONNECTED:                       return NAC_WIFI_CONNECTED;
        case WIFI_STATE_CONNECTING: /* fall-through */
        case WIFI_STATE_RECONNECT:                       return NAC_WIFI_CONNECTING;
        case WIFI_STATE_START_SCAN: /* fall-through */
        case WIFI_STATE_SCANNING:                        return NAC_WIFI_SCANNING;
        case WIFI_STATE_ERROR:                           return NAC_WIFI_ERROR;
        case WIFI_STATE_IDLE:       /* fall-through */
        default:                                         return NAC_WIFI_DISCONNECTED;
    }
}

esp_err_t nac_request_wifi_connect(void)
{
    wifi_state_t s = s_nac.wifi.state;
    if (s == WIFI_STATE_CONNECTED || s == WIFI_STATE_CONNECTING)
    {
        ESP_LOGI("NAC", "WiFi already connected/connecting — ignoring");
        return ESP_OK;
    }

    s_nac.wifi.retry_count    = 0;
    s_nac.wifi.state          = WIFI_STATE_IDLE;
    s_nac.wifi.task_node.work = wifi_connect;

    return task_scheduler_add(&s_nac.wifi.task_node, 0) == 0 ? ESP_OK : ESP_FAIL;
}

esp_err_t nac_request_wifi_disconnect(void)
{
    wifi_disconnect(&s_nac.wifi);
    return ESP_OK;
}

esp_err_t nac_request_wifi_scan(void)
{
    wifi_ctx_t *wifi = &s_nac.wifi;

    if (wifi->state == WIFI_STATE_SCANNING || wifi->state == WIFI_STATE_START_SCAN)
    {
        ESP_LOGI("NAC", "Scan already in progress — ignoring");
        return ESP_OK;
    }

    wifi->ap_count        = 0;
    wifi->scan_complete   = 0;
    wifi->state           = WIFI_STATE_START_SCAN;
    wifi->task_node.work  = wifi_connect;

    return task_scheduler_add(&wifi->task_node, 0) == 0 ? ESP_OK : ESP_FAIL;
}

const wifi_ap_record_t *nac_get_scan_results(uint16_t *out_count)
{
    if (!out_count) return NULL;
    *out_count = s_nac.wifi.ap_count;
    return s_nac.wifi.ap_records;
}

bool nac_scan_is_complete(void)
{
    return s_nac.wifi.scan_complete != 0;
}

/*  WiFi — internal implementation */

/**
 * @brief Initialises the WiFi interface.
 * @note  Initialization order and ownership:
 *   1. [SYSTEM] nvs_flash_init()           rm_nvs_init() in main.c
 *   2. [STACK]  esp_netif_init()           nac_init()
 *   3. [SYSTEM] esp_event_loop_...()       nac_init()
 *   4. [BIND]   esp_netif_create_...()     here  — glue between LwIP and WiFi driver
 *   5. [HAL]    esp_wifi_init()            wifi_bring_hw_online()
 *   6. [RADIO]  esp_wifi_start()           wifi_connect()
 * @return 0 on success, -1 on failure
 */
static int8_t wifi_init(wifi_ctx_t *self)
{
    self->netif = esp_netif_create_default_wifi_sta();
    if (!self->netif)
    {
        ESP_LOGE("WIFI", "Failed to create default STA netif");
        return -1;
    }

    /*
     * Pre-allocate scan buffer in PSRAM once. wifi_scan_done() writes
     * directly into this buffer — no stack allocation, no copy.
     */
    self->ap_records = (wifi_ap_record_t *)heap_caps_malloc(
        WIFI_SCAN_MAX_RESULT * sizeof(wifi_ap_record_t), MALLOC_CAP_SPIRAM);

    if (!self->ap_records)
    {
        ESP_LOGE("WIFI", "Failed to allocate scan buffer in PSRAM");
        esp_netif_destroy(self->netif);
        self->netif = NULL;
        return -1;
    }

    self->state          = WIFI_STATE_IDLE;
    self->tag            = "WIFI";
    self->retry_count    = 0;
    self->saved_to_nvs   = 0;
    self->hw_online      = 0;
    self->ap_count       = 0;
    self->scan_complete  = 0;
    self->task_node.work = wifi_connect;

    return 0;
}

static void wifi_dispose(wifi_ctx_t *self)
{
    if (self->task_node.active)
    {
        task_scheduler_remove(&self->task_node);
    }

    wifi_bring_hw_offline(self);

    if (self->ap_records)
    {
        heap_caps_free(self->ap_records);
        self->ap_records = NULL;
    }

    if (self->netif)
    {
        esp_netif_destroy(self->netif);
        self->netif = NULL;
    }

    ESP_LOGI(self->tag, "Disposed");
}

/**
 * @brief Initialises the WiFi driver, registers event handlers, sets STA mode.
 * @note  esp_wifi_start() is left to wifi_connect() — the scheduler controls timing.
 * @return 0 on success, -1 on failure
 */
static int8_t wifi_bring_hw_online(wifi_ctx_t *self)
{
    if (self->hw_online) return 0;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&cfg) != ESP_OK)
    {
        ESP_LOGE(self->tag, "esp_wifi_init failed");
        return -1;
    }

    if (esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                   wifi_event_handler, self) != ESP_OK)
    {
        ESP_LOGE(self->tag, "Failed to register WIFI_EVENT handler");
        esp_wifi_deinit();
        return -1;
    }

    if (esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID,
                                   wifi_event_handler, self) != ESP_OK)
    {
        ESP_LOGE(self->tag, "Failed to register IP_EVENT handler");
        esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler);
        esp_wifi_deinit();
        return -1;
    }

    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK)
    {
        ESP_LOGE(self->tag, "Failed to set WiFi mode STA");
        esp_event_handler_unregister(IP_EVENT,   ESP_EVENT_ANY_ID, wifi_event_handler);
        esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler);
        esp_wifi_deinit();
        return -1;
    }

    self->hw_online = 1;
    ESP_LOGI(self->tag, "Hardware online");
    return 0;
}

/**
 * @brief Tears down the WiFi driver. Safe to call when already offline.
 * @return 0 on success, -1 on failure
 */
static int8_t wifi_bring_hw_offline(wifi_ctx_t *self)
{
    if (!self->hw_online) return 0;

    if (self->state == WIFI_STATE_SCANNING)
    {
        esp_wifi_scan_stop();
    }

    if (self->state == WIFI_STATE_CONNECTED ||
        self->state == WIFI_STATE_CONNECTING)
    {
        esp_wifi_disconnect();
    }

    if (esp_wifi_stop() != ESP_OK)
    {
        ESP_LOGE(self->tag, "esp_wifi_stop failed");
        return -1;
    }

    /* Unregister before deinit to prevent late callbacks */
    esp_event_handler_unregister(IP_EVENT,   ESP_EVENT_ANY_ID, wifi_event_handler);
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler);

    if (esp_wifi_deinit() != ESP_OK)
    {
        ESP_LOGE(self->tag, "esp_wifi_deinit failed");
        return -1;
    }

    self->hw_online = 0;
    self->state     = WIFI_STATE_IDLE;
    ESP_LOGI(self->tag, "Hardware offline");
    return 0;
}

/**
 * @brief Cooperative task driving the WiFi connection state machine.
 *
 * Scheduled by nac_request_wifi_connect() and nac_request_wifi_scan().
 * Switches on wifi_ctx_t::state to determine the current phase of work,
 * then returns control to the scheduler. Further progress is triggered
 * either by the scheduler re-adding this node (reconnect back-off) or
 * by the WiFi event handler updating state and re-adding the node
 * (WIFI_EVENT_STA_DISCONNECTED).
 *
 * @param task_node  Scheduler node embedded in wifi_ctx_t. Recover the
 *                   parent context with:
 *                   container_of(task_node, wifi_ctx_t, task_node)
 *
 * @return TASK_DONE   Invocation complete; node removed from scheduler.
 *                     Re-added automatically if reconnection is needed.
 * @return TASK_ERROR  Driver failure or retry limit reached; node removed.
 *                     Call nac_request_wifi_connect() to start over.
 *
 * @note TASK_RUN_AGAIN is unused — all rescheduling goes through
 *       task_scheduler_add() so back-off delay is enforced explicitly.
 */
task_status_t wifi_connect(task_node_t *task_node)
{
    wifi_ctx_t *self = container_of(task_node, wifi_ctx_t, task_node);

    switch (self->state)
    {
        /*  Initial connect or scheduled reconnect */
        case WIFI_STATE_IDLE:
        case WIFI_STATE_RECONNECT:
        {
            if (self->retry_count >= REDMOLE_MAX_RETRY)
            {
                ESP_LOGE(self->tag, "Max retries (%d) reached — aborting", REDMOLE_MAX_RETRY);
                self->retry_count = 0;
                self->state       = WIFI_STATE_ERROR;
                return TASK_ERROR;
            }

            if (wifi_bring_hw_offline(self) != 0)
            {
                ESP_LOGE(self->tag, "Could not bring hardware offline");
                return TASK_ERROR;
            }

            vTaskDelay(pdMS_TO_TICKS(50));

            if (wifi_bring_hw_online(self) != 0)
            {
                ESP_LOGE(self->tag, "Could not bring hardware online");
                return TASK_ERROR;
            }

            /* --------------------------------------------------
             * TODO: NVS credential lookup via rm_nvs
             *
             * Replace sdkconfig fallback values with stored credentials:
             *
             *   char ssid[WIFI_SSID_MAX_LENGTH + 1] = REDMOLE_WIFI_SSID;
             *   char pass[WIFI_PASS_MAX_LENGTH + 1]  = REDMOLE_WIFI_PASS;
             *   size_t ssid_len = sizeof(ssid);
             *   size_t pass_len = sizeof(pass);
             *   rm_nvs_get_str("wifi_ssid", ssid, &ssid_len);
             *   rm_nvs_get_str("wifi_pass", pass, &pass_len);
             *
             * On IP_EVENT_STA_GOT_IP, if saved_to_nvs == 0:
             *   rm_nvs_set_str("wifi_ssid", (char *)wifi_config.sta.ssid);
             *   rm_nvs_set_str("wifi_pass", (char *)wifi_config.sta.password);
             *   self->saved_to_nvs = 1;
             * -------------------------------------------------- */

            wifi_config_t wifi_config;
            memset(&wifi_config, 0, sizeof(wifi_config));
            wifi_config.sta.threshold.authmode = REDMOLE_AUTH_THRESHOLD;
            wifi_config.sta.sae_pwe_h2e        = REDMOLE_WPA3_SAE_MODE;

            strncpy((char *)wifi_config.sta.ssid,
                    REDMOLE_WIFI_SSID,
                    sizeof(wifi_config.sta.ssid) - 1);
            strncpy((char *)wifi_config.sta.password,
                    REDMOLE_WIFI_PASS,
                    sizeof(wifi_config.sta.password) - 1);
            strncpy((char *)wifi_config.sta.sae_h2e_identifier,
                    REDMOLE_H2E_IDENTIFIER,
                    sizeof(wifi_config.sta.sae_h2e_identifier) - 1);

            if (esp_wifi_set_config(WIFI_IF_STA, &wifi_config) != ESP_OK)
            {
                ESP_LOGE(self->tag, "esp_wifi_set_config failed");
                wifi_bring_hw_offline(self);
                return TASK_ERROR;
            }

            /*
             * Set CONNECTING before esp_wifi_start(). The call posts
             * WIFI_EVENT_STA_START to the event loop queue before returning.
             * Once it returns, the event loop task can preempt us and dispatch
             * the event before we get another instruction. Setting state first
             * closes that window.
             */
            self->state = WIFI_STATE_CONNECTING;
            self->retry_count++;
            ESP_LOGI(self->tag, "Starting WiFi (attempt %d/%d)",
                     self->retry_count, REDMOLE_MAX_RETRY);

            if (esp_wifi_start() != ESP_OK)
            {
                ESP_LOGE(self->tag, "esp_wifi_start failed");
                wifi_bring_hw_offline(self);
                self->state = WIFI_STATE_IDLE;
                return TASK_ERROR;
            }

            /* Disable modem sleep avoids latency spikes on the 7" display */
            esp_wifi_set_ps(WIFI_PS_NONE);

            return TASK_DONE;
        }

        /* ---- WiFi scan requested ---- */
        case WIFI_STATE_START_SCAN:
        {
            if (!self->hw_online && wifi_bring_hw_online(self) != 0)
            {
                ESP_LOGE(self->tag, "Could not bring hardware online for scan");
                return TASK_ERROR;
            }

            esp_err_t err = esp_wifi_start();
            if (err != ESP_OK && err != ESP_ERR_WIFI_IF)
            {
                ESP_LOGE(self->tag, "esp_wifi_start failed: %s", esp_err_to_name(err));
                return TASK_ERROR;
            }

            wifi_scan_config_t scan_cfg;
            memset(&scan_cfg, 0, sizeof(scan_cfg));
            scan_cfg.scan_type = WIFI_SCAN_TYPE_ACTIVE;

            if (esp_wifi_scan_start(&scan_cfg, false) != ESP_OK)
            {
                ESP_LOGE(self->tag, "esp_wifi_scan_start failed");
                return TASK_ERROR;
            }

            self->state = WIFI_STATE_SCANNING;
            ESP_LOGI(self->tag, "Scan started");
            return TASK_DONE;
        }

        /* ---- Entirely event-driven states ---- */
        case WIFI_STATE_CONNECTING:
            ESP_LOGD(self->tag, "Connection in progress");
            return TASK_DONE;

        case WIFI_STATE_SCANNING:
            ESP_LOGD(self->tag, "Scan in progress");
            return TASK_DONE;

        case WIFI_STATE_CONNECTED:
            self->retry_count = 0;
            ESP_LOGD(self->tag, "Already connected");
            return TASK_DONE;

        default:
            ESP_LOGW(self->tag, "Unhandled state %d in wifi_connect", (int)self->state);
            return TASK_DONE;
    }
}

/**
 * @brief Explicit user-requested disconnect. Sets IDLE before tearing down
 *        hardware so the resulting disconnect event is ignored by the handler.
 */
static void wifi_disconnect(wifi_ctx_t *self)
{
    self->state = WIFI_STATE_IDLE;
    if (wifi_bring_hw_offline(self) != 0)
    {
        ESP_LOGE(self->tag, "Failed to bring WiFi hardware offline");
        return;
    }
    ESP_LOGI(self->tag, "Disconnected (user request)");
}

/**
 * @brief Schedules the next connect attempt with exponential back-off.
 *        Delay: 500 ms × 2^attempt, capped at 32 s.
 */
static void wifi_reconnect(wifi_ctx_t *self)
{
    uint8_t  exp      = (self->retry_count < 6) ? self->retry_count : 6;
    uint32_t delay_ms = 500u * (1u << exp);

    self->state = WIFI_STATE_RECONNECT;
    ESP_LOGI(self->tag, "Reconnect in %" PRIu32 " ms (attempt %d/%d)",
             delay_ms, self->retry_count, REDMOLE_MAX_RETRY);

    task_scheduler_add(&self->task_node, delay_ms);
}

/**
 * @brief Reads AP records from the driver into the pre-allocated PSRAM buffer.
 *        No stack allocation, no copy.
 */
static void wifi_scan_done(wifi_ctx_t *self)
{
    self->ap_count = WIFI_SCAN_MAX_RESULT;

    if (esp_wifi_scan_get_ap_records(&self->ap_count, self->ap_records) != ESP_OK)
    {
        ESP_LOGE(self->tag, "esp_wifi_scan_get_ap_records failed");
        self->ap_count = 0;
        self->state    = WIFI_STATE_IDLE;
        return;
    }

    self->scan_complete = 1;
    self->state         = WIFI_STATE_IDLE;
    ESP_LOGI(self->tag, "Scan complete: %" PRIu16 " AP(s) found", self->ap_count);
}

/**
 * @brief Event handler for WIFI_EVENT and IP_EVENT. arg is always wifi_ctx_t *.
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    wifi_ctx_t *self = (wifi_ctx_t *)arg;

    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
            case WIFI_EVENT_STA_START:
            {
                /*
                 * Only call esp_wifi_connect() if we asked for a connection.
                 * A scan also triggers STA_START but state will be SCANNING,
                 * not CONNECTING, so we correctly skip here in that case.
                 */
                if (self->state == WIFI_STATE_CONNECTING)
                {
                    esp_err_t err = esp_wifi_connect();
                    if (err != ESP_OK && err != ESP_ERR_WIFI_CONN)
                    {
                        ESP_LOGE(self->tag, "esp_wifi_connect failed: 0x%x", err);
                    }
                }
                break;
            }

            case WIFI_EVENT_STA_DISCONNECTED:
            {
                /*
                 * Reconnect only from active states. IDLE means the disconnect
                 * was intentional (wifi_disconnect() sets IDLE before tearing
                 * down hardware), so we leave it alone.
                 */
                if (self->state == WIFI_STATE_CONNECTING ||
                    self->state == WIFI_STATE_CONNECTED  ||
                    self->state == WIFI_STATE_RECONNECT)
                {
                    wifi_event_sta_disconnected_t *d =
                        (wifi_event_sta_disconnected_t *)event_data;
                    ESP_LOGW(self->tag, "Disconnected, reason: %d — scheduling reconnect",
                             d->reason);
                    wifi_reconnect(self);
                }
                else
                {
                    ESP_LOGI(self->tag, "Disconnect event ignored (state = %d)",
                             (int)self->state);
                }
                break;
            }

            case WIFI_EVENT_SCAN_DONE:
                wifi_scan_done(self);
                break;

            default:
                break;
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(self->tag, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        self->state       = WIFI_STATE_CONNECTED;
        self->retry_count = 0;

        /* TODO: NVS write-back via rm_nvs — see comment block in wifi_connect() */
    }
}

/*  Bluetooth */

static int8_t bluetooth_init(bluetooth_ctx_t *self)
{
    self->state                 = BLUETOOTH_STATE_IDLE;
    self->bluetooth_event_group = xEventGroupCreate();

    if (!self->bluetooth_event_group)
    {
        ESP_LOGE("BT", "Failed to create Bluetooth event group");
        return -1;
    }

    return 0;
}

static void bluetooth_dispose(bluetooth_ctx_t *self)
{
    if (self->bluetooth_event_group)
    {
        vEventGroupDelete(self->bluetooth_event_group);
        self->bluetooth_event_group = NULL;
    }

    /*
     * TODO: when Bluetooth is wired up, add:
     *   esp_bluedroid_disable();
     *   esp_bluedroid_deinit();
     *   esp_bt_controller_disable();
     *   esp_bt_controller_deinit();
     */

    ESP_LOGI("BT", "Disposed");
}
