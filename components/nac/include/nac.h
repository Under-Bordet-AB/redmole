/* NAC - Network and Communications
 * Handles WiFi and Bluetooth
 */

#ifndef NAC_H
#define NAC_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "task_scheduler.h"

#define WIFI_SSID_MAX_LENGTH  32
#define WIFI_PASS_MAX_LENGTH  64
#define WIFI_SCAN_MAX_RESULT  20

/*  WiFi */

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

typedef struct wifi_ctx
{
    task_node_t       task_node;
    wifi_state_t      state;
    wifi_ap_record_t *ap_records;   /* heap_caps_malloc(PSRAM), WIFI_SCAN_MAX_RESULT entries */
    uint16_t          ap_count;
    nvs_handle_t      nvs_handle;   /* opened once in wifi_init(), read in wifi_connect()    */
    esp_netif_t      *netif;
    const char       *tag;
    uint8_t           retry_count;
    uint8_t           saved_to_nvs;
    uint8_t           hw_online;
    uint8_t           scan_complete;
} wifi_ctx_t;

/*  Bluetooth */

typedef enum
{
    BLUETOOTH_STATE_IDLE = 0,
    BLUETOOTH_STATE_DISCONNECTED,
    BLUETOOTH_STATE_CONNECTING,
    BLUETOOTH_STATE_CONNECTED,
    BLUETOOTH_STATE_RECONNECTING,
} bluetooth_state_t;

typedef struct bluetooth_ctx
{
    bluetooth_state_t  state;
    EventGroupHandle_t bluetooth_event_group;
} bluetooth_ctx_t;

/*  NAC composite */

typedef enum
{
    NAC_WIFI_DISCONNECTED = 0,
    NAC_WIFI_CONNECTING,
    NAC_WIFI_CONNECTED,
    NAC_WIFI_SCANNING,
    NAC_WIFI_ERROR,
} nac_wifi_status_t;

typedef struct
{
    wifi_ctx_t      wifi;
    bluetooth_ctx_t bluetooth;
} nac_ctx_t;

int8_t wifi_init(wifi_ctx_t *self);
void   wifi_dispose(wifi_ctx_t *self);
task_status_t wifi_connect(task_node_t *node);

int8_t bluetooth_init(bluetooth_ctx_t *self);
void   bluetooth_dispose(bluetooth_ctx_t *self);

nac_ctx_t *nac_init(void);
void       nac_dispose(nac_ctx_t *self);
nac_wifi_status_t nac_get_wifi_status(const nac_ctx_t *self);
int8_t nac_request_wifi_connect(nac_ctx_t *self);
int8_t nac_request_wifi_disconnect(nac_ctx_t *self);
int8_t nac_request_wifi_scan(nac_ctx_t *self);

const wifi_ap_record_t *nac_get_scan_results(const nac_ctx_t *self, uint16_t *out_count);

#endif /* NAC_H */
