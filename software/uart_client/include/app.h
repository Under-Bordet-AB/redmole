/**
 * @file
 * @brief Top-level application entry point for the UART client.
 */

#ifndef APP_H
#define APP_H

#include "uart_client.h"

/**
 * @brief Top-level application context.
 */
typedef struct
{
    uart_client_t client; /*!< UART client instance owned by the application. */
} app_t;

/**
 * @brief Run the application main loop.
 *
 * Blocks until the application exits.
 *
 * @param self Application context; must not be NULL.
 */
void app_run(app_t *self);

#endif /* APP_H */
