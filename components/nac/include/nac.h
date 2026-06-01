/**
 * @file
 * @brief Network and communications module (NAC).
 *
 * Manages the WiFi subsystem, including connection, scanning, and status
 * reporting. Integrates with the task scheduler for deferred WiFi work.
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
#include "uart_mole.h"

#define WIFI_SSID_MAX_LENGTH  32 /*!< Maximum SSID length in bytes, as defined by 802.11. */
#define WIFI_PASS_MAX_LENGTH  64 /*!< Maximum passphrase length in bytes. */
#define WIFI_SCAN_MAX_RESULT  20 /*!< Maximum number of AP records retained after a scan. */
#define WIFI_CRED_MAX_LENGTH  64 /*!< Maximum credential string length in bytes (NVS storage limit). */

/**
 * @brief Internal WiFi state machine states.
 */
typedef enum
{
    WIFI_STATE_IDLE = 0,   /*!< No WiFi activity in progress. */
    WIFI_STATE_CONNECTING, /*!< Association in progress. */
    WIFI_STATE_CONNECTED,  /*!< IP address acquired. */
    WIFI_STATE_RECONNECT,  /*!< Reconnection after an unexpected disconnect. */
    WIFI_STATE_START_SCAN, /*!< Scan requested but not yet started. */
    WIFI_STATE_SCANNING,   /*!< Scan is in progress. */
    WIFI_STATE_ERROR,      /*!< Unrecoverable driver error. */
} wifi_state_t;

/**
 * @brief WiFi module context, embedded in the NAC module state.
 */
typedef struct wifi_ctx
{
    task_node_t       task_node;     /*!< Scheduler node; must be first so task callbacks can use container_of. */
    wifi_state_t      state;         /*!< Current WiFi state machine state. */
    wifi_ap_record_t *ap_records;    /*!< PSRAM buffer of WIFI_SCAN_MAX_RESULT AP records, allocated at init. */
    uint16_t          ap_count;      /*!< Number of valid entries in ap_records after the last scan. */
    esp_netif_t      *netif;         /*!< Netif handle created at init, destroyed at dispose. */
    const char       *tag;           /*!< Log tag string, not owned by this struct. */
    uint8_t           retry_count;   /*!< Number of consecutive connection attempts since last success. */
    uint8_t           saved_to_nvs;  /*!< Non-zero if the current credentials have been persisted to NVS. */
    uint8_t           hw_online;     /*!< Non-zero if the WiFi driver is running. */
    uint8_t           scan_complete; /*!< Set by wifi_scan_done(), cleared by nac_request_wifi_scan(). */
} wifi_ctx_t;

/**
 * @brief Simplified WiFi status reported to callers outside the NAC module.
 */
typedef enum
{
    NAC_WIFI_DISCONNECTED = 0, /*!< Not connected and not scanning. */
    NAC_WIFI_CONNECTING,       /*!< Association or DHCP in progress. */
    NAC_WIFI_CONNECTED,        /*!< IP address acquired and link is up. */
    NAC_WIFI_SCANNING,         /*!< AP scan is in progress. */
    NAC_WIFI_ERROR,            /*!< Driver error; requires reinitialisation. */
} nac_wifi_status_t;

/**
 * @brief Initialise the NAC module.
 *
 * Also calls esp_netif_init() and esp_event_loop_create_default(). Must be
 * called before any other nac_ function.
 *
 * @param event_group Pointer to an application-owned event group used to signal
 *                    WiFi state changes; must not be NULL.
 * @return ESP_OK on success, an esp_err_t error code on failure.
 */
esp_err_t nac_init(EventGroupHandle_t *event_group);

/**
 * @brief Tear down the NAC module and release all resources.
 */
void nac_dispose(void);

/**
 * @brief Returns the simplified WiFi status for the GUI.
 *
 * @return The current WiFi status as a nac_wifi_status_t value.
 */
nac_wifi_status_t nac_get_wifi_status(void);

/**
 * @brief Schedule a WiFi connection attempt.
 *
 * Queues a connect task with the given credentials. Credentials longer than
 * WIFI_SSID_MAX_LENGTH or WIFI_PASS_MAX_LENGTH are truncated.
 *
 * @param ssid     Null-terminated SSID string; must not be NULL.
 * @param password Null-terminated passphrase; must not be NULL, pass "" for open networks.
 * @return ESP_OK on success, an esp_err_t error code on failure.
 */
esp_err_t nac_request_wifi_connect(const char *ssid, const char *password);

/**
 * @brief Explicitly disconnect WiFi. Safe to call from any state.
 *
 * @return ESP_OK on success, an esp_err_t error code on failure.
 */
esp_err_t nac_request_wifi_disconnect(void);

/**
 * @brief Schedule a WiFi scan. No-op if a scan is already in progress.
 *
 * Poll nac_scan_is_complete() or check nac_get_wifi_status() to determine
 * when the scan finishes.
 *
 * @return ESP_OK on success, an esp_err_t error code on failure.
 * @see nac_get_scan_results
 * @see nac_scan_is_complete
 */
esp_err_t nac_request_wifi_scan(void);

/**
 * @brief Return the last completed scan results.
 *
 * Valid only after a scan completes (nac_scan_is_complete() returns true).
 * The buffer is PSRAM-allocated at init and reused on each scan.
 *
 * @param out_count Populated with the number of valid entries; must not be NULL.
 * @return Pointer to the AP record buffer, or NULL on error.
 * @see nac_request_wifi_scan
 */
const wifi_ap_record_t *nac_get_scan_results(uint16_t *out_count);

/**
 * @brief Returns true if the last requested scan has completed.
 *
 * Cleared automatically when nac_request_wifi_scan() is called again.
 *
 * @return True if scan results are ready, false if a scan is in progress or none was requested.
 */
bool nac_scan_is_complete(void);

#endif /* NAC_H */
