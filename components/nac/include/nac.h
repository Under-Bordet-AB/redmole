/* NAC - Network and communications
 * - This module handles wifi and bluetooth
 */

#ifndef NAC_H
#define NAC_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_netif.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define WIFI_SSID_MAX_LENGTH 32
#define WIFI_PASS_MAX_LENGTH 64
#define WIFI_SCAN_MAX_RESULT 20


typedef enum
{
    WIFI_STATE_IDLE = 0,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_RECONNECT,
    WIFI_STATE_START_SCAN,
    WIFI_STATE_SCANNING
} wifi_state_t;

typedef enum
{
    BLUETOOTH_STATE_DISCONNECTED,
    BLUETOOTH_STATE_CONNECTING,
    BLUETOOTH_STATE_CONNECTED,
    BLUETOOTH_STATE_RECONNECTING,
} bluetooth_state_t;

typedef struct wifi_scan_result
{
    char     ssid[WIFI_SCAN_MAX_RESULT][WIFI_SSID_MAX_LENGTH + 1];
    int8_t   irssi[WIFI_SCAN_MAX_RESULT];
    uint16_t count;
} wifi_scan_result_t;

typedef struct wifi_ctx
{
    wifi_state_t state;
    wifi_scan_result_t scan_result;
    esp_netif_t *netif;
    const char *tag;
    uint8_t saved_to_nvs;
    uint8_t hw_online;
} wifi_ctx_t;

typedef struct bluetooth_ctx
{
    bluetooth_state_t state;
    EventGroupHandle_t bluetooth_event_group;
} bluetooth_ctx_t;

typedef struct
{
    wifi_ctx_t wifi;
    bluetooth_ctx_t bluetooth;
} nac_ctx_t;

// Will implement UART diagnostics later
// void wifi_init(wifi_ctx_t *self, QueueHandle_t log_queue);
int8_t wifi_init(wifi_ctx_t *self);
void wifi_dispose(wifi_ctx_t *self);
int8_t wifi_connect(wifi_ctx_t *self, const char *ssid, const char *password);

int8_t bluetooth_init(bluetooth_ctx_t *self);
void bluetooth_dispose(bluetooth_ctx_t *self);

nac_ctx_t *nac_init();
void nac_dispose(nac_ctx_t *self);

#endif /* NAC_H */
