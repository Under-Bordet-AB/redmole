#include "hal_internal/bme280_hal_backend.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "board_i2c.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BME280_ADDR_PRIMARY 0x76
#define BME280_ADDR_ALTERNATE 0x77
#define BME280_REG_CHIP_ID 0xD0
#define BME280_REG_RESET 0xE0
#define BME280_REG_CTRL_HUM 0xF2
#define BME280_REG_STATUS 0xF3
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_CONFIG 0xF5
#define BME280_REG_CALIB_00 0x88
#define BME280_REG_CALIB_26 0xE1
#define BME280_REG_DATA 0xF7
#define BME280_CHIP_ID 0x60
#define BME280_RESET_CMD 0xB6
#define BME280_STATUS_BUSY_MASK 0x09U
#define BME280_READY_WAIT_ATTEMPTS 10U
#define BME280_READY_WAIT_DELAY_MS 10U
#define BME280_RESET_DELAY_MS 5U
#define BME280_OVERSAMPLING_X1 0x01U
#define BME280_STANDBY_1000_MS_FILTER_OFF 0xA0U
#define BME280_NORMAL_MODE_TEMP_PRESS_X1 0x27U
#define BME280_US_PER_MS 1000LL

typedef struct {
    int32_t adc_temperature;
    int32_t adc_pressure;
    int32_t adc_humidity;
} bme280_raw_sample;

typedef struct {
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;
} bme280_calibration;

typedef struct {
    i2c_master_dev_handle_t dev;
    uint8_t address;
    bme280_calibration calibration;
    bool calibration_loaded;
} bme280_hw_backend_state;

static bme280_hw_backend_state* bme280_hw_get_state(bme280_hal* self) {
    if (self == NULL) {
        return NULL;
    }

    return (bme280_hw_backend_state*)self->backend_state;
}

static uint16_t bme280_u16_le(const uint8_t* data) {
    return ((uint16_t)data[1] << 8) | data[0];
}

static int16_t bme280_s16_le(const uint8_t* data) {
    return (int16_t)bme280_u16_le(data);
}

static int16_t bme280_s12(int16_t value) {
    if ((value & 0x0800) != 0) {
        value |= (int16_t)0xF000;
    }

    return value;
}

static esp_err_t bme280_hw_choose_address(uint8_t* out_address) {
    if (out_address == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (board_i2c_probe_address(BME280_ADDR_PRIMARY)) {
        *out_address = BME280_ADDR_PRIMARY;
        return ESP_OK;
    }

    if (board_i2c_probe_address(BME280_ADDR_ALTERNATE)) {
        *out_address = BME280_ADDR_ALTERNATE;
        return ESP_OK;
    }

    return ESP_ERR_NOT_FOUND;
}

static esp_err_t bme280_hw_read_u8(bme280_hw_backend_state* state, uint8_t reg, uint8_t* out) {
    return board_i2c_read_reg(state->dev, reg, out, 1);
}

static esp_err_t bme280_hw_wait_until_ready(bme280_hw_backend_state* state) {
    for (uint8_t attempt = 0; attempt < BME280_READY_WAIT_ATTEMPTS; attempt++) {
        uint8_t status = 0;
        esp_err_t rv = bme280_hw_read_u8(state, BME280_REG_STATUS, &status);
        if (rv != ESP_OK) {
            return rv;
        }

        if ((status & BME280_STATUS_BUSY_MASK) == 0U) {
            return ESP_OK;
        }

        vTaskDelay(pdMS_TO_TICKS(BME280_READY_WAIT_DELAY_MS));
    }

    return ESP_ERR_TIMEOUT;
}

static esp_err_t bme280_hw_load_calibration(bme280_hw_backend_state* state) {
    uint8_t calib0[26] = {0};
    uint8_t calib1[7] = {0};
    esp_err_t rv = board_i2c_read_reg(state->dev, BME280_REG_CALIB_00, calib0, sizeof(calib0));
    if (rv != ESP_OK) {
        return rv;
    }

    rv = board_i2c_read_reg(state->dev, BME280_REG_CALIB_26, calib1, sizeof(calib1));
    if (rv != ESP_OK) {
        return rv;
    }

    state->calibration.dig_T1 = bme280_u16_le(&calib0[0]);
    state->calibration.dig_T2 = bme280_s16_le(&calib0[2]);
    state->calibration.dig_T3 = bme280_s16_le(&calib0[4]);
    state->calibration.dig_P1 = bme280_u16_le(&calib0[6]);
    state->calibration.dig_P2 = bme280_s16_le(&calib0[8]);
    state->calibration.dig_P3 = bme280_s16_le(&calib0[10]);
    state->calibration.dig_P4 = bme280_s16_le(&calib0[12]);
    state->calibration.dig_P5 = bme280_s16_le(&calib0[14]);
    state->calibration.dig_P6 = bme280_s16_le(&calib0[16]);
    state->calibration.dig_P7 = bme280_s16_le(&calib0[18]);
    state->calibration.dig_P8 = bme280_s16_le(&calib0[20]);
    state->calibration.dig_P9 = bme280_s16_le(&calib0[22]);
    state->calibration.dig_H1 = calib0[25];
    state->calibration.dig_H2 = bme280_s16_le(&calib1[0]);
    state->calibration.dig_H3 = calib1[2];
    state->calibration.dig_H4 =
        bme280_s12((int16_t)(((int16_t)calib1[3] << 4) | (calib1[4] & 0x0F)));
    state->calibration.dig_H5 = bme280_s12((int16_t)(((int16_t)calib1[5] << 4) | (calib1[4] >> 4)));
    state->calibration.dig_H6 = (int8_t)calib1[6];
    state->calibration_loaded = true;

    return ESP_OK;
}

static esp_err_t bme280_hw_configure(bme280_hw_backend_state* state) {
    esp_err_t rv = board_i2c_write_reg(state->dev, BME280_REG_CTRL_HUM, BME280_OVERSAMPLING_X1);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = board_i2c_write_reg(state->dev, BME280_REG_CONFIG, BME280_STANDBY_1000_MS_FILTER_OFF);
    if (rv != ESP_OK) {
        return rv;
    }

    return board_i2c_write_reg(state->dev, BME280_REG_CTRL_MEAS, BME280_NORMAL_MODE_TEMP_PRESS_X1);
}

static esp_err_t bme280_hw_connect(bme280_hal* self) {
    bme280_hw_backend_state* state = bme280_hw_get_state(self);
    uint8_t address = 0;
    uint8_t chip_id = 0;

    if (state == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t rv = bme280_hw_choose_address(&address);
    if (rv != ESP_OK) {
        self->hardware_present = false;
        state->calibration_loaded = false;
        return rv;
    }

    if ((state->dev == NULL) || (state->address != address)) {
        rv = board_i2c_add_device(address, BOARD_I2C_DEFAULT_SPEED_HZ, &state->dev);
        if (rv != ESP_OK) {
            self->hardware_present = false;
            state->calibration_loaded = false;
            return rv;
        }
        state->address = address;
        state->calibration_loaded = false;
    }

    rv = bme280_hw_read_u8(state, BME280_REG_CHIP_ID, &chip_id);
    if (rv != ESP_OK) {
        self->hardware_present = false;
        state->calibration_loaded = false;
        return rv;
    }

    if (chip_id != BME280_CHIP_ID) {
        ESP_LOGW(self->tag, "Unexpected chip id at 0x%02x: 0x%02x", address, chip_id);
        self->hardware_present = false;
        state->calibration_loaded = false;
        return ESP_ERR_NOT_FOUND;
    }

    if (!state->calibration_loaded) {
        rv = board_i2c_write_reg(state->dev, BME280_REG_RESET, BME280_RESET_CMD);
        if (rv != ESP_OK) {
            self->hardware_present = false;
            return rv;
        }

        vTaskDelay(pdMS_TO_TICKS(BME280_RESET_DELAY_MS));

        rv = bme280_hw_wait_until_ready(state);
        if (rv != ESP_OK) {
            self->hardware_present = false;
            return rv;
        }

        rv = bme280_hw_load_calibration(state);
        if (rv != ESP_OK) {
            self->hardware_present = false;
            return rv;
        }

        rv = bme280_hw_configure(state);
        if (rv != ESP_OK) {
            self->hardware_present = false;
            return rv;
        }

        ESP_LOGI(self->tag, "BME280 hardware initialized at 0x%02x", address);
    }

    self->hardware_present = true;
    return ESP_OK;
}

static esp_err_t bme280_hw_read_raw(bme280_hw_backend_state* state, bme280_raw_sample* out_raw) {
    uint8_t data[8] = {0};

    if (out_raw == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t rv = bme280_hw_wait_until_ready(state);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = board_i2c_read_reg(state->dev, BME280_REG_DATA, data, sizeof(data));
    if (rv != ESP_OK) {
        return rv;
    }

    out_raw->adc_pressure = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | (data[2] >> 4);
    out_raw->adc_temperature = ((int32_t)data[3] << 12) | ((int32_t)data[4] << 4) | (data[5] >> 4);
    out_raw->adc_humidity = ((int32_t)data[6] << 8) | data[7];

    return ESP_OK;
}

static void bme280_hw_convert(const bme280_hw_backend_state* state, const bme280_raw_sample* raw,
                              bme280_measurement* out_measurement) {
    double var1 =
        (((double)raw->adc_temperature) / 16384.0 - ((double)state->calibration.dig_T1) / 1024.0) *
        ((double)state->calibration.dig_T2);
    double var2 = ((((double)raw->adc_temperature) / 131072.0 -
                    ((double)state->calibration.dig_T1) / 8192.0) *
                   (((double)raw->adc_temperature) / 131072.0 -
                    ((double)state->calibration.dig_T1) / 8192.0)) *
                  ((double)state->calibration.dig_T3);
    double t_fine = var1 + var2;
    double temperature_c = t_fine / 5120.0;

    var1 = (t_fine / 2.0) - 64000.0;
    var2 = var1 * var1 * ((double)state->calibration.dig_P6) / 32768.0;
    var2 = var2 + var1 * ((double)state->calibration.dig_P5) * 2.0;
    var2 = (var2 / 4.0) + (((double)state->calibration.dig_P4) * 65536.0);
    var1 = (((double)state->calibration.dig_P3) * var1 * var1 / 524288.0 +
            ((double)state->calibration.dig_P2) * var1) /
           524288.0;
    var1 = (1.0 + var1 / 32768.0) * ((double)state->calibration.dig_P1);

    double pressure_pa = 0.0;
    if (var1 != 0.0) {
        pressure_pa = 1048576.0 - (double)raw->adc_pressure;
        pressure_pa = (pressure_pa - (var2 / 4096.0)) * 6250.0 / var1;
        var1 = ((double)state->calibration.dig_P9) * pressure_pa * pressure_pa / 2147483648.0;
        var2 = pressure_pa * ((double)state->calibration.dig_P8) / 32768.0;
        pressure_pa = pressure_pa + (var1 + var2 + ((double)state->calibration.dig_P7)) / 16.0;
    }

    double humidity_pct = t_fine - 76800.0;
    humidity_pct =
        (raw->adc_humidity - (((double)state->calibration.dig_H4) * 64.0 +
                              ((double)state->calibration.dig_H5) / 16384.0 * humidity_pct)) *
        (((double)state->calibration.dig_H2) / 65536.0 *
         (1.0 + ((double)state->calibration.dig_H6) / 67108864.0 * humidity_pct *
                    (1.0 + ((double)state->calibration.dig_H3) / 67108864.0 * humidity_pct)));
    humidity_pct =
        humidity_pct * (1.0 - ((double)state->calibration.dig_H1) * humidity_pct / 524288.0);
    if (humidity_pct > 100.0) {
        humidity_pct = 100.0;
    } else if (humidity_pct < 0.0) {
        humidity_pct = 0.0;
    }

    out_measurement->timestamp_ms = esp_timer_get_time() / BME280_US_PER_MS;
    out_measurement->temperature_deci_c = (int32_t)((temperature_c * 10.0) + 0.5);
    out_measurement->humidity_deci_pct = (int32_t)((humidity_pct * 10.0) + 0.5);
    out_measurement->pressure_deci_hpa = (int32_t)((pressure_pa / 10.0) + 0.5);
}

esp_err_t bme280_hal_backend_init(bme280_hal* self) {
    esp_err_t rv = ESP_OK;

    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    bme280_hw_backend_state* state = calloc(1, sizeof(*state));
    if (state == NULL) {
        return ESP_ERR_NO_MEM;
    }

    self->backend_state = state;
    self->period_ms = BME280_HAL_DEFAULT_PERIOD_MS;

    rv = bme280_hw_connect(self);
    if (rv == ESP_ERR_NOT_FOUND) {
        ESP_LOGW(self->tag, "BME280 not found during init");
        return ESP_OK;
    }
    if (rv != ESP_OK) {
        ESP_LOGW(self->tag, "BME280 init deferred: %s", esp_err_to_name(rv));
        return ESP_OK;
    }

    return ESP_OK;
}

esp_err_t bme280_hal_backend_read(bme280_hal* self, bme280_measurement* out) {
    bme280_hw_backend_state* state = bme280_hw_get_state(self);
    bme280_raw_sample raw = {0};
    esp_err_t rv = ESP_OK;

    if ((self == NULL) || (out == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (state == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    rv = bme280_hw_connect(self);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = bme280_hw_read_raw(state, &raw);
    if (rv != ESP_OK) {
        self->hardware_present = false;
        state->calibration_loaded = false;
        return rv;
    }

    bme280_hw_convert(state, &raw, out);
    return ESP_OK;
}

void bme280_hal_backend_deinit(bme280_hal* self) {
    if (self == NULL) {
        return;
    }

    free(self->backend_state);
    self->backend_state = NULL;
    self->period_ms = 0U;
    self->hardware_present = false;
}
