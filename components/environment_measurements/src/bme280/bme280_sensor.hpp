#pragma once

/**
 * @file
 * @brief BME280 hardware source for environment measurements.
 *
 * The source retains one board_i2c device handle for its lifetime and converts
 * each physical acquisition into a coherent temperature, humidity, and pressure
 * batch.
 */

#include <cstdint>

#include "driver/i2c_master.h"
#include "environment_sensor.hpp"

namespace redmole::environment {

/** @brief Uncompensated register values from one BME280 acquisition. */
struct Bme280RawSample {
    int32_t adc_temperature; /*!< Uncompensated 20-bit temperature ADC value. */
    int32_t adc_pressure;    /*!< Uncompensated 20-bit pressure ADC value. */
    int32_t adc_humidity;    /*!< Uncompensated 16-bit humidity ADC value. */
};

/** @brief Factory calibration coefficients retained while the source is active. */
struct Bme280Calibration {
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
};

/**
 * @brief Process-lifetime BME280 source bound to one configured I2C address.
 */
class Bme280Sensor final : public EnvironmentSensor {
  public:
    /**
     * @brief Construct a BME280 source for one seven-bit I2C address.
     * @param address Configured seven-bit BME280 address.
     */
    explicit Bme280Sensor(uint8_t address);

    /**
     * @brief Register the permanent board_i2c device handle.
     * @return ESP_OK when the permanent I2C device handle is registered.
     */
    esp_err_t init() override;

    /**
     * @brief Acquire and report temperature, humidity, and pressure together.
     * @param sample Output sample.
     * @return ESP_OK on complete success, otherwise an ESP-IDF error code.
     */
    esp_err_t read(environment_measurement_sample_t& sample) override;

  private:
    esp_err_t ensure_device_handle();
    esp_err_t check_chip_id();
    esp_err_t configure();
    esp_err_t write_ctrl_meas();
    esp_err_t read_u8(uint8_t reg, uint8_t& out);
    esp_err_t wait_until_ready();
    esp_err_t load_calibration();
    esp_err_t read_raw(Bme280RawSample& out_raw);
    void convert(const Bme280RawSample& raw, environment_measurement_sample_t& out) const;

    uint8_t address_;                        /*!< Configured seven-bit I2C address. */
    i2c_master_dev_handle_t dev_ = nullptr;  /*!< Process-lifetime board_i2c device handle. */
    Bme280Calibration calibration_ = {};     /*!< Calibration loaded during initialization. */
    bool configured_ = false;                /*!< True when the device is ready for polling. */
    uint8_t mode_ = 1U;                      /*!< BME280 ctrl_meas mode field. */
    uint8_t osrs_t_ = 1U;                    /*!< Temperature oversampling register field. */
    uint8_t osrs_p_ = 1U;                    /*!< Pressure oversampling register field. */
    uint8_t osrs_h_ = 1U;                    /*!< Humidity oversampling register field. */
    uint8_t filter_ = 0U;                    /*!< IIR filter register field. */
    uint8_t standby_ = 5U;                   /*!< Normal-mode standby register field. */
};

} // namespace redmole::environment
