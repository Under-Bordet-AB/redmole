/* NAC - Network and Communications
 * Handles WiFi and Bluetooth.
 *
 * @note
 * The GUI layer never touches wifi_ctx_t or bluetooth_ctx_t directly.
 * It calls only the nac_* abstraction functions declared at the bottom
 * of nac.h.  Those functions translate GUI-level intents ("connect",
 * "scan") into scheduler tasks and internal state transitions.
 *
 * The WiFi state machine lives entirely in wifi_connect(), which is the
 * single task_scheduler callback for all WiFi work.  A switch on
 * wifi_ctx_t::state selects the correct action each time the scheduler
 * invokes it.  Async events (WIFI_EVENT / IP_EVENT) update state and,
 * when a reconnect is needed, re-add the task node with an exponential
 * back-off delay.
 */

#include "nac.h"
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "nvs.h"
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

/* Internal forward declarations */

static void   wifi_event_handler(void *arg, esp_event_base_t event_base,
                                  int32_t event_id, void *event_data);
static int8_t wifi_bring_hw_online(wifi_ctx_t *self);
static int8_t wifi_bring_hw_offline(wifi_ctx_t *self);
static void   wifi_disconnect(wifi_ctx_t *self);
static void   wifi_reconnect(wifi_ctx_t *self);
static void   wifi_scan_done(wifi_ctx_t *self);

/* NAC - Network and communication section */

/**
 * @brief Initializes the NAC context (wifi and bluetooth).
 * @return A pointer to the initialized NAC context, or NULL on failure.
 */
nac_ctx_t *nac_init(void)
{
    nac_ctx_t *self = (nac_ctx_t *)malloc(sizeof(nac_ctx_t));
    if (!self)
    {
        ESP_LOGE("NAC", "Failed to allocate nac_ctx_t");
        return NULL;
    }
    memset(self, 0, sizeof(nac_ctx_t));

    if (wifi_init(&self->wifi) != 0)
    {
        ESP_LOGE("NAC", "wifi_init failed");
        free(self);
        return NULL;
    }

    if (bluetooth_init(&self->bluetooth) != 0)
    {
        ESP_LOGE("NAC", "bluetooth_init failed");
        wifi_dispose(&self->wifi);
        free(self);
        return NULL;
    }

    return self;
}

/**
 * @brief Disposes of the NAC context, freeing all resources.
 * @param self The NAC context to dispose of.
 */
void nac_dispose(nac_ctx_t *self)
{
    if (!self) return;
    wifi_dispose(&self->wifi);
    bluetooth_dispose(&self->bluetooth);
    free(self);
}


/* Public API for NAC */

/**
 * @brief Returns the current WiFi status of the NAC.
 * @param self The NAC context.
 * @return The current WiFi status.
 */
nac_wifi_status_t nac_get_wifi_status(const nac_ctx_t *self)
{
    if (!self) return NAC_WIFI_DISCONNECTED;

    switch (self->wifi.state)
    {
        case WIFI_STATE_CONNECTED:  return NAC_WIFI_CONNECTED;
        case WIFI_STATE_CONNECTING:  /* fall-through */
        case WIFI_STATE_RECONNECT:  return NAC_WIFI_CONNECTING;
        case WIFI_STATE_START_SCAN:  /* fall-through */
        case WIFI_STATE_SCANNING:  return NAC_WIFI_SCANNING;
        case WIFI_STATE_IDLE:      return NAC_WIFI_DISCONNECTED;
        case WIFI_STATE_ERROR:  return NAC_WIFI_ERROR;
        default:  return NAC_WIFI_DISCONNECTED;
    }
}

/**
 * @brief Requests a WiFi connection.
 * @param self The NAC context.
 * @return 0 on success, -1 on failure.
 */
int8_t nac_request_wifi_connect(nac_ctx_t *self)
{
    if (!self) return -1;

    wifi_state_t s = self->wifi.state;
    if (s == WIFI_STATE_CONNECTED || s == WIFI_STATE_CONNECTING)
    {
        ESP_LOGI("NAC", "WiFi already connected/connecting — ignoring");
        return 0;
    }

    self->wifi.retry_count    = 0;
    self->wifi.state          = WIFI_STATE_IDLE;
    self->wifi.task_node.work = wifi_connect;

    return task_scheduler_add(&self->wifi.task_node, 0);
}

/**
 * @brief Requests a WiFi disconnect.
 * @param self The NAC context.
 * @return 0 on success, -1 on failure.
 */
int8_t nac_request_wifi_disconnect(nac_ctx_t *self)
{
    if (!self) return -1;
    wifi_disconnect(&self->wifi);
    return 0;
}

/**
 * @brief Requests a WiFi scan.
 * @param self The NAC context.
 * @return 0 on success, -1 on failure.
 */
int8_t nac_request_wifi_scan(nac_ctx_t *self)
{
    if (!self) return -1;

    if (self->wifi.state == WIFI_STATE_SCANNING || self->wifi.state == WIFI_STATE_START_SCAN)
    {
        ESP_LOGI("NAC", "Scan already in progress — ignoring");
        return 0;
    }
    self->wifi.scan_complete  = 0;
    self->wifi.ap_count       = 0;
    self->wifi.state          = WIFI_STATE_START_SCAN;
    self->wifi.task_node.work = wifi_connect;

    return task_scheduler_add(&self->wifi.task_node, 0);
}

/**
 * @brief Returns the scan results.
 * @param self The NAC context.
 * @param out_count Pointer to store the number of scan results.
 * @return Pointer to the scan results, or NULL if none.
 */
const wifi_ap_record_t *nac_get_scan_results(const nac_ctx_t *self, uint16_t *out_count)
{
    if (!self || !out_count) return NULL;
    *out_count = self->wifi.ap_count;
    return self->wifi.ap_records;
}

/* WiFi section */

/**
 * @brief Initializes the WiFi interface.
 * @param self The WiFi context.
 * @return 0 on success, -1 on failure.
 * @note WiFi Initialization Order and where the call lives in the project:
 * 1. [SYSTEM] nvs_flash_init() (app_init)              - Flash storage for PHY/Radio calibration & creds.
 * 2. [STACK]  esp_netif_init() (app_init)              - TCP/IP (LwIP) global stack initialization.
 * 3. [SYSTEM] esp_event_loop_...() (app_init)          - System-wide async event dispatcher.
 * 4. [BIND]   esp_netif_create_...() (wifi_init)       - Creates the 'glue' between LwIP and the WiFi driver.
 * 5. [HAL]    esp_wifi_init() (wifi_bring_hw_online)   - Allocates WiFi buffers and starts the WiFi driver task.
 * 6. [RADIO]  esp_wifi_start() (wifi_connect)          - Powers on the physical Radio/PHY hardware.
 */
int8_t wifi_init(wifi_ctx_t *self)
{
    if (!self) return -1;

    self->netif = esp_netif_create_default_wifi_sta();
    if (!self->netif)
    {
        ESP_LOGE("WIFI", "Failed to create default STA netif");
        return -1;
    }

    /*
     * Pre-allocate scan buffer in PSRAM once.  wifi_scan_done() writes
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

    /* TO DO: switch these to RedMole nvs */
    esp_err_t nvs_err = nvs_open("wifi_creds", NVS_READONLY, &self->nvs_handle);
    if (nvs_err != ESP_OK && nvs_err != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGW("WIFI", "NVS open failed (0x%x) — sdkconfig fallback will be used", nvs_err);
    }

    self->state           = WIFI_STATE_IDLE;
    self->tag             = "WIFI";
    self->retry_count     = 0;
    self->saved_to_nvs    = 0;
    self->hw_online       = 0;
    self->ap_count        = 0;
    /* The function to call when the task is run */
    self->task_node.work  = wifi_connect;

    return 0;
}

/**
 * @brief Disposes of the WiFi context, freeing all allocated resources.
 * @param self The WiFi context.
 */
void wifi_dispose(wifi_ctx_t *self)
{
    if (!self) return;

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

    nvs_close(self->nvs_handle);

    if (self->netif)
    {
        esp_netif_destroy(self->netif);
        self->netif = NULL;
    }

    ESP_LOGI(self->tag, "Disposed");
}

/**
 * @brief Initialises the WiFi driver, registers event handlers, sets STA mode.
 * @param self The WiFi context.
 * @return 0 on success, -1 on failure.
 * @note Please note that the final call to esp_wifi_start() is left to wifi_connect()
 */
static int8_t wifi_bring_hw_online(wifi_ctx_t *self)
{
    if (self->hw_online) return 0;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    /* Initialise the WiFi driver with default configuration and starts the WiFi task */
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
    /* Set WiFi mode to STA (Station mode) */
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
 * @brief Tears down the WiFi driver.  Safe to call when already offline.
 * @param self The WiFi context.
 * @return 0 on success, -1 on failure.
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
    /* Stop the WiFi driver */
    if (esp_wifi_stop() != ESP_OK)
    {
        ESP_LOGE(self->tag, "esp_wifi_stop failed");
        return -1;
    }

    /* Unregister before deinit to prevent late callbacks */
    esp_event_handler_unregister(IP_EVENT,   ESP_EVENT_ANY_ID, wifi_event_handler);
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler);
    /* Frees all resources allocated by the WiFi driver and stops the task */
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
 * @return TASK_DONE      Work for this invocation is complete. The node
 *                        is removed from the scheduler; it will be re-added
 *                        only if reconnection is required.
 * @return TASK_ERROR     A non-recoverable driver call failed, or the retry
 *                        limit was reached. The node is removed from the
 *                        scheduler. Call nac_request_wifi_connect() to retry.
 *
 * @note TASK_RUN_AGAIN is intentionally unused. All rescheduling goes
 *       through task_scheduler_add() with an explicit delay so that
 *       reconnect back-off is enforced at the call site, not here.
 */
task_status_t wifi_connect(task_node_t *task_node)
{
    wifi_ctx_t *self = container_of(task_node, wifi_ctx_t, task_node);

    switch (self->state)
    {
        case WIFI_STATE_IDLE:
        case WIFI_STATE_RECONNECT:
        {
            if (self->retry_count >= REDMOLE_MAX_RETRY)
            {
                ESP_LOGE(self->tag, "Max retries (%d) reached — aborting", REDMOLE_MAX_RETRY);
                self->retry_count = 0;
                self->state       = WIFI_STATE_IDLE;
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
             * TODO: NVS credential lookup
             *
             * Attempt to read saved credentials from NVS before
             * falling back to the sdkconfig compile-time values:
             *
             *   char ssid[WIFI_SSID_MAX_LENGTH + 1] = REDMOLE_WIFI_SSID;
             *   char pass[WIFI_PASS_MAX_LENGTH + 1] = REDMOLE_WIFI_PASS;
             *   size_t ssid_len = sizeof(ssid);
             *   size_t pass_len = sizeof(pass);
             *   nvs_get_str(self->nvs_handle, "ssid",     ssid, &ssid_len);
             *   nvs_get_str(self->nvs_handle, "password", pass, &pass_len);
             *
             * On IP_EVENT_STA_GOT_IP (below), if saved_to_nvs == 0:
             *   nvs_handle_t rw;
             *   nvs_open("wifi_creds", NVS_READWRITE, &rw);
             *   nvs_set_str(rw, "ssid",     (char*)wifi_config.sta.ssid);
             *   nvs_set_str(rw, "password", (char*)wifi_config.sta.password);
             *   nvs_commit(rw);
             *   nvs_close(rw);
             *   self->saved_to_nvs = 1;
             * -------------------------------------------------- */

            /* These are defaults values for the WiFi configuration if no saved credentials are found */
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
                /* Clean up and return error */
                wifi_bring_hw_offline(self);
                return TASK_ERROR;
            }

            /* Ensuring we're the correct state before starting WiFi
             * The call posts WIFI_EVENT_STA_START to the event loop queue
             * before returning.  Once it returns, the event loop task can
             * preempt us and dispatch the event before we get another
             * instruction.
             */
            self->state = WIFI_STATE_CONNECTING;
            self->retry_count++;
            ESP_LOGI(self->tag, "Starting WiFi (attempt %d/%d)",
                     self->retry_count, REDMOLE_MAX_RETRY);

            /* This is where the radio is powered on and WiFi is started, the last step of the setup process */
            if (esp_wifi_start() != ESP_OK)
            {
                ESP_LOGE(self->tag, "esp_wifi_start failed");
                wifi_bring_hw_offline(self);
                self->state = WIFI_STATE_IDLE;
                return TASK_ERROR;
            }
            /* Disable power saving mode to keep the radio awake, will consume more power */
            esp_wifi_set_ps(WIFI_PS_NONE);

            /* Event handler drives everything from here. Task is re-added to the scheduler only by wifi_reconnect(). */
            return TASK_DONE;
        }

        /* WiFi scan requested */
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

        /* Entirely event-driven states */
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
 * @brief Explicit user-requested disconnect. Sets WIFI_STATE_IDLE before
 * tearing down hardware so the resulting disconnect event is ignored.
 */
static void wifi_disconnect(wifi_ctx_t *self)
{
    self->state = WIFI_STATE_IDLE;
    if (wifi_bring_hw_offline(self) != 0) {
        ESP_LOGE(self->tag, "Failed to bring WiFi hardware offline");
        return;
    }
    ESP_LOGI(self->tag, "Disconnected (user request)");
}

/**
 * @brief Schedules the next connect attempt with exponential back-off.
 *
 * Delay: 500 ms × 2^attempt, capped at 32 s.
 * retry_count was already incremented by the last wifi_connect() call.
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
 * @brief Callback invoked when a Wi-Fi scan completes.
 *
 * Reads the scan results into the pre-allocated PSRAM buffer and
 * transitions the state machine to WIFI_STATE_IDLE.
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

    self->state = WIFI_STATE_IDLE;
    self->scan_complete = 1;
    ESP_LOGI(self->tag, "Scan complete: %" PRIu16 " AP(s) found", self->ap_count);
}

/**
 * @brief Event handler for WIFI_EVENT and IP_EVENT.
 *
 * arg is always wifi_ctx_t *.
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
                /* Check that we are in the correct state before attempting to connect. */
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
                if (self->state == WIFI_STATE_CONNECTING ||
                    self->state == WIFI_STATE_CONNECTED  ||
                    self->state == WIFI_STATE_RECONNECT)
                {
                    wifi_event_sta_disconnected_t *d = (wifi_event_sta_disconnected_t *)event_data;
                    ESP_LOGW(self->tag, "Disconnected, reason: %d, scheduling reconnect", d->reason);
                    wifi_reconnect(self);
                }
                else
                {
                    ESP_LOGI(self->tag, "Disconnect event ignored (state = %d)", (int)self->state);
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

        /* TODO: NVS write-back — see comment block in wifi_connect() */
    }
}

/* Bluetooth */

int8_t bluetooth_init(bluetooth_ctx_t *self)
{
    if (!self) return -1;

    self->state                 = BLUETOOTH_STATE_IDLE;
    self->bluetooth_event_group = xEventGroupCreate();

    if (!self->bluetooth_event_group)
    {
        ESP_LOGE("BT", "Failed to create Bluetooth event group");
        return -1;
    }

    return 0;
}

void bluetooth_dispose(bluetooth_ctx_t *self)
{
    if (!self) return;

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
