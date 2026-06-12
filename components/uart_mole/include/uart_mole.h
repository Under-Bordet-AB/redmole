/**
 * @file
 * @brief UART software protocol layer (uart_mole).
 *
 * Listens for UART events, assembles typed response packets, and sends them
 * to the client. Each packet contains a tag byte, a data-length field, a
 * payload, and a CRC-16/X25 checksum (esp_crc16_le: polynomial 0x8408
 * reflected, init=0xFFFF, xorout=0xFFFF). Payload bytes that collide with
 * framing values are escaped before transmission.
 *
 * Module order of operations:
 *   1. uart_mole_init() installs the UART driver and spawns the listener task.
 *   2. The listener task waits for UART events from the driver queue.
 *   3. A received command causes a packet to be assembled and posted to the
 *      task scheduler.
 *   4. The scheduler sends the packet over UART.
 *
 * @note The CRC algorithm must match the client exactly. The client must
 *       compute ~protocol_crc16_update(0xFFFF, buf, len) with polynomial
 *       0x8408. Several incompatible CRC-16 variants exist; verify the exact
 *       variant before swapping implementations.
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

/** @brief Selection bitmask for STATUS packet type. */
#define UART_TAG_STATUS             (1 << 0)
/** @brief Selection bitmask for SERVER packet type. */
#define UART_TAG_SERVER             (1 << 1)
/** @brief Selection bitmask for SENSOR packet type. */
#define UART_TAG_SENSOR             (1 << 2)
/** @brief Selection bitmask for RESTART packet type. */
#define UART_TAG_RESTART            (1 << 3)

/** @brief Status byte flag: WiFi is connected. */
#define SET_WIFI_ONLINE             (1 << 0)
/** @brief Status byte flag: LEOP server is reachable. */
#define SET_LEOP_SERVER_ONLINE      (1 << 1)
/** @brief Status byte flag: BME280 sensor is present and responding. */
#define SET_BME280_SENSOR_ONLINE    (1 << 2)
/** @brief Status byte flag: GUI is running. */
#define SET_GUI_ONLINE              (1 << 3)
/** @brief Status byte flag: SD card is mounted. */
#define SET_SDCARD_ONLINE           (1 << 4)

/**
 * @brief Event group bit: WiFi is connected.
 *
 * Set with xEventGroupSetBits(event_group, UART_MOLE_WIFI_CONNECTED_BIT);
 * clear with xEventGroupClearBits().
 */
#define UART_MOLE_WIFI_CONNECTED_BIT    BIT0
/** @brief Event group bit: environmental sensor is online. */
#define UART_MOLE_SENSOR_ONLINE_BIT     BIT1
/** @brief Event group bit: LEOP server is reachable. */
#define UART_MOLE_SERVER_ONLINE_BIT     BIT2
/** @brief Event group bit: GUI is running. */
#define UART_MOLE_GUI_ONLINE_BIT        BIT3
/** @brief Event group bit: SD card is mounted. */
#define UART_MOLE_SDCARD_ONLINE_BIT     BIT4

/** @brief Restart result byte flag: restart was initiated successfully. */
#define SET_RESTART_OK              (1 << 0)
/** @brief Restart result byte flag: restart could not be initiated. */
#define SET_RESTART_FAILED          (1 << 1)

/**
 * @brief Packet type tags used in the tag byte of every UART frame.
 */
typedef enum
{
    UART_STATUS_PKG  = 0, /*!< Status response packet. */
    UART_SERVER_PKG  = 1, /*!< Server data response packet. */
    UART_SENSOR_PKG  = 2, /*!< Sensor readings packet. */
    UART_RESTART_PKG = 3, /*!< Restart acknowledgement packet. */
    UART_DIAG_PKG    = 4, /*!< Diagnostics response packet. */
    UART_TEST_PKG    = 5, /*!< Test/loopback packet. */
} uart_pkg_tag_t;

/**
 * @brief Response packet for a STATUS command.
 */
typedef struct __attribute__ ((packed))
{
    uint8_t  tag_bit;     /*!< Packet type tag; always UART_STATUS_PKG. */
    uint16_t data_len;    /*!< Byte length of the payload following this field. */
    uint8_t  status;      /*!< Bitmask of SET_* flags indicating subsystem online state. */
    uint32_t uptime_s;    /*!< Seconds since the ESP32 last booted. */
    uint32_t timestamp_s; /*!< Unix timestamp in seconds at time of transmission. */
    uint16_t crc16;       /*!< CRC-16/X25 over the payload bytes. */
} uart_status_pkg_t;

/**
 * @brief Response packet for a SERVER command.
 *
 * json_data points into a PSRAM buffer allocated at init and kept for the
 * lifetime of the module. The buffer is reused for each SERVER response.
 */
typedef struct __attribute__ ((packed))
{
    uint8_t  tag_bit;     /*!< Packet type tag; always UART_SERVER_PKG. */
    uint16_t data_len;    /*!< Byte length of the payload following this field. */
    char    *json_data;   /*!< PSRAM-allocated JSON string; owned by uart_ctx_t, not the caller. */
    uint32_t timestamp_s; /*!< Unix timestamp in seconds at time of transmission. */
    uint16_t crc16;       /*!< CRC-16/X25 over the payload bytes. */
} uart_server_pkg_t;

/**
 * @brief Response packet for a SENSOR command.
 *
 * All sensor values are scaled integers (raw value × 100). Divide by 100 to
 * recover the physical quantity.
 */
typedef struct __attribute__ ((packed))
{
    uint8_t  tag_bit;           /*!< Packet type tag; always UART_SENSOR_PKG. */
    uint16_t data_len;          /*!< Byte length of the payload following this field. */
    int32_t  temperature_x_100; /*!< Temperature in degrees Celsius × 100. */
    int32_t  humidity_x_100;    /*!< Relative humidity in percent × 100. */
    int32_t  pressure_x_100;    /*!< Atmospheric pressure in hPa × 100. */
    uint32_t timestamp_s;       /*!< Unix timestamp in seconds at time of transmission. */
    uint16_t crc16;             /*!< CRC-16/X25 over the payload bytes. */
} uart_sensor_pkg_t;

/**
 * @brief Acknowledgement packet for a RESTART command, sent before the chip resets.
 */
typedef struct __attribute__ ((packed))
{
    uint8_t  tag_bit;     /*!< Packet type tag; always UART_RESTART_PKG. */
    uint16_t data_len;    /*!< Byte length of the payload following this field. */
    uint8_t  rslt_bit;    /*!< Result bitmask; SET_RESTART_OK or SET_RESTART_FAILED. */
    uint32_t timestamp_s; /*!< Unix timestamp in seconds at time of transmission. */
    uint16_t crc16;       /*!< CRC-16/X25 over the payload bytes. */
} uart_restart_pkg_t;

/**
 * @brief Response packet for a DIAGNOSTICS command.
 */
typedef struct __attribute__ ((packed))
{
    uint8_t  tag_bit;         /*!< Packet type tag; always UART_DIAG_PKG. */
    uint16_t data_len;        /*!< Byte length of the payload following this field. */
    uint8_t  number_of_tasks; /*!< Number of active FreeRTOS tasks at time of sampling. */
    uint32_t stack_hw;        /*!< High-water mark of the listener task stack in bytes. */
    uint32_t stack_used;      /*!< Bytes of stack currently in use by the listener task. */
    uint32_t timestamp_s;     /*!< Unix timestamp in seconds at time of transmission. */
    uint16_t crc16;           /*!< CRC-16/X25 over the payload bytes. */
} uart_diag_pkg_t;

/**
 * @brief Packet transmission counters for the uart_mole module.
 */
typedef struct uart_mole_stats
{
    uint32_t uart_mole_status_pkg;  /*!< Number of STATUS packets sent. */
    uint32_t uart_mole_server_pkg;  /*!< Number of SERVER packets sent. */
    uint32_t uart_mole_sensor_pkg;  /*!< Number of SENSOR packets sent. */
    uint32_t uart_mole_restart_pkg; /*!< Number of RESTART packets sent. */
} uart_mole_stats_t;

/**
 * @brief Runtime context for the uart_mole module.
 *
 * Allocated statically in the module. task_node must be first so the
 * scheduler can recover the full context with container_of.
 */
typedef struct uart_ctx
{
    task_node_t        task_node;             /*!< Scheduler node; must remain first in the struct. */
    EventGroupHandle_t *event_group;           /*!< Application event group, injected from main; not owned here. */
    QueueHandle_t      queue;                 /*!< UART driver event queue, created by uart_driver_install. */
    TaskHandle_t       rtos_task;             /*!< Handle for the listener FreeRTOS task. */
    uart_mole_stats_t  stats;                 /*!< Running packet counters. */
    const char         *tag;                  /*!< Log tag string, not owned by this struct. */
    uart_port_t        uart_mole_port;        /*!< UART peripheral number (e.g. UART_NUM_1). */
    uint8_t            uart_mole_rx_pin;      /*!< GPIO pin number used for UART RX. */
    uint8_t            uart_mole_tx_pin;      /*!< GPIO pin number used for UART TX. */
    uint16_t           uart_mole_rx_buf_size; /*!< UART driver RX ring-buffer size in bytes. */
    uint16_t           uart_mole_tx_buf_size; /*!< UART driver TX ring-buffer size in bytes. */
    uint32_t           uart_mole_baud_rate;   /*!< UART baud rate in bits per second. */
    uart_pkg_tag_t     pkg_tag;               /*!< Tag of the packet currently being assembled. */
    char               *json_buf;             /*!< PSRAM buffer for JSON server data; owned by this struct. */
    union
    {
        uart_status_pkg_t  status;
        uart_server_pkg_t  server;
        uart_sensor_pkg_t  sensor;
        uart_restart_pkg_t restart;
        uart_diag_pkg_t    diag;
    } pkg_buf; /*!< Staging area for the packet currently being assembled. */
} uart_ctx_t;

/**
 * @brief Initialise the UART driver, configure pins, and spawn the listener task.
 *
 * Installs the UART driver and ISR via uart_driver_install() and allocates a
 * persistent JSON buffer on PSRAM. Must be called before uart_mole_deinit().
 *
 * @param event_group Pointer to the application event group used to signal
 *                    subsystem state; must not be NULL.
 * @return ESP_OK on success, or an esp_err_t error code on failure.
 */
esp_err_t uart_mole_init(EventGroupHandle_t *event_group);

/**
 * @brief Delete the listener task, uninstall the UART driver and ISR, and free the JSON buffer.
 *
 * @return ESP_OK unconditionally.
 */
esp_err_t uart_mole_deinit(void);

#endif // UART_MOLE_H
