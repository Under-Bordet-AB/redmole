/* UART Mole - UART software protocol
 * This module listens for UART event and packs the correct package
 * and sends it over UART to the client.
 *
 * A package consists of:
 * (- Start byte) DEPRECATED
 * - Tag byte (identifies the type of package)
 * - Data length
 * - Data bytes
 * - Checksum of the data bytes
 * (- End byte) DEPRECATED
 *
 * When making a package bytes that collide with UART_START and UART_END are escaped.
 * The client decodes the package by removing escape bytes and validating the checksum.
 * The checksum is calculated using esp_crc16_le();
 *
 * Modules order of operations:
 * 1. Initialize UART driver
 * 2. Start UART listener task
 * 3. Wait for UART events
 * 4. Pack data into packages (dynamic allocation on PSRAM) and add task to scheduler
 * 5. Send packages over UART and free allocated memory.
 * 6. Repeat steps 3-5
 *
 * IMPORTANT: esp_crc16_le(0, buf, len) computes CRC-16/X25 (poly 0x1021 reflected = 0x8408, init=0xFFFF, xorout=0xFFFF).
 * Internally it runs ~crc_raw(~init, buf, len), so passing init=0 yields ~crc_raw(0xFFFF, buf, len).
 * The client must mirror this: ~protocol_crc16_update(0xFFFF, buf, len) with polynomial 0x8408.
 * CRC-16 has several incompatible variants (CCITT, X25, Modbus, IBM) that all produce different results on the same data.
 */

#ifndef UART_MOLE_H
#define UART_MOLE_H

#include <stdint.h>
#include <sys/types.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "freertos/idf_additions.h"
#include "task_scheduler.h"

/* TAG bits for UART packets */
#define UART_TAG_STATUS             (1 << 0)
#define UART_TAG_SERVER             (1 << 1)
#define UART_TAG_SENSOR             (1 << 2)
#define UART_TAG_CONFIG             (1 << 3)

/* Status bits for UART status packages */
#define SET_WIFI_ONLINE             (1 << 0)
#define SET_LEOP_SERVER_ONLINE      (1 << 1)
#define SET_BME280_SENSOR_ONLINE    (1 << 2)
#define SET_GUI_ONLINE              (1 << 3)
#define SET_SDCARD_ONLINE           (1 << 4)

/* Event group bits injected from main.
 * NAC sets/clears WIFI, sensor_data sets/clears SENSOR, http_client sets/clears SERVER. */
#define UART_MOLE_WIFI_CONNECTED_BIT    BIT0
#define UART_MOLE_SENSOR_ONLINE_BIT     BIT1
#define UART_MOLE_SERVER_ONLINE_BIT     BIT2
#define UART_MOLE_GUI_ONLINE_BIT        BIT3
#define UART_MOLE_SDCARD_ONLINE_BIT     BIT4


/* Result bits for UART config packages */
#define SET_CONFIG_OK               (1 << 0)
#define SET_CONFIG_FAILED           (1 << 1)

/* UART packet tags */
typedef enum
{
    UART_STATUS_PKG = 0,
    UART_SERVER_PKG = 1,
    UART_SENSOR_PKG = 2,
    UART_CONFIG_PKG = 3,
    UART_DIAG_PKG   = 4,
    UART_TEST_PKG   = 5,
} uart_pkg_tag_t;

/* UART packages */

/* Sent to client when command: STATUS is received */
typedef struct __attribute__ ((packed))
{
    uint8_t  tag_bit;
    uint16_t data_len;
    /* Flags indicating the status of the mole: WIFI, SERVER and SENSOR online. */
    uint8_t  status;
    uint32_t uptime_s;
    uint32_t timestamp_s;
    uint16_t crc16;
} uart_status_pkg_t;

/* Sent to client when command: SERVER is received */
typedef struct __attribute__ ((packed))
{
    uint8_t  tag_bit;
    uint16_t data_len;
    /* Changes: This handle will be declared and allocated on the
     * PSRAM heap on init and kept throughout the lifetime of the mole. */
    char     *json_data;
    uint32_t timestamp_s;
    uint16_t crc16;
} uart_server_pkg_t;

/* Sent to client when command: SENSOR is received */
typedef struct __attribute__ ((packed))
{
    uint8_t  tag_bit;
    uint16_t data_len;
    /* scaled integers (multiplied by 100)
     * Receiver should divide by 100 to get actual value */
    int32_t  temperature_x_100;
    int32_t  humidity_x_100;
    int32_t  pressure_x_100;
    uint32_t timestamp_s;
    uint16_t crc16;
} uart_sensor_pkg_t;

/* Sent to client when command: CONFIG is received */
typedef struct __attribute__ ((packed))
{

    uint8_t  tag_bit;
    uint16_t data_len;
    uint8_t  rslt_bit;
    uint32_t timestamp_s;
    uint16_t crc16;
} uart_config_pkg_t;

/* Sent to client when command: DIAGNOSTICS is received */
typedef struct __attribute__ ((packed))
{
    uint8_t  tag_bit;
    uint16_t data_len;
    uint8_t  number_of_tasks;
    uint32_t stack_hw;
    uint32_t stack_used;
    uint32_t timestamp_s;
    uint16_t crc16;
} uart_diag_pkg_t;

typedef struct uart_mole_stats
{
    uint32_t uart_mole_status_pkg;
    uint32_t uart_mole_server_pkg;
    uint32_t uart_mole_sensor_pkg;
    uint32_t uart_mole_config_pkg;
} uart_mole_stats_t;

typedef struct uart_ctx
{
    task_node_t        task_node;
    /* Injected from global in main,
     * used to check flags of other modules */
    EventGroupHandle_t *event_group;
    QueueHandle_t      queue;
    TaskHandle_t       rtos_task;
    uart_mole_stats_t  stats;
    const char         *tag;
    uart_port_t        uart_mole_port;
    uint8_t            uart_mole_rx_pin;
    uint8_t            uart_mole_tx_pin;
    uint16_t           uart_mole_rx_buf_size;
    uint16_t           uart_mole_tx_buf_size;
    uint32_t           uart_mole_baud_rate;

    uart_pkg_tag_t     pkg_tag;
    char               *json_buf;
    union {
        uart_status_pkg_t  status;
        uart_server_pkg_t  server;
        uart_sensor_pkg_t  sensor;
        uart_config_pkg_t  config;
        uart_diag_pkg_t    diag;
    } pkg_buf;
} uart_ctx_t;


/** @brief Initialises the UART driver, configures pins, spawns the listener task,
 *         and starts the underlying UART driver task and ISR via uart_driver_install.
 *  @note  Allocates a persistent JSON buffer on PSRAM; all other package storage uses pkg_buf.
 *  @param event_group Pointer to the application event group, injected from main.
 *  @return ESP_OK on success, or an esp_err_t error code. */
esp_err_t uart_mole_init(EventGroupHandle_t *event_group);

/** @brief Deletes the listener task, uninstalls the UART driver and ISR, and frees the JSON buffer.
 *  @return ESP_OK unconditionally. */
esp_err_t uart_mole_deinit(void);

#endif // UART_MOLE_H
