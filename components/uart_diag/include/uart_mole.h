/* UART Mole - UART software protocol
 * This module listens for UART event and packs the correct package
 * and sends it over UART to the client.
 *
 * A package consists of:
 * - Start byte
 * - Tag byte (identifies the type of package)
 * - Data length
 * - Data bytes
 * - Checksum
 * - End byte
 *
 * When making a package bytes that collide with UART_START and UART_END are escaped.
 * The client decodes the package by removing escape bytes and validating the checksum.
 * The checksum is calculated using esp_crc16_le();
 *
 * Modules order of operations:
 * 1. Initialize UART driver
 * 2. Start UART listener task
 * 3. Wait for UART events
 * 4. Pack data into packages (dynamic allocation on PSRAM, heap_4 needed) and add task to scheduler
 * 5. Send packages over UART and free allocated memory.
 * 6. Repeat steps 3-5
 */

#ifndef UART_MOLE_H
#define UART_MOLE_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/uart.h"
#include "task_scheduler.h"

/* Start, stop and escape bytes for UART packets */
#define UART_START                  0xF0
#define UART_END                    0xF1
#define UART_ESCAPE                 0xF2
#define UART_XOR_MASK               0x20

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
#define SET_CONFIG_FAILED           (1 << 1)

/*
 * UART packet tags
 */
typedef enum
{
    UART_STATUS_PKG = 0,
    UART_SERVER_PKG = 1,
    UART_SENSOR_PKG = 2,
    UART_CONFIG_PKG = 3,
} uart_pkg_tag_t;

/* UART packages */

/* Sent to client when command: STATUS is received */
typedef struct __attribute__ ((packed))
{
    uint8_t  start_of_packet;
    uint8_t  tag_bit;
    uint16_t data_len;
    uint8_t  status;
    uint32_t uptime_ms;
    uint32_t timestamp_ms;
    uint16_t crc16;
    uint8_t  end_of_packet;
} uart_status_pkg_t;

/* Sent to client when command: SERVER is received */
typedef struct __attribute__ ((packed))
{
    uint8_t  start_of_packet;
    uint8_t  tag_bit;
    uint16_t data_len;
    char     json_data[UART_SERVER_PKG_MAX_SIZE];
    uint32_t timestamp_ms;
    uint16_t crc16;
    uint8_t  end_of_packet;
} uart_server_pkg_t;

/* Sent to client when command: SENSOR is received */
typedef struct __attribute__ ((packed))
{
    uint8_t  start_of_packet;
    uint8_t  tag_bit;
    uint16_t data_len;
    /* scaled integers (multiplied by 100)
     * Receiver should divide by 100 to get actual value */
    int32_t  temperature_x_100;
    int32_t  humidity_x_100;
    int32_t  pressure_x_100;
    uint32_t timestamp_ms;
    uint16_t crc16;
    uint8_t  end_of_packet;
} uart_sensor_pkg_t;

/* Sent to client when command: CONFIG is received */
typedef struct __attribute__ ((packed))
{
    uint8_t  start_of_packet;
    uint8_t  tag_bit;
    uint16_t data_len;
    uint8_t  rslt_bit;
    uint32_t timestamp_ms;
    uint16_t crc16;
    uint8_t  end_of_packet;
} uart_config_pkg_t;

/* Sent to client when command: DIAGNOSTICS is received */
typedef struct __attribute__ ((packed))
{
    uint8_t  start_of_packet;
    uint8_t  tag_bit;
    uint16_t data_len;
    uint8_t  number_of_tasks;
    uint32_t stack_hw;
    uint32_t stack_used;
    uint32_t timestamp_ms;
    uint16_t crc16;
    uint8_t  end_of_packet;
} uart_diag_pkg_t;

typedef struct uart_mole_persist
{
    uint32_t      time_since_boot;
    uart_config_t uart_config;
} uart_mole_persist_t;

typedef struct uart_ctx
{
    task_node_t           task_node;
    uart_pkg_tag_t        pkg_tag;
    uart_config_pkg_t     persistant_vars;
    /* Tagged union for UART mole context packet types */
    union
    {
        uart_status_pkg_t status_pkg;
        uart_server_pkg_t server_pkg;
        uart_sensor_pkg_t sensor_pkg;
        uart_config_pkg_t config_pkg;
        uart_diag_pkg_t   diag_pkg;
    };
} uart_ctx_t;


/* @brief Initialize the UART mole context and ESP-IDF UART driver.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t uart_mole_init(void);

/* @brief  Deinitializes the UART mole context and ESP-IDF UART driver.
 * @param  self Pointer to the UART mole context.
 */
void uart_diag_deinit(uart_ctx_t *self);

#endif // UART_MOLE_H
