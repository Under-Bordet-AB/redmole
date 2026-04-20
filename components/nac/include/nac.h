/* NAC - Network and Communications
 * Handles WiFi and Bluetooth.
 *
 * This module follows the same single-instance pattern as rm_nvs and
 * sensor_data. Call nac_init() once during startup; all subsequent
 * calls use the module-internal state directly with no context pointer.
 *
 * Prerequisites (must be called before nac_init, owned by main.c):
 *   rm_nvs_init()
 *   esp_netif_init()
 *   esp_event_loop_create_default()
 */

#ifndef NAC_H
#define NAC_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "task_scheduler.h"

#define WIFI_SSID_MAX_LENGTH  32
#define WIFI_PASS_MAX_LENGTH  64
#define WIFI_SCAN_MAX_RESULT  20

/* ------------------------------------------------------------------ */
/*  Internal types — visible so gui_module can use nac_wifi_status_t  */
/*  but gui never touches wifi_ctx_t or nac_ctx_t directly.           */
/* ------------------------------------------------------------------ */

typedef enum
{
    WIFI_STATE_IDLE = 0,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_RECONNECT,
    WIFI_STATE_START_SCAN,
    WIFI_STATE_SCANNING,
    WIFI_STATE_ERROR,
} wifi_state_t;

typedef enum
{
    BLUETOOTH_STATE_IDLE = 0,
    BLUETOOTH_STATE_DISCONNECTED,
    BLUETOOTH_STATE_CONNECTING,
    BLUETOOTH_STATE_CONNECTED,
    BLUETOOTH_STATE_RECONNECTING,
} bluetooth_state_t;

typedef struct wifi_ctx
{
    task_node_t       task_node;
    wifi_state_t      state;
    wifi_ap_record_t *ap_records;   /* PSRAM, WIFI_SCAN_MAX_RESULT entries, allocated at init */
    uint16_t          ap_count;
    esp_netif_t      *netif;
    const char       *tag;
    uint8_t           retry_count;
    uint8_t           saved_to_nvs;
    uint8_t           hw_online;
    uint8_t           scan_complete; /* set by wifi_scan_done(), cleared by nac_request_wifi_scan() */
} wifi_ctx_t;

typedef struct bluetooth_ctx
{
    bluetooth_state_t  state;
    EventGroupHandle_t bluetooth_event_group;
} bluetooth_ctx_t;

/* ------------------------------------------------------------------ */
/*  Public API — no context pointer, module owns all internal state   */
/* ------------------------------------------------------------------ */

typedef enum
{
    NAC_WIFI_DISCONNECTED = 0,
    NAC_WIFI_CONNECTING,
    NAC_WIFI_CONNECTED,
    NAC_WIFI_SCANNING,
    NAC_WIFI_ERROR,
} nac_wifi_status_t;

/**
 * @brief Initialise the NAC module (WiFi + Bluetooth).
 *
 * Must be called after rm_nvs_init(), esp_netif_init(), and
 * esp_event_loop_create_default(). Idiomatic with the rest of the
 * project: returns ESP_OK on success, an error code on failure.
 */
esp_err_t nac_init(void);

/**
 * @brief Tear down the NAC module and release all resources.
 */
void nac_dispose(void);

/** @brief Returns the simplified WiFi status for the GUI. */
nac_wifi_status_t nac_get_wifi_status(void);

/**
 * @brief Schedule a WiFi connection attempt.
 *        No-op if already connecting or connected.
 */
esp_err_t nac_request_wifi_connect(void);

/**
 * @brief Explicitly disconnect WiFi. Safe to call from any state.
 */
esp_err_t nac_request_wifi_disconnect(void);

/**
 * @brief Schedule a WiFi scan. No-op if a scan is already in progress.
 *        Check scan_complete or nac_get_wifi_status() to know when done.
 */
esp_err_t nac_request_wifi_scan(void);

/**
 * @brief Returns the last completed scan results.
 *        Valid only after scan_complete is set (status != NAC_WIFI_SCANNING).
 *
 * @param out_count  Populated with the number of valid entries.
 * @return Pointer to the PSRAM AP record buffer, or NULL on error.
 */
const wifi_ap_record_t *nac_get_scan_results(uint16_t *out_count);

/**
 * @brief Returns true if the last requested scan has completed.
 *        Cleared automatically when nac_request_wifi_scan() is called again.
 */
bool nac_scan_is_complete(void);

#endif /* NAC_H */
