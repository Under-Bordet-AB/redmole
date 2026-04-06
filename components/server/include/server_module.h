#ifndef SERVER_MODULE_H
#define SERVER_MODULE_H

#include "freertos/FreeRTOS.h"
#include <stdbool.h>

typedef enum {
    SERVER_STATE_IDLE,
    SERVER_STATE_TRANSMITTING,
    SERVER_STATE_BLOCKED // Awaiting WiFi (Sec 3.2)
} server_state_t;

typedef struct {
    server_state_t state;
    char endpoint_url[128];
} server_ctx_t;

void server_init(server_ctx_t *self);
void server_run(server_ctx_t *self);
void server_deinit(server_ctx_t *self);

#endif // SERVER_MODULE_H
