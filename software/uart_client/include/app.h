#ifndef APP_H
#define APP_H

#include "uart_client.h"

typedef struct
{
    uart_client_t client;
} app_t;

void app_run(app_t *self);

#endif /* APP_H */
