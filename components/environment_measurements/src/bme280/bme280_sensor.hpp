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

/** @brief Power and acquisition modes accepted by the BME280 mode field. */
enum class Bme280Mode : uint8_t {
    Sleep = 0,           /*!< No conversion runs until another mode is written. */
    Forced = 1,          /*!< Run one conversion and return automatically to sleep. */
    ForcedAlternate = 2, /*!< Second register encoding with the same behavior as forced mode. */
    Normal = 3,          /*!< Repeat conversions using the configured standby period. */
};

/** @brief Per-channel oversampling accepted by the BME280 control registers. */
enum class Bme280Oversampling : uint8_t {
    Skipped = 0, /*!< Disable this measurement channel. */
    X1 = 1,      /*!< Perform one internal conversion. */
    X2 = 2,      /*!< Average two internal conversions. */
    X4 = 3,      /*!< Average four internal conversions. */
    X8 = 4,      /*!< Average eight internal conversions. */
    X16 = 5,     /*!< Average sixteen internal conversions. */
};

/** @brief IIR filter coefficients accepted by the BME280 config register. */
enum class Bme280Filter : uint8_t {
    Off = 0,           /*!< Do not filter pressure or temperature. */
    Coefficient2 = 1,  /*!< Use IIR filter coefficient 2. */
    Coefficient4 = 2,  /*!< Use IIR filter coefficient 4. */
    Coefficient8 = 3,  /*!< Use IIR filter coefficient 8. */
    Coefficient16 = 4, /*!< Use IIR filter coefficient 16. */
};

/** @brief Inactive periods available between measurements in normal mode. */
enum class Bme280Standby : uint8_t {
    Ms0_5 = 0,  /*!< Wait 0.5 milliseconds. */
    Ms62_5 = 1, /*!< Wait 62.5 milliseconds. */
    Ms125 = 2,  /*!< Wait 125 milliseconds. */
    Ms250 = 3,  /*!< Wait 250 milliseconds. */
    Ms500 = 4,  /*!< Wait 500 milliseconds. */
    Ms1000 = 5, /*!< Wait 1000 milliseconds. */
    Ms10 = 6,   /*!< Wait 10 milliseconds. */
    Ms20 = 7,   /*!< Wait 20 milliseconds. */
};

/**
 * @brief Complete set of BME280 measurement controls available over I2C.
 *
 * Temperature must be enabled whenever pressure or humidity is enabled because
 * both compensation formulas use the fine temperature value.
 */
struct Bme280Settings {
    Bme280Mode mode;                             /*!< Acquisition and power mode. */
    Bme280Oversampling temperature_oversampling; /*!< Temperature channel setting. */
    Bme280Oversampling pressure_oversampling;    /*!< Pressure channel setting. */
    Bme280Oversampling humidity_oversampling;    /*!< Humidity channel setting. */
    Bme280Filter filter;                         /*!< Pressure and temperature IIR filter. */
    Bme280Standby standby;                       /*!< Normal-mode inactive period. */
};

/** @brief Live work flags reported by the BME280 status register. */
struct Bme280Status {
    bool measuring;            /*!< A measurement conversion is running. */
    bool updating_calibration; /*!< Calibration data is being copied from internal memory. */
};

/** @brief Raw register values that must be compensated with this device's calibration. */
struct Bme280RawSample {
    int32_t adc_temperature; /*!< Uncompensated 20-bit temperature ADC value. */
    int32_t adc_pressure;    /*!< Uncompensated 20-bit pressure ADC value. */
    int32_t adc_humidity;    /*!< Uncompensated 16-bit humidity ADC value. */
};

/**
 * @brief Compensated BME280 values in native engineering units.
 *
 * Presence flags remain false for channels disabled by the current settings.
 */
struct Bme280Data {
    int64_t timestamp_ms; /*!< Acquisition time in milliseconds since boot. */
    double temperature_c; /*!< Compensated temperature in degrees Celsius. */
    double pressure_pa;   /*!< Compensated pressure in pascals. */
    double humidity_pct;  /*!< Compensated relative humidity in percent. */
    bool has_temperature; /*!< The temperature field contains a measurement. */
    bool has_pressure;    /*!< The pressure field contains a measurement. */
    bool has_humidity;    /*!< The humidity field contains a measurement. */
};

/** @brief Factory calibration coefficients read from one BME280. */
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
 * @brief Read one explicitly addressed BME280 on the shared board I2C bus.
 *
 * A failed read marks the device unready. The following read then performs the
 * complete initialization sequence again, which allows recovery after a sensor
 * is disconnected and reconnected.
 */
class Bme280Sensor final : public EnvironmentSensor {
  public:
    /**
     * @brief Construct a BME280 source for one seven-bit I2C address and settings.
     * @param address Configured seven-bit BME280 address.
     * @param settings Initial measurement controls to apply during initialization.
     */
    Bme280Sensor(uint8_t address, const Bme280Settings& settings);

    /**
     * @brief Verify, reset, calibrate, and configure the sensor.
     * @return ESP_OK when reads can begin, otherwise an ESP-IDF error code.
     */
    esp_err_t init() override;

    /**
     * @brief Acquire enabled values and convert them to the environment API units.
     * @param reading Cleared output record populated with enabled measurements.
     * @return ESP_OK on complete success, otherwise an ESP-IDF error code.
     */
    esp_err_t read(EnvironmentSensorReading& reading) override;

    /**
     * @brief Acquire compensated values without reducing the driver's precision.
     * @param data Cleared output populated in degrees Celsius, pascals, and percent.
     * @return ESP_OK when the acquisition completed, otherwise an ESP-IDF error code.
     */
    esp_err_t read_data(Bme280Data& data);

    /**
     * @brief Perform the BME280 software reset command.
     *
     * The next read performs full initialization and reapplies current settings.
     *
     * @return ESP_OK when the reset command was accepted, otherwise an I2C error.
     */
    esp_err_t reset();

    /**
     * @brief Validate and apply every I2C-relevant BME280 measurement setting.
     *
     * The sensor is placed in sleep mode before config is written because the
     * BME280 may ignore config writes in normal mode.
     *
     * @param settings Complete settings to apply and retain for recovery.
     * @return ESP_OK when readback matches, ESP_ERR_INVALID_ARG for an invalid
     *         combination, otherwise an I2C error.
     */
    esp_err_t apply_settings(const Bme280Settings& settings);

    /**
     * @brief Read the active measurement settings back from the sensor.
     * @param settings Output populated from ctrl_hum, ctrl_meas, and config.
     * @return ESP_OK when all settings registers were read, otherwise an I2C error.
     */
    esp_err_t read_settings(Bme280Settings& settings);

    /**
     * @brief Read the sensor's measurement and calibration-update flags.
     * @param status Output populated from the BME280 status register.
     * @return ESP_OK when status was read, otherwise an I2C error.
     */
    esp_err_t read_status(Bme280Status& status);

    /**
     * @brief Read one coherent block of uncompensated measurement registers.
     *
     * This is available for diagnostics. Normal application code should use
     * read(), which applies calibration and marks skipped channels.
     *
     * @param out_raw Output populated from the sensor's measurement register block.
     * @return ESP_OK when data was read, otherwise an I2C or timeout error.
     */
    esp_err_t read_raw(Bme280RawSample& out_raw);

  private:
    /**
     * @brief Confirm that the responding device has the BME280 chip ID.
     * @return ESP_OK for a BME280, ESP_ERR_NOT_FOUND for no device or a different chip.
     */
    esp_err_t check_chip_id();

    /**
     * @brief Reset the sensor and apply calibration and build-time settings.
     * @return ESP_OK when configuration completed, otherwise an I2C or timeout error.
     */
    esp_err_t configure();

    /**
     * @brief Write the current oversampling fields and requested operating mode.
     * @param mode Mode field to write with the retained oversampling settings.
     * @return Result from writing the BME280 measurement-control register.
     */
    esp_err_t write_ctrl_meas(Bme280Mode mode);

    /**
     * @brief Wait until reset or measurement work reported by the sensor is complete.
     * @return ESP_OK when ready, ESP_ERR_TIMEOUT after the bounded wait, or an I2C error.
     */
    esp_err_t wait_until_ready();

    /**
     * @brief Read the factory coefficients needed by the compensation formulas.
     * @return ESP_OK when both calibration blocks were read, otherwise an I2C error.
     */
    esp_err_t load_calibration();

    /**
     * @brief Apply the BME280 datasheet formulas and public API units.
     * @param raw Uncompensated values from the sensor.
     * @param out Output populated with compensated values and acquisition time.
     */
    void convert(const Bme280RawSample& raw, Bme280Data& out) const;

    uint8_t address_;                          /*!< Address fixed by the product circuit. */
    Bme280Settings settings_;                  /*!< Controls reapplied after every recovery. */
    i2c_master_dev_handle_t device_ = nullptr; /*!< Handle retained across disconnects. */
    Bme280Calibration calibration_ = {};       /*!< Replaced after every successful reset. */
    bool ready_for_reads_ = false;             /*!< False requests full initialization on read. */
};

} // namespace redmole::environment
