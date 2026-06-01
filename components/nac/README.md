# NAC — Network and Communications

Manages the WiFi subsystem on the ESP32, including connection, disconnection, AP scanning, and status reporting. Integrates with the task scheduler for deferred WiFi work and signals state changes via a FreeRTOS event group.

## Overview

NAC wraps the ESP-IDF WiFi driver behind a simple request/status API. Internally it runs a state machine; externally it exposes a simplified four-state status suitable for the GUI.

### Internal state machine

| State | Meaning |
|---|---|
| `WIFI_STATE_IDLE` | No WiFi activity. |
| `WIFI_STATE_CONNECTING` | Association in progress. |
| `WIFI_STATE_CONNECTED` | IP address acquired. |
| `WIFI_STATE_RECONNECT` | Reconnecting after unexpected disconnect. |
| `WIFI_STATE_START_SCAN` | Scan requested, not yet started. |
| `WIFI_STATE_SCANNING` | Scan in progress. |
| `WIFI_STATE_ERROR` | Unrecoverable driver error; requires reinit. |

### Public status (`nac_wifi_status_t`)

Returned by `nac_get_wifi_status()` for use outside the module.

| Value | Meaning |
|---|---|
| `NAC_WIFI_DISCONNECTED` | Not connected and not scanning. |
| `NAC_WIFI_CONNECTING` | Association or DHCP in progress. |
| `NAC_WIFI_CONNECTED` | IP address acquired, link is up. |
| `NAC_WIFI_SCANNING` | AP scan in progress. |
| `NAC_WIFI_ERROR` | Driver error; reinitialisation required. |

## API

```c
esp_err_t         nac_init(EventGroupHandle_t *event_group);
void              nac_dispose(void);

nac_wifi_status_t nac_get_wifi_status(void);

esp_err_t         nac_request_wifi_connect(const char *ssid, const char *password);
esp_err_t         nac_request_wifi_disconnect(void);

esp_err_t         nac_request_wifi_scan(void);
const wifi_ap_record_t *nac_get_scan_results(uint16_t *out_count);
bool              nac_scan_is_complete(void);
```

### Initialisation

`nac_init()` calls `esp_netif_init()` and `esp_event_loop_create_default()` internally. Pass a pointer to an application-owned `EventGroupHandle_t`; NAC uses it to signal WiFi state changes to other modules.

### Connecting

`nac_request_wifi_connect()` queues a connect task via the task scheduler. SSIDs longer than 32 bytes and passphrases longer than 64 bytes are truncated. Pass `""` as the password for open networks. Credentials are persisted to NVS on successful connection.

### Scanning

```c
nac_request_wifi_scan();          // kick off a scan

// later, poll or check status
if (nac_scan_is_complete())
{
    uint16_t count;
    const wifi_ap_record_t *aps = nac_get_scan_results(&count);
}
```

The AP record buffer is PSRAM-allocated at init (up to `WIFI_SCAN_MAX_RESULT` = 20 entries) and reused on every scan. `nac_scan_is_complete()` is cleared automatically when `nac_request_wifi_scan()` is called again.

## Constants

| Macro | Value | Description |
|---|---|---|
| `WIFI_SSID_MAX_LENGTH` | 32 | Maximum SSID length in bytes (802.11). |
| `WIFI_PASS_MAX_LENGTH` | 64 | Maximum passphrase length in bytes. |
| `WIFI_SCAN_MAX_RESULT` | 20 | Maximum AP records retained after a scan. |
| `WIFI_CRED_MAX_LENGTH` | 64 | Maximum credential string length for NVS. |

## Dependencies

- `task_scheduler` — deferred WiFi work is queued through the scheduler.
- `uart_mole` — NAC passes the application event group to uart_mole for subsystem status signalling.
- ESP-IDF: `esp_wifi`, `esp_netif`, `esp_event`, `nvs_flash`, FreeRTOS event groups.
