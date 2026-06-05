#include "bme280/bme280_sensor.hpp"

#include <cmath>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

extern "C" {
#include "board_i2c.h"
}

#ifndef CONFIG_REDMOLE_BME280_MODE
#define CONFIG_REDMOLE_BME280_MODE 1
#endif

#ifndef CONFIG_REDMOLE_BME280_OVERSAMPLING_TEMPERATURE
#define CONFIG_REDMOLE_BME280_OVERSAMPLING_TEMPERATURE 1
#endif

#ifndef CONFIG_REDMOLE_BME280_OVERSAMPLING_PRESSURE
#define CONFIG_REDMOLE_BME280_OVERSAMPLING_PRESSURE 1
#endif

#ifndef CONFIG_REDMOLE_BME280_OVERSAMPLING_HUMIDITY
#define CONFIG_REDMOLE_BME280_OVERSAMPLING_HUMIDITY 1
#endif

#ifndef CONFIG_REDMOLE_BME280_IIR_FILTER
#define CONFIG_REDMOLE_BME280_IIR_FILTER 0
#endif

#ifndef CONFIG_REDMOLE_BME280_STANDBY_TIME
#define CONFIG_REDMOLE_BME280_STANDBY_TIME 5
#endif

namespace redmole::environment {
namespace {

constexpr const char* kTag = "ENV_BME280";
constexpr uint8_t kBme280ChipId = 0x60U;
constexpr uint8_t kBme280ResetCommand = 0xB6U;
constexpr uint8_t kBme280StatusBusyMask = 0x09U;
constexpr uint8_t kBme280ReadyWaitAttempts = 10U;
constexpr uint32_t kBme280ReadyWaitDelayMs = 10U;
constexpr uint32_t kBme280ResetDelayMs = 5U;

constexpr uint8_t BME280_REG_CHIP_ID = 0xD0U;
constexpr uint8_t BME280_REG_RESET = 0xE0U;
constexpr uint8_t BME280_REG_CTRL_HUM = 0xF2U;
constexpr uint8_t BME280_REG_STATUS = 0xF3U;
constexpr uint8_t BME280_REG_CTRL_MEAS = 0xF4U;
constexpr uint8_t BME280_REG_CONFIG = 0xF5U;
constexpr uint8_t BME280_REG_CALIB_00 = 0x88U;
constexpr uint8_t BME280_REG_CALIB_26 = 0xE1U;
constexpr uint8_t BME280_REG_DATA = 0xF7U;

static uint8_t clamp_bme280_setting(int value, uint8_t max_value) {
    if (value < 0) {
        return 0U;
    }

    if (value > max_value) {
        return max_value;
    }

    return static_cast<uint8_t>(value);
}

static uint16_t u16_le(const uint8_t* data) {
    return static_cast<uint16_t>((static_cast<uint16_t>(data[1]) << 8U) | data[0]);
}

static int16_t s16_le(const uint8_t* data) {
    return static_cast<int16_t>(u16_le(data));
}

static int16_t s12(int16_t value) {
    if ((value & 0x0800) != 0) {
        value |= static_cast<int16_t>(0xF000);
    }

    return value;
}

} // namespace

Bme280Source::Bme280Source(uint8_t address) : address_(address) {
}

esp_err_t Bme280Source::init() {
    return ensure_device_handle();
}

bool Bme280Source::probe() {
    uint8_t chip_id = 0;

    if (!board_i2c_probe_address(address_)) {
        mark_disconnected();
        return false;
    }

    if (ensure_device_handle() != ESP_OK) {
        mark_disconnected();
        return false;
    }

    if (read_u8(BME280_REG_CHIP_ID, chip_id) != ESP_OK) {
        mark_disconnected();
        return false;
    }

    if (chip_id != kBme280ChipId) {
        mark_disconnected();
        return false;
    }

    present_ = true;
    return true;
}

esp_err_t Bme280Source::activate() {
    return connect();
}

esp_err_t Bme280Source::poll(MeasurementBatch& batch) {
    Bme280RawSample raw = {};
    Bme280Reading reading = {};
    EnvironmentMeasurement temperature{};
    EnvironmentMeasurement humidity{};
    EnvironmentMeasurement pressure{};

    if (!configured_ || (mode_ == 0U)) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t rv = ESP_OK;
    if ((mode_ == 1U) || (mode_ == 2U)) {
        rv = write_ctrl_meas();
        if (rv != ESP_OK) {
            mark_disconnected();
            return rv;
        }
    }

    rv = read_raw(raw);
    if (rv != ESP_OK) {
        mark_disconnected();
        return rv;
    }

    convert(raw, reading);
    if (!make_temperature(reading.temperature_deci_c, reading.timestamp_ms, temperature) ||
        !make_humidity(reading.humidity_deci_pct, reading.timestamp_ms, humidity) ||
        !make_pressure(reading.pressure_deci_hpa, reading.timestamp_ms, pressure) ||
        !batch.report(temperature) || !batch.report(humidity) || !batch.report(pressure)) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}

esp_err_t Bme280Source::ensure_device_handle() {
    if (dev_ != nullptr) {
        return ESP_OK;
    }

    return board_i2c_add_device(address_, BOARD_I2C_DEFAULT_SPEED_HZ, &dev_);
}

esp_err_t Bme280Source::connect() {
    uint8_t chip_id = 0;

    if (!probe()) {
        return ESP_ERR_NOT_FOUND;
    }

    esp_err_t rv = read_u8(BME280_REG_CHIP_ID, chip_id);
    if (rv != ESP_OK) {
        mark_disconnected();
        return rv;
    }

    if (chip_id != kBme280ChipId) {
        ESP_LOGW(kTag, "Unexpected BME280 chip id at 0x%02x: 0x%02x", address_, chip_id);
        mark_disconnected();
        return ESP_ERR_NOT_FOUND;
    }

    if (!configured_) {
        rv = configure();
        if (rv != ESP_OK) {
            mark_disconnected();
            return rv;
        }
        ESP_LOGI(kTag, "BME280 hardware initialized at 0x%02x", address_);
    }

    present_ = true;
    return ESP_OK;
}

void Bme280Source::mark_disconnected() {
    present_ = false;
    configured_ = false;
}

esp_err_t Bme280Source::configure() {
    esp_err_t rv = board_i2c_write_reg(dev_, BME280_REG_RESET, kBme280ResetCommand);
    if (rv != ESP_OK) {
        return rv;
    }

    vTaskDelay(pdMS_TO_TICKS(kBme280ResetDelayMs));

    rv = wait_until_ready();
    if (rv != ESP_OK) {
        return rv;
    }

    rv = load_calibration();
    if (rv != ESP_OK) {
        return rv;
    }

    mode_ = clamp_bme280_setting(CONFIG_REDMOLE_BME280_MODE, 3U);
    osrs_t_ = clamp_bme280_setting(CONFIG_REDMOLE_BME280_OVERSAMPLING_TEMPERATURE, 5U);
    osrs_p_ = clamp_bme280_setting(CONFIG_REDMOLE_BME280_OVERSAMPLING_PRESSURE, 5U);
    osrs_h_ = clamp_bme280_setting(CONFIG_REDMOLE_BME280_OVERSAMPLING_HUMIDITY, 5U);
    filter_ = clamp_bme280_setting(CONFIG_REDMOLE_BME280_IIR_FILTER, 4U);
    standby_ = clamp_bme280_setting(CONFIG_REDMOLE_BME280_STANDBY_TIME, 7U);

    rv = board_i2c_write_reg(dev_, BME280_REG_CTRL_HUM, osrs_h_);
    if (rv != ESP_OK) {
        return rv;
    }

    const uint8_t config = static_cast<uint8_t>((standby_ << 5U) | (filter_ << 2U));
    rv = board_i2c_write_reg(dev_, BME280_REG_CONFIG, config);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = write_ctrl_meas();
    if (rv != ESP_OK) {
        return rv;
    }

    configured_ = true;
    return ESP_OK;
}

esp_err_t Bme280Source::write_ctrl_meas() {
    const uint8_t ctrl_meas = static_cast<uint8_t>((osrs_t_ << 5U) | (osrs_p_ << 2U) | mode_);
    return board_i2c_write_reg(dev_, BME280_REG_CTRL_MEAS, ctrl_meas);
}

esp_err_t Bme280Source::read_u8(uint8_t reg, uint8_t& out) {
    return board_i2c_read_reg(dev_, reg, &out, 1U);
}

esp_err_t Bme280Source::wait_until_ready() {
    for (uint8_t attempt = 0; attempt < kBme280ReadyWaitAttempts; attempt++) {
        uint8_t status = 0;
        esp_err_t rv = read_u8(BME280_REG_STATUS, status);
        if (rv != ESP_OK) {
            return rv;
        }

        if ((status & kBme280StatusBusyMask) == 0U) {
            return ESP_OK;
        }

        vTaskDelay(pdMS_TO_TICKS(kBme280ReadyWaitDelayMs));
    }

    return ESP_ERR_TIMEOUT;
}

esp_err_t Bme280Source::load_calibration() {
    uint8_t calib0[26] = {};
    uint8_t calib1[7] = {};

    esp_err_t rv = board_i2c_read_reg(dev_, BME280_REG_CALIB_00, calib0, sizeof(calib0));
    if (rv != ESP_OK) {
        return rv;
    }

    rv = board_i2c_read_reg(dev_, BME280_REG_CALIB_26, calib1, sizeof(calib1));
    if (rv != ESP_OK) {
        return rv;
    }

    calibration_.dig_T1 = u16_le(&calib0[0]);
    calibration_.dig_T2 = s16_le(&calib0[2]);
    calibration_.dig_T3 = s16_le(&calib0[4]);
    calibration_.dig_P1 = u16_le(&calib0[6]);
    calibration_.dig_P2 = s16_le(&calib0[8]);
    calibration_.dig_P3 = s16_le(&calib0[10]);
    calibration_.dig_P4 = s16_le(&calib0[12]);
    calibration_.dig_P5 = s16_le(&calib0[14]);
    calibration_.dig_P6 = s16_le(&calib0[16]);
    calibration_.dig_P7 = s16_le(&calib0[18]);
    calibration_.dig_P8 = s16_le(&calib0[20]);
    calibration_.dig_P9 = s16_le(&calib0[22]);
    calibration_.dig_H1 = calib0[25];
    calibration_.dig_H2 = s16_le(&calib1[0]);
    calibration_.dig_H3 = calib1[2];
    calibration_.dig_H4 =
        s12(static_cast<int16_t>((static_cast<int16_t>(calib1[3]) << 4U) | (calib1[4] & 0x0FU)));
    calibration_.dig_H5 =
        s12(static_cast<int16_t>((static_cast<int16_t>(calib1[5]) << 4U) | (calib1[4] >> 4U)));
    calibration_.dig_H6 = static_cast<int8_t>(calib1[6]);
    return ESP_OK;
}

esp_err_t Bme280Source::read_raw(Bme280RawSample& out_raw) {
    uint8_t data[8] = {};

    esp_err_t rv = wait_until_ready();
    if (rv != ESP_OK) {
        return rv;
    }

    rv = board_i2c_read_reg(dev_, BME280_REG_DATA, data, sizeof(data));
    if (rv != ESP_OK) {
        return rv;
    }

    out_raw.adc_pressure = (static_cast<int32_t>(data[0]) << 12U) |
                           (static_cast<int32_t>(data[1]) << 4U) | (data[2] >> 4U);
    out_raw.adc_temperature = (static_cast<int32_t>(data[3]) << 12U) |
                              (static_cast<int32_t>(data[4]) << 4U) | (data[5] >> 4U);
    out_raw.adc_humidity = (static_cast<int32_t>(data[6]) << 8U) | data[7];
    return ESP_OK;
}

void Bme280Source::convert(const Bme280RawSample& raw, Bme280Reading& out) const {
    double var1 = ((static_cast<double>(raw.adc_temperature) / 16384.0) -
                   (static_cast<double>(calibration_.dig_T1) / 1024.0)) *
                  static_cast<double>(calibration_.dig_T2);
    double var2 = (((static_cast<double>(raw.adc_temperature) / 131072.0) -
                    (static_cast<double>(calibration_.dig_T1) / 8192.0)) *
                   ((static_cast<double>(raw.adc_temperature) / 131072.0) -
                    (static_cast<double>(calibration_.dig_T1) / 8192.0))) *
                  static_cast<double>(calibration_.dig_T3);
    const double t_fine = var1 + var2;
    const double temperature_c = t_fine / 5120.0;

    var1 = (t_fine / 2.0) - 64000.0;
    var2 = var1 * var1 * static_cast<double>(calibration_.dig_P6) / 32768.0;
    var2 = var2 + var1 * static_cast<double>(calibration_.dig_P5) * 2.0;
    var2 = (var2 / 4.0) + (static_cast<double>(calibration_.dig_P4) * 65536.0);
    var1 = ((static_cast<double>(calibration_.dig_P3) * var1 * var1 / 524288.0) +
            (static_cast<double>(calibration_.dig_P2) * var1)) /
           524288.0;
    var1 = (1.0 + (var1 / 32768.0)) * static_cast<double>(calibration_.dig_P1);

    double pressure_pa = 0.0;
    if (var1 != 0.0) {
        pressure_pa = 1048576.0 - static_cast<double>(raw.adc_pressure);
        pressure_pa = (pressure_pa - (var2 / 4096.0)) * 6250.0 / var1;
        var1 = static_cast<double>(calibration_.dig_P9) * pressure_pa * pressure_pa / 2147483648.0;
        var2 = pressure_pa * static_cast<double>(calibration_.dig_P8) / 32768.0;
        pressure_pa = pressure_pa + (var1 + var2 + static_cast<double>(calibration_.dig_P7)) / 16.0;
    }

    double humidity_pct = t_fine - 76800.0;
    humidity_pct =
        (raw.adc_humidity - ((static_cast<double>(calibration_.dig_H4) * 64.0) +
                             (static_cast<double>(calibration_.dig_H5) / 16384.0 * humidity_pct))) *
        (static_cast<double>(calibration_.dig_H2) / 65536.0 *
         (1.0 + (static_cast<double>(calibration_.dig_H6) / 67108864.0 * humidity_pct *
                 (1.0 + (static_cast<double>(calibration_.dig_H3) / 67108864.0 * humidity_pct)))));
    humidity_pct =
        humidity_pct * (1.0 - (static_cast<double>(calibration_.dig_H1) * humidity_pct / 524288.0));
    if (humidity_pct > 100.0) {
        humidity_pct = 100.0;
    } else if (humidity_pct < 0.0) {
        humidity_pct = 0.0;
    }

    out.timestamp_ms = esp_timer_get_time() / kUsPerMs;
    out.temperature_deci_c = static_cast<int32_t>(std::lround(temperature_c * 10.0));
    out.humidity_deci_pct = static_cast<int32_t>(std::lround(humidity_pct * 10.0));
    out.pressure_deci_hpa = static_cast<int32_t>(std::lround(pressure_pa / 10.0));
}

} // namespace redmole::environment
