#include "bme280/bme280_sensor.hpp"

#include <cmath>

#include "esp_log.h"
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

namespace redmole::environment::bme280 {
namespace {

constexpr const char* kTag = "ENV_BME280";
constexpr uint8_t kBme280ChipId = 0x60U;
constexpr uint8_t kBme280ResetCommand = 0xB6U;
constexpr uint8_t kBme280ReadyWaitAttempts = 100U;
constexpr uint32_t kBme280ReadyWaitDelayMs = 2U;
constexpr uint32_t kBme280ResetDelayMs = 5U;
constexpr int32_t kSkippedTemperatureOrPressure = 0x80000;
constexpr int32_t kSkippedHumidity = 0x8000;

constexpr uint8_t BME280_REG_CHIP_ID = 0xD0U;
constexpr uint8_t BME280_REG_RESET = 0xE0U;
constexpr uint8_t BME280_REG_CTRL_HUM = 0xF2U;
constexpr uint8_t BME280_REG_STATUS = 0xF3U;
constexpr uint8_t BME280_REG_CTRL_MEAS = 0xF4U;
constexpr uint8_t BME280_REG_CONFIG = 0xF5U;
constexpr uint8_t BME280_REG_CALIB_00 = 0x88U;
constexpr uint8_t BME280_REG_CALIB_26 = 0xE1U;
constexpr uint8_t BME280_REG_DATA = 0xF7U;

static uint16_t read_unsigned_16_le(const uint8_t* data) {
    return static_cast<uint16_t>((static_cast<uint16_t>(data[1]) << 8U) | data[0]);
}

static int16_t read_signed_16_le(const uint8_t* data) {
    return static_cast<int16_t>(read_unsigned_16_le(data));
}

static int16_t sign_extend_12_bit(int16_t value) {
    if ((value & 0x0800) != 0) {
        value |= static_cast<int16_t>(0xF000);
    }

    return value;
}

static int32_t read_unsigned_20_be(const uint8_t* data) {
    return (static_cast<int32_t>(data[0]) << 12U) | (static_cast<int32_t>(data[1]) << 4U) |
           (data[2] >> 4U);
}

static int32_t read_unsigned_16_be(const uint8_t* data) {
    return (static_cast<int32_t>(data[0]) << 8U) | data[1];
}

static bool is_forced_mode(Bme280Mode mode) {
    return mode == Bme280Mode::Forced || mode == Bme280Mode::ForcedAlternate;
}

static bool settings_are_valid(const Bme280Settings& settings) {
    if (static_cast<uint8_t>(settings.mode) > static_cast<uint8_t>(Bme280Mode::Normal)) {
        return false;
    }

    if (static_cast<uint8_t>(settings.temperature_oversampling) >
        static_cast<uint8_t>(Bme280Oversampling::X16)) {
        return false;
    }

    if (static_cast<uint8_t>(settings.pressure_oversampling) >
        static_cast<uint8_t>(Bme280Oversampling::X16)) {
        return false;
    }

    if (static_cast<uint8_t>(settings.humidity_oversampling) >
        static_cast<uint8_t>(Bme280Oversampling::X16)) {
        return false;
    }

    if (static_cast<uint8_t>(settings.filter) > static_cast<uint8_t>(Bme280Filter::Coefficient16)) {
        return false;
    }

    if (static_cast<uint8_t>(settings.standby) > static_cast<uint8_t>(Bme280Standby::Ms20)) {
        return false;
    }

    if (settings.temperature_oversampling == Bme280Oversampling::Skipped) {
        return false;
    }

    if (settings.pressure_oversampling == Bme280Oversampling::Skipped) {
        return false;
    }

    if (settings.humidity_oversampling == Bme280Oversampling::Skipped) {
        return false;
    }

    return true;
}

static bool settings_match(const Bme280Settings& first, const Bme280Settings& second) {
    if (first.mode == Bme280Mode::Normal) {
        if (second.mode != Bme280Mode::Normal) {
            return false;
        }
    }

    if (first.mode == Bme280Mode::Sleep) {
        if (second.mode != Bme280Mode::Sleep) {
            return false;
        }
    }

    if (first.mode == Bme280Mode::Forced) {
        if (second.mode == Bme280Mode::Normal) {
            return false;
        }
    }

    if (first.mode == Bme280Mode::ForcedAlternate) {
        if (second.mode == Bme280Mode::Normal) {
            return false;
        }
    }

    if (first.temperature_oversampling != second.temperature_oversampling) {
        return false;
    }

    if (first.pressure_oversampling != second.pressure_oversampling) {
        return false;
    }

    if (first.humidity_oversampling != second.humidity_oversampling) {
        return false;
    }

    if (first.filter != second.filter) {
        return false;
    }

    if (first.standby != second.standby) {
        return false;
    }

    return true;
}

} // namespace

Bme280Sensor::Bme280Sensor(uint8_t address, const Bme280Settings& settings)
    : address_(address), settings_(settings) {
}

esp_err_t Bme280Sensor::init() {
    esp_err_t result;

    ready_for_reads_ = false;

    if (device_ == nullptr) {
        result = board_i2c_add_device(address_, BOARD_I2C_DEFAULT_SPEED_HZ, &device_);
        if (result != ESP_OK) {
            return result;
        }
    }

    result = check_chip_id();
    if (result != ESP_OK) {
        return result;
    }

    if (!settings_are_valid(settings_)) {
        return ESP_ERR_INVALID_ARG;
    }

    result = configure();
    if (result != ESP_OK) {
        return result;
    }

    ready_for_reads_ = true;
    ESP_LOGI(kTag, "BME280 initialized at 0x%02x", address_);
    return ESP_OK;
}

esp_err_t Bme280Sensor::read(Bme280Reading& out) {
    Bme280RawSample raw = {};
    esp_err_t result;

    if (!ready_for_reads_) {
        result = init();
        if (result != ESP_OK) {
            return result;
        }
    }
    if (settings_.mode == Bme280Mode::Sleep) {
        return ESP_ERR_INVALID_STATE;
    }

    if (is_forced_mode(settings_.mode)) {
        result = write_ctrl_meas(settings_.mode);
        if (result != ESP_OK) {
            ready_for_reads_ = false;
            return result;
        }
    }

    result = read_raw(raw);
    if (result != ESP_OK) {
        ready_for_reads_ = false;
        return result;
    }

    result = convert(raw, out);
    if (result != ESP_OK) {
        ready_for_reads_ = false;
        return result;
    }

    return ESP_OK;
}

esp_err_t Bme280Sensor::check_chip_id() {
    uint8_t chip_id = 0;
    esp_err_t result;

    if (!board_i2c_probe_address(address_)) {
        return ESP_ERR_NOT_FOUND;
    }

    result = board_i2c_read_reg(device_, BME280_REG_CHIP_ID, &chip_id, 1U);
    if (result != ESP_OK) {
        return result;
    }

    if (chip_id != kBme280ChipId) {
        ESP_LOGW(kTag, "Unexpected BME280 chip id at 0x%02x: 0x%02x", address_, chip_id);
        return ESP_ERR_NOT_FOUND;
    }

    return ESP_OK;
}

esp_err_t Bme280Sensor::configure() {
    esp_err_t result;

    result = reset();
    if (result != ESP_OK) {
        return result;
    }

    vTaskDelay(pdMS_TO_TICKS(kBme280ResetDelayMs));

    result = wait_until_ready();
    if (result != ESP_OK) {
        return result;
    }

    result = load_calibration();
    if (result != ESP_OK) {
        return result;
    }

    result = apply_settings(settings_);
    if (result != ESP_OK) {
        return result;
    }

    return ESP_OK;
}

esp_err_t Bme280Sensor::reset() {
    esp_err_t result;

    if (device_ == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    result = board_i2c_write_reg(device_, BME280_REG_RESET, kBme280ResetCommand);
    ready_for_reads_ = false;
    return result;
}

esp_err_t Bme280Sensor::apply_settings(const Bme280Settings& settings) {
    Bme280Settings readback = {};
    uint8_t config_value;
    esp_err_t result;

    if (device_ == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!settings_are_valid(settings)) {
        return ESP_ERR_INVALID_ARG;
    }

    settings_ = settings;

    result = write_ctrl_meas(Bme280Mode::Sleep);
    if (result != ESP_OK) {
        ready_for_reads_ = false;
        return result;
    }

    result = wait_until_ready();
    if (result != ESP_OK) {
        ready_for_reads_ = false;
        return result;
    }

    result = board_i2c_write_reg(device_, BME280_REG_CTRL_HUM,
                                 static_cast<uint8_t>(settings_.humidity_oversampling));
    if (result != ESP_OK) {
        ready_for_reads_ = false;
        return result;
    }

    config_value = static_cast<uint8_t>((static_cast<uint8_t>(settings_.standby) << 5U) |
                                        (static_cast<uint8_t>(settings_.filter) << 2U));
    result = board_i2c_write_reg(device_, BME280_REG_CONFIG, config_value);
    if (result != ESP_OK) {
        ready_for_reads_ = false;
        return result;
    }

    result = write_ctrl_meas(settings_.mode);
    if (result != ESP_OK) {
        ready_for_reads_ = false;
        return result;
    }

    result = read_settings(readback);
    if (result != ESP_OK) {
        ready_for_reads_ = false;
        return result;
    }

    if (!settings_match(settings_, readback)) {
        ready_for_reads_ = false;
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}

esp_err_t Bme280Sensor::read_settings(Bme280Settings& settings) {
    uint8_t control_humidity;
    uint8_t control_measurement;
    uint8_t config;
    esp_err_t result;

    if (device_ == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    result = board_i2c_read_reg(device_, BME280_REG_CTRL_HUM, &control_humidity, 1U);
    if (result != ESP_OK) {
        return result;
    }

    result = board_i2c_read_reg(device_, BME280_REG_CTRL_MEAS, &control_measurement, 1U);
    if (result != ESP_OK) {
        return result;
    }

    result = board_i2c_read_reg(device_, BME280_REG_CONFIG, &config, 1U);
    if (result != ESP_OK) {
        return result;
    }

    settings.mode = static_cast<Bme280Mode>(control_measurement & 0x03U);
    settings.temperature_oversampling =
        static_cast<Bme280Oversampling>((control_measurement >> 5U) & 0x07U);
    settings.pressure_oversampling =
        static_cast<Bme280Oversampling>((control_measurement >> 2U) & 0x07U);
    settings.humidity_oversampling = static_cast<Bme280Oversampling>(control_humidity & 0x07U);
    settings.filter = static_cast<Bme280Filter>((config >> 2U) & 0x07U);
    settings.standby = static_cast<Bme280Standby>((config >> 5U) & 0x07U);
    return ESP_OK;
}

esp_err_t Bme280Sensor::read_status(Bme280Status& status) {
    uint8_t status_register;
    esp_err_t result;

    if (device_ == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    result = board_i2c_read_reg(device_, BME280_REG_STATUS, &status_register, 1U);
    if (result != ESP_OK) {
        return result;
    }

    status.measuring = (status_register & 0x08U) != 0U;
    status.updating_calibration = (status_register & 0x01U) != 0U;
    return ESP_OK;
}

esp_err_t Bme280Sensor::write_ctrl_meas(Bme280Mode mode) {
    const uint8_t temperature_bits = static_cast<uint8_t>(settings_.temperature_oversampling) << 5U;
    const uint8_t pressure_bits = static_cast<uint8_t>(settings_.pressure_oversampling) << 2U;
    const uint8_t mode_bits = static_cast<uint8_t>(mode);
    const uint8_t control_value = temperature_bits | pressure_bits | mode_bits;

    return board_i2c_write_reg(device_, BME280_REG_CTRL_MEAS, control_value);
}

esp_err_t Bme280Sensor::wait_until_ready() {
    Bme280Status status = {};
    uint8_t attempt;
    esp_err_t result;

    for (attempt = 0; attempt < kBme280ReadyWaitAttempts; attempt++) {
        result = read_status(status);
        if (result != ESP_OK) {
            return result;
        }

        if (!status.measuring) {
            if (!status.updating_calibration) {
                return ESP_OK;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(kBme280ReadyWaitDelayMs));
    }

    return ESP_ERR_TIMEOUT;
}

esp_err_t Bme280Sensor::load_calibration() {
    uint8_t calib0[26] = {};
    uint8_t calib1[7] = {};
    esp_err_t result;

    result = board_i2c_read_reg(device_, BME280_REG_CALIB_00, calib0, sizeof(calib0));
    if (result != ESP_OK) {
        return result;
    }

    result = board_i2c_read_reg(device_, BME280_REG_CALIB_26, calib1, sizeof(calib1));
    if (result != ESP_OK) {
        return result;
    }

    calibration_.dig_T1 = read_unsigned_16_le(&calib0[0]);
    calibration_.dig_T2 = read_signed_16_le(&calib0[2]);
    calibration_.dig_T3 = read_signed_16_le(&calib0[4]);
    calibration_.dig_P1 = read_unsigned_16_le(&calib0[6]);
    calibration_.dig_P2 = read_signed_16_le(&calib0[8]);
    calibration_.dig_P3 = read_signed_16_le(&calib0[10]);
    calibration_.dig_P4 = read_signed_16_le(&calib0[12]);
    calibration_.dig_P5 = read_signed_16_le(&calib0[14]);
    calibration_.dig_P6 = read_signed_16_le(&calib0[16]);
    calibration_.dig_P7 = read_signed_16_le(&calib0[18]);
    calibration_.dig_P8 = read_signed_16_le(&calib0[20]);
    calibration_.dig_P9 = read_signed_16_le(&calib0[22]);
    calibration_.dig_H1 = calib0[25];
    calibration_.dig_H2 = read_signed_16_le(&calib1[0]);
    calibration_.dig_H3 = calib1[2];
    calibration_.dig_H4 = sign_extend_12_bit(
        static_cast<int16_t>((static_cast<int16_t>(calib1[3]) << 4U) | (calib1[4] & 0x0FU)));
    calibration_.dig_H5 = sign_extend_12_bit(
        static_cast<int16_t>((static_cast<int16_t>(calib1[5]) << 4U) | (calib1[4] >> 4U)));
    calibration_.dig_H6 = static_cast<int8_t>(calib1[6]);
    return ESP_OK;
}

esp_err_t Bme280Sensor::read_raw(Bme280RawSample& out_raw) {
    uint8_t data[8] = {};
    esp_err_t result;

    if (is_forced_mode(settings_.mode)) {
        result = wait_until_ready();
        if (result != ESP_OK) {
            return result;
        }
    }

    result = board_i2c_read_reg(device_, BME280_REG_DATA, data, sizeof(data));
    if (result != ESP_OK) {
        return result;
    }

    out_raw.adc_pressure = read_unsigned_20_be(&data[0]);
    out_raw.adc_temperature = read_unsigned_20_be(&data[3]);
    out_raw.adc_humidity = read_unsigned_16_be(&data[6]);
    return ESP_OK;
}

esp_err_t Bme280Sensor::convert(const Bme280RawSample& raw, Bme280Reading& out) const {
    double compensation_value_1;
    double compensation_value_2;
    double t_fine;
    double temperature_c;
    double pressure_pa;
    double humidity_pct;

    if (raw.adc_temperature == kSkippedTemperatureOrPressure) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    if (raw.adc_pressure == kSkippedTemperatureOrPressure) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    if (raw.adc_humidity == kSkippedHumidity) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    // These steps follow the BME280 datasheet compensation formulas.
    // TODO should we use the bosch provided code for this?
    compensation_value_1 = ((static_cast<double>(raw.adc_temperature) / 16384.0) -
                            (static_cast<double>(calibration_.dig_T1) / 1024.0)) *
                           static_cast<double>(calibration_.dig_T2);
    compensation_value_2 = (((static_cast<double>(raw.adc_temperature) / 131072.0) -
                             (static_cast<double>(calibration_.dig_T1) / 8192.0)) *
                            ((static_cast<double>(raw.adc_temperature) / 131072.0) -
                             (static_cast<double>(calibration_.dig_T1) / 8192.0))) *
                           static_cast<double>(calibration_.dig_T3);
    t_fine = compensation_value_1 + compensation_value_2;
    temperature_c = t_fine / 5120.0;

    compensation_value_1 = (t_fine / 2.0) - 64000.0;
    compensation_value_2 = compensation_value_1 * compensation_value_1 *
                           static_cast<double>(calibration_.dig_P6) / 32768.0;
    compensation_value_2 = compensation_value_2 +
                           compensation_value_1 * static_cast<double>(calibration_.dig_P5) * 2.0;
    compensation_value_2 =
        (compensation_value_2 / 4.0) + (static_cast<double>(calibration_.dig_P4) * 65536.0);
    compensation_value_1 = ((static_cast<double>(calibration_.dig_P3) * compensation_value_1 *
                             compensation_value_1 / 524288.0) +
                            (static_cast<double>(calibration_.dig_P2) * compensation_value_1)) /
                           524288.0;
    compensation_value_1 =
        (1.0 + (compensation_value_1 / 32768.0)) * static_cast<double>(calibration_.dig_P1);

    if (compensation_value_1 == 0.0) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    pressure_pa = 1048576.0 - static_cast<double>(raw.adc_pressure);
    pressure_pa = (pressure_pa - (compensation_value_2 / 4096.0)) * 6250.0 / compensation_value_1;
    compensation_value_1 =
        static_cast<double>(calibration_.dig_P9) * pressure_pa * pressure_pa / 2147483648.0;
    compensation_value_2 = pressure_pa * static_cast<double>(calibration_.dig_P8) / 32768.0;
    pressure_pa = pressure_pa + (compensation_value_1 + compensation_value_2 +
                                 static_cast<double>(calibration_.dig_P7)) /
                                    16.0;

    humidity_pct = t_fine - 76800.0;
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
    }

    if (humidity_pct < 0.0) {
        humidity_pct = 0.0;
    }

    if (!std::isfinite(temperature_c) || !std::isfinite(humidity_pct) ||
        !std::isfinite(pressure_pa)) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    out.temperature.milli_c = std::llround(temperature_c * 1000.0);
    out.humidity.milli_pct = std::llround(humidity_pct * 1000.0);
    out.pressure.pa = std::llround(pressure_pa);
    return ESP_OK;
}

} // namespace redmole::environment::bme280
