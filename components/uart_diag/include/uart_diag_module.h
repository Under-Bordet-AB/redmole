#ifndef UART_DIAG_MODULE_H
#define UART_DIAG_MODULE_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct {
    QueueHandle_t diag_queue;
    int dropped_messages;
} uart_diag_ctx_t;

void uart_diag_init(uart_diag_ctx_t *self);
void uart_diag_run(uart_diag_ctx_t *self); // Task to pop from queue and print
void uart_diag_deinit(uart_diag_ctx_t *self);

#endif // UART_DIAG_MODULE_H
