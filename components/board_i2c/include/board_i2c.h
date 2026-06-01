#ifndef BOARD_I2C_H
#define BOARD_I2C_H

/**
 * @file
 * @brief Shared board-level I2C bus API.
 *
 * This component owns the Waveshare board I2C bus used by the IO expander,
 * GT911 touch controller, and external I2C header. Sensor-specific register
 * protocols belong in their own drivers.
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"

/** GPIO used as SDA for the board-level I2C bus. */
#define BOARD_I2C_MASTER_SDA GPIO_NUM_8
/** GPIO used as SCL for the board-level I2C bus. */
#define BOARD_I2C_MASTER_SCL GPIO_NUM_9
/** ESP-IDF I2C controller used for the shared board bus. */
#define BOARD_I2C_MASTER_PORT I2C_NUM_0
/** Default bus speed for board devices when callers do not need a custom speed. */
#define BOARD_I2C_DEFAULT_SPEED_HZ (400U * 1000U)
/** Timeout used by board-level I2C transactions and probes. */
#define BOARD_I2C_TIMEOUT_MS 100

/**
 * @brief Known devices expected on the Waveshare board I2C bus.
 *
 * These identifiers are used for diagnostics and human-readable scan output.
 * The enum does not reserve or register devices by itself.
 */
typedef enum {
    BOARD_I2C_KNOWN_DEVICE_IO_EXTENSION,
    BOARD_I2C_KNOWN_DEVICE_GT911_TOUCH,
    BOARD_I2C_KNOWN_DEVICE_GT911_TOUCH_ALT,
    BOARD_I2C_KNOWN_DEVICE_BME280,
    BOARD_I2C_KNOWN_DEVICE_BME280_ALT,
} board_i2c_known_device;

/**
 * @brief Initialize the shared board I2C bus.
 *
 * The bus is single-instance and idempotent. It is used by the board IO
 * expander, GT911 touch controller, and external I2C header.
 *
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t board_i2c_init(void);

/**
 * @brief Return the shared board I2C bus handle.
 *
 * Initializes the bus if needed. This is intended for ESP-IDF APIs that need
 * the raw bus handle, such as LCD/touch panel IO creation.
 *
 * @return Bus handle on success, NULL when initialization fails.
 */
i2c_master_bus_handle_t board_i2c_get_bus(void);

/**
 * @brief Scan the shared board I2C bus and log devices that ACK.
 *
 * This is a diagnostic helper for bring-up and troubleshooting. Normal runtime
 * code should prefer targeted probes or owned device handles.
 *
 * @param out_found_count Optional output for number of ACKing addresses found.
 * @return ESP_OK when the scan ran, otherwise an ESP-IDF error code.
 */
esp_err_t board_i2c_scan(uint8_t* out_found_count);

/**
 * @brief Probe one 7-bit I2C address on the shared board bus.
 *
 * @param address 7-bit I2C address to probe.
 * @return True when a device ACKs the address.
 */
bool board_i2c_probe_address(uint8_t address);

/**
 * @brief Check whether a BME280 responds at one of its expected addresses.
 *
 * This only proves I2C presence at 0x76 or 0x77. The BME280 backend still
 * verifies chip ID before treating the device as a real BME280.
 *
 * @return True when 0x76 or 0x77 ACKs.
 */
bool board_i2c_bme280_present(void);

/**
 * @brief Add an I2C device handle on the shared board bus.
 *
 * Ownership of the returned handle belongs to the caller. The current helper
 * creates a new handle, so callers must avoid registering the same address
 * repeatedly. The intended long-term direction is for board_i2c to own a small
 * address registry and return an existing handle for duplicate requests.
 *
 * @param address 7-bit I2C address.
 * @param speed_hz Device bus speed, or 0 to use BOARD_I2C_DEFAULT_SPEED_HZ.
 * @param out Output device handle.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t board_i2c_add_device(uint16_t address, uint32_t speed_hz, i2c_master_dev_handle_t* out);

/**
 * @brief Read bytes from an 8-bit register address.
 *
 * @param dev Device handle returned by board_i2c_add_device().
 * @param reg Register address to read from.
 * @param data Output buffer.
 * @param len Number of bytes to read.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t board_i2c_read_reg(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t* data, size_t len);

/**
 * @brief Write one byte to an 8-bit register address.
 *
 * @param dev Device handle returned by board_i2c_add_device().
 * @param reg Register address to write to.
 * @param value Byte value to write.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t board_i2c_write_reg(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t value);

/**
 * @brief Read a little-endian 16-bit value from an 8-bit register address.
 *
 * This helper exists for board devices that expose 16-bit little-endian
 * register values.
 *
 * @param dev Device handle returned by board_i2c_add_device().
 * @param reg Register address to read from.
 * @param out Output value.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t board_i2c_read_reg_u16_le(i2c_master_dev_handle_t dev, uint8_t reg, uint16_t* out);

/**
 * @brief Write a raw byte buffer to an I2C device.
 *
 * @param dev Device handle returned by board_i2c_add_device().
 * @param data Buffer to transmit.
 * @param len Number of bytes to transmit.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t board_i2c_write(i2c_master_dev_handle_t dev, const uint8_t* data, size_t len);

/**
 * @brief Read raw bytes from an I2C device.
 *
 * @param dev Device handle returned by board_i2c_add_device().
 * @param data Output buffer.
 * @param len Number of bytes to receive.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t board_i2c_read(i2c_master_dev_handle_t dev, uint8_t* data, size_t len);

/**
 * @brief Delete the shared board I2C bus.
 *
 * Call only during teardown when no device handles are still in use.
 */
void board_i2c_deinit(void);

#endif // BOARD_I2C_H
