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

/** @brief Compensated values from one physical BME280 acquisition. */
struct Bme280Reading {
    int64_t timestamp_ms;       /*!< Monotonic acquisition time in milliseconds. */
    int32_t temperature_deci_c; /*!< Temperature in 0.1 degrees Celsius. */
    int32_t humidity_deci_pct;  /*!< Relative humidity in 0.1 percent. */
    int32_t pressure_deci_hpa;  /*!< Pressure in 0.1 hectopascals. */
};

/**
 * @brief Process-lifetime BME280 source bound to one configured I2C address.
 */
class Bme280Source final : public EnvironmentSource {
  public:
    /**
     * @brief Construct a BME280 source for one seven-bit I2C address.
     * @param address Configured seven-bit BME280 address.
     */
    explicit Bme280Source(uint8_t address);
    Bme280Source(const Bme280Source&) = delete;
    Bme280Source& operator=(const Bme280Source&) = delete;
    Bme280Source(Bme280Source&&) = delete;
    Bme280Source& operator=(Bme280Source&&) = delete;

    /**
     * @brief Register the permanent board_i2c device handle.
     * @return ESP_OK when the permanent I2C device handle is registered.
     */
    esp_err_t init() override;
    /**
     * @brief Probe the configured address and verify the BME280 chip ID.
     * @return True when the configured address responds with the BME280 chip ID.
     */
    bool probe() override;
    /**
     * @brief Reset and configure detected BME280 hardware.
     * @return ESP_OK when calibration and measurement settings are loaded.
     */
    esp_err_t activate() override;

    /**
     * @brief Acquire and report temperature, humidity, and pressure together.
     * @param batch Empty manager-owned batch receiving copied measurements.
     * @return ESP_OK on complete success, otherwise an ESP-IDF error code.
     */
    esp_err_t poll(MeasurementBatch& batch) override;

  private:
    esp_err_t ensure_device_handle();
    esp_err_t connect();
    void mark_disconnected();
    esp_err_t configure();
    esp_err_t write_ctrl_meas();
    esp_err_t read_u8(uint8_t reg, uint8_t& out);
    esp_err_t wait_until_ready();
    esp_err_t load_calibration();
    esp_err_t read_raw(Bme280RawSample& out_raw);
    void convert(const Bme280RawSample& raw, Bme280Reading& out) const;

    uint8_t address_;                        /*!< Configured seven-bit I2C address. */
    i2c_master_dev_handle_t dev_ = nullptr;  /*!< Process-lifetime board_i2c device handle. */
    Bme280Calibration calibration_ = {};     /*!< Calibration loaded during activation. */
    bool present_ = false;                   /*!< True after successful identity probing. */
    bool configured_ = false;                /*!< True when the device is ready for polling. */
    uint8_t mode_ = 1U;                      /*!< BME280 ctrl_meas mode field. */
    uint8_t osrs_t_ = 1U;                    /*!< Temperature oversampling register field. */
    uint8_t osrs_p_ = 1U;                    /*!< Pressure oversampling register field. */
    uint8_t osrs_h_ = 1U;                    /*!< Humidity oversampling register field. */
    uint8_t filter_ = 0U;                    /*!< IIR filter register field. */
    uint8_t standby_ = 5U;                   /*!< Normal-mode standby register field. */
};

} // namespace redmole::environment
