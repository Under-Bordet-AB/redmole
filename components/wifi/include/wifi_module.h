#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#include "freertos/FreeRTOS.h"
#include <stdbool.h>

typedef enum {
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_RECONNECTING
} wifi_state_t;

typedef struct {
    wifi_state_t state;     // FSM state (Sec 3.1)
    char current_ssid[32];
    bool needs_nvs_save;
} wifi_ctx_t;

void wifi_init(wifi_ctx_t *self, QueueHandle_t log_queue);
void wifi_run(wifi_ctx_t *self);
void wifi_deinit(wifi_ctx_t *self);

#endif // WIFI_MODULE_H
