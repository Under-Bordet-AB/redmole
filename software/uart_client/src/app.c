#include "app.h"

void app_run(app_t *self)
{
    if (uart_client_init(&self->client) < 0)
        return;

    uart_client_work(&self->client);
    uart_client_deinit(&self->client);
}
