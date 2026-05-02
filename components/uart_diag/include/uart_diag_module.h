#ifndef UART_DIAG_MODULE_H
#define UART_DIAG_MODULE_H

#include <stdint.h>
#include <sys/cdefs.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "task_scheduler.h"

/* Start and stop bytes for UART packets */
#define UART_START 0xF0
#define UART_END 0xF1
/* TAG bits for UART packets */
#define UART_TAG_STATUS             (1 << 0)
#define UART_TAG_SERVER             (1 << 1)
#define UART_TAG_SENSOR             (1 << 2)
#define UART_TAG_CONFIG             (1 << 3)

/* Status bits for UART status packages */
#define SET_WIFI_ONLINE             (1 << 0)
#define SET_LEOP_SERVER_ONLINE      (1 << 1)
#define SET_BME280_SENSOR_ONLINE    (1 << 2)

/* Max buffer size for UART server packages */
#define UART_SERVER_PKG_MAX_SIZE 256 // Calculate exact size needed

/* Result bits for UART config packages */
#define SET_CONFIG_OK               (1 << 0)


/* UART packages */

typedef struct __attribute__ ((packed))
{
    uint8_t  start_of_packet;
    uint8_t  tag_bit;
    uint8_t  status;
    uint32_t uptime_ms;
    uint32_t timestamp_ms;
    uint8_t  end_of_packet;
} uart_status_pkg_t;

typedef struct __attribute__ ((packed))
{
    uint8_t  start_of_paket;
    uint8_t  tag_bit;
    char     json_data[UART_SERVER_PKG_MAX_SIZE];
    uint32_t timestamp_ms;
    uint8_t  end_of_packet;
} uart_server_pkg_t;

typedef struct __attribute__ ((packed))
{
    uint8_t  start_of_packet;
    uint8_t  tag_bit;
    double   temperature;
    double   humidity;
    double   pressure;
    uint32_t timestamp_ms;
    uint8_t  end_of_packet;
} uart_sensor_pkg_t;

typedef struct __attribute__ ((packed))
{
    uint8_t  start_of_packet;
    uint8_t  tag_bit;
    uint8_t  rslt_bit;
    uint32_t timestamp_ms;
    uint8_t  end_of_packet;
} uart_config_pkg_t;

typedef struct __attribute__ ((packed))
{
    uint8_t  start_of_packet;
    uint8_t  tag_bit;
    uint8_t  number_of_tasks;
    uint32_t stack_hw;
    uint32_t stack_used;
    uint32_t timestamp_ms;
    uint8_t  end_of_packet;
} uart_diag_pkg_t;

typedef struct uart_ctx
{
    uart_status_pkg_t status_pkg;
    uart_server_pkg_t server_pkg;
    uart_sensor_pkg_t sensor_pkg;
    uart_config_pkg_t config_pkg;
    uart_diag_pkg_t   diag_pkg;
    task_node_t       task_node;
} uart_ctx_t;


void uart_diag_init(uart_ctx_t *self);
uint8_t uart_diag_run(uart_ctx_t *self);
void uart_diag_deinit(uart_ctx_t *self);

#endif // UART_DIAG_MODULE_H
