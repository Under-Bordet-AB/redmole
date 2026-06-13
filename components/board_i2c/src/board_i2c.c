#include "board_i2c.h"

/**
 * @file
 * @brief Implementation of the shared board-level I2C bus.
 */

#include <stdbool.h>
#include <sys/lock.h>

#include "esp_log.h"

static const char* TAG = "BOARD_I2C";

typedef struct {
    board_i2c_known_device device; /*!< Stable identifier used by diagnostics. */
    uint8_t address;               /*!< Seven-bit address expected on the board bus. */
    const char* name;              /*!< Human-readable scan-log description. */
} board_i2c_known_device_desc;

static const board_i2c_known_device_desc s_known_devices[] = {
    {BOARD_I2C_KNOWN_DEVICE_IO_EXTENSION, 0x24, "IO expander"},
    {BOARD_I2C_KNOWN_DEVICE_GT911_TOUCH, 0x5d, "GT911 touch"},
    {BOARD_I2C_KNOWN_DEVICE_GT911_TOUCH_ALT, 0x14, "GT911 touch alternate address"},
    {BOARD_I2C_KNOWN_DEVICE_BME280, 0x76, "BME280 environmental sensor"},
    {BOARD_I2C_KNOWN_DEVICE_BME280_ALT, 0x77, "BME280 environmental sensor alternate address"},
};

static i2c_master_bus_handle_t s_bus; /*!< Shared bus retained until controlled teardown. */
static bool s_initialized;            /*!< True after the shared bus is created successfully. */
static _lock_t s_lifecycle_lock;      /*!< Serializes bus lifecycle and wrapper operations. */

/**
 * @brief Initialize the shared bus while the lifecycle lock is held.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
static esp_err_t board_i2c_init_locked(void) {
    if (s_initialized) {
        return ESP_OK;
    }

    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = BOARD_I2C_MASTER_PORT,
        .scl_io_num = BOARD_I2C_MASTER_SCL,
        .sda_io_num = BOARD_I2C_MASTER_SDA,
        .glitch_ignore_cnt = 7,
    };

    esp_err_t rv = i2c_new_master_bus(&bus_config, &s_bus);
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(rv));
        return rv;
    }

    s_initialized = true;
    return ESP_OK;
}

/**
 * @brief Return the diagnostic name associated with one seven-bit address.
 * @param address Address observed during a bus scan.
 * @return Known-device name or a stable unknown-device description.
 */
static const char* board_i2c_known_device_name(uint8_t address) {
    for (size_t index = 0; index < (sizeof(s_known_devices) / sizeof(s_known_devices[0]));
         index++) {
        if (s_known_devices[index].address == address) {
            return s_known_devices[index].name;
        }
    }

    return "unknown device";
}

esp_err_t board_i2c_init(void) {
    _lock_acquire(&s_lifecycle_lock);
    esp_err_t rv = board_i2c_init_locked();
    _lock_release(&s_lifecycle_lock);
    return rv;
}

i2c_master_bus_handle_t board_i2c_get_bus(void) {
    _lock_acquire(&s_lifecycle_lock);
    i2c_master_bus_handle_t bus = board_i2c_init_locked() == ESP_OK ? s_bus : NULL;
    _lock_release(&s_lifecycle_lock);
    return bus;
}

esp_err_t board_i2c_scan(uint8_t* out_found_count) {
    uint8_t found_count = 0;

    _lock_acquire(&s_lifecycle_lock);
    esp_err_t rv = board_i2c_init_locked();
    if (rv != ESP_OK) {
        _lock_release(&s_lifecycle_lock);
        return rv;
    }

    ESP_LOGI(TAG, "Scanning I2C bus on SDA=%d SCL=%d", BOARD_I2C_MASTER_SDA, BOARD_I2C_MASTER_SCL);

    for (uint16_t address = 0x03; address <= 0x77; address++) {
        rv = i2c_master_probe(s_bus, address, BOARD_I2C_TIMEOUT_MS);
        if (rv == ESP_OK) {
            ESP_LOGI(TAG, "I2C device found at 0x%02x (%s)", address,
                     board_i2c_known_device_name((uint8_t)address));
            found_count++;
        }
    }

    ESP_LOGI(TAG, "I2C scan complete: %u device(s) found", found_count);

    if (out_found_count != NULL) {
        *out_found_count = found_count;
    }

    _lock_release(&s_lifecycle_lock);
    return ESP_OK;
}

bool board_i2c_probe_address(uint8_t address) {
    _lock_acquire(&s_lifecycle_lock);
    if (board_i2c_init_locked() != ESP_OK) {
        _lock_release(&s_lifecycle_lock);
        return false;
    }

    bool present = i2c_master_probe(s_bus, address, BOARD_I2C_TIMEOUT_MS) == ESP_OK;
    _lock_release(&s_lifecycle_lock);
    return present;
}

bool board_i2c_bme280_present(void) {
    return board_i2c_probe_address(0x76) || board_i2c_probe_address(0x77);
}

esp_err_t board_i2c_add_device(uint16_t address, uint32_t speed_hz, i2c_master_dev_handle_t* out) {
    if ((out == NULL) || (address > 0x7FU)) {
        return ESP_ERR_INVALID_ARG;
    }

    *out = NULL;

    _lock_acquire(&s_lifecycle_lock);
    esp_err_t rv = board_i2c_init_locked();
    if (rv != ESP_OK) {
        _lock_release(&s_lifecycle_lock);
        return rv;
    }

    i2c_device_config_t dev_config = {
        .device_address = address,
        .scl_speed_hz = (speed_hz == 0U) ? BOARD_I2C_DEFAULT_SPEED_HZ : speed_hz,
    };

    rv = i2c_master_bus_add_device(s_bus, &dev_config, out);
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "i2c_master_bus_add_device(0x%02x) failed: %s", address, esp_err_to_name(rv));
    }

    _lock_release(&s_lifecycle_lock);
    return rv;
}

esp_err_t board_i2c_read_reg(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t* data, size_t len) {
    if ((dev == NULL) || (data == NULL) || (len == 0U)) {
        return ESP_ERR_INVALID_ARG;
    }

    _lock_acquire(&s_lifecycle_lock);
    esp_err_t rv = i2c_master_transmit_receive(dev, &reg, 1, data, len, BOARD_I2C_TIMEOUT_MS);
    _lock_release(&s_lifecycle_lock);
    return rv;
}

esp_err_t board_i2c_write_reg(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};

    return board_i2c_write(dev, data, sizeof(data));
}

esp_err_t board_i2c_read_reg_u16_le(i2c_master_dev_handle_t dev, uint8_t reg, uint16_t* out) {
    uint8_t data[2] = {0};

    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t rv = board_i2c_read_reg(dev, reg, data, sizeof(data));
    if (rv != ESP_OK) {
        return rv;
    }

    *out = ((uint16_t)data[1] << 8) | data[0];
    return ESP_OK;
}

esp_err_t board_i2c_write(i2c_master_dev_handle_t dev, const uint8_t* data, size_t len) {
    if ((dev == NULL) || (data == NULL) || (len == 0U)) {
        return ESP_ERR_INVALID_ARG;
    }

    _lock_acquire(&s_lifecycle_lock);
    esp_err_t rv = i2c_master_transmit(dev, data, len, BOARD_I2C_TIMEOUT_MS);
    _lock_release(&s_lifecycle_lock);
    return rv;
}

esp_err_t board_i2c_read(i2c_master_dev_handle_t dev, uint8_t* data, size_t len) {
    if ((dev == NULL) || (data == NULL) || (len == 0U)) {
        return ESP_ERR_INVALID_ARG;
    }

    _lock_acquire(&s_lifecycle_lock);
    esp_err_t rv = i2c_master_receive(dev, data, len, BOARD_I2C_TIMEOUT_MS);
    _lock_release(&s_lifecycle_lock);
    return rv;
}

void board_i2c_deinit(void) {
    _lock_acquire(&s_lifecycle_lock);
    if (!s_initialized) {
        _lock_release(&s_lifecycle_lock);
        return;
    }

    if (i2c_del_master_bus(s_bus) == ESP_OK) {
        s_bus = NULL;
        s_initialized = false;
    } else {
        ESP_LOGW(TAG, "i2c_del_master_bus failed");
    }
    _lock_release(&s_lifecycle_lock);
}
