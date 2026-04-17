#ifndef SERVER_MODULE_H
#define SERVER_MODULE_H

#include "freertos/FreeRTOS.h"
#include <stdbool.h>

typedef enum {
    SERVER_STATE_IDLE,
    SERVER_STATE_TRANSMITTING,
    SERVER_STATE_BLOCKED // Awaiting WiFi (Sec 3.2)
} server_state;

typedef struct {
    server_state state;
    char endpoint_url[128];
} server_ctx;

void server_init(server_ctx *self);
void server_run(server_ctx *self);
void server_deinit(server_ctx *self);

#endif // SERVER_MODULE_H
