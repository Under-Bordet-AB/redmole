#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

typedef enum {
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_RECONNECTING
} wifi_state_t;

typedef struct {
    wifi_state_t state;
    EventGroupHandle_t wifi_event_group;
    int retry_count;
    const char *tag;
    bool needs_nvs_save;

} wifi_ctx_t;

// Will implement UART diagnostics later
// void wifi_init(wifi_ctx_t *self, QueueHandle_t log_queue);
void wifi_init(wifi_ctx_t *self);
void wifi_task(void *pvParameters);
void wifi_dispose(wifi_ctx_t *self);

#endif // WIFI_MODULE_H
