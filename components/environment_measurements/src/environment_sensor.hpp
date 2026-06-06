#pragma once

/**
 * @file
 * @brief Common data contract implemented by environment sensor drivers.
 */

#include <cstdint>

#include "esp_err.h"

namespace redmole::environment {

constexpr int64_t kMicrosecondsPerMillisecond = 1000LL;

/**
 * @brief Values returned by one sensor read.
 *
 * A sensor sets the presence flag for every value it actually measured. This
 * lets simple sensors report one value without inventing the others.
 */
struct EnvironmentSensorReading {
    int64_t timestamp_ms;       /*!< Acquisition time in milliseconds since boot. */
    int32_t temperature_deci_c; /*!< Temperature in tenths of a degree Celsius. */
    int32_t humidity_deci_pct;  /*!< Relative humidity in tenths of a percent. */
    int32_t pressure_deci_hpa;  /*!< Pressure in tenths of a hectopascal. */
    bool has_temperature;       /*!< The temperature field contains a measurement. */
    bool has_humidity;          /*!< The humidity field contains a measurement. */
    bool has_pressure;          /*!< The pressure field contains a measurement. */
};

/**
 * @brief Hardware-facing contract for one environment sensor.
 *
 * Drivers only communicate with their device. Product details such as the
 * sensor's name and physical location belong to EnvironmentSensors.
 */
class EnvironmentSensor {
  public:
    virtual ~EnvironmentSensor() = default;

    /**
     * @brief Prepare the device for reads.
     *
     * A failed initialization may be retried later by read().
     *
     * @return ESP_OK when the device is ready, otherwise an ESP-IDF error code.
     */
    virtual esp_err_t init() = 0;

    /**
     * @brief Acquire the values this device can measure.
     *
     * @param reading Cleared output record populated by the driver.
     * @return ESP_OK when the acquisition completed, otherwise an ESP-IDF error code.
     */
    virtual esp_err_t read(EnvironmentSensorReading& reading) = 0;
};

} // namespace redmole::environment
