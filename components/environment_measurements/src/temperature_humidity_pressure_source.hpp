#pragma once

/**
 * @file
 * @brief Exact source contract for a complete temperature, humidity, and pressure output.
 */

#include "esp_err.h"
#include "reading_types.hpp"

namespace redmole::environment {

/** @brief Complete values promised by a temperature, humidity, and pressure source. */
struct TemperatureHumidityPressureReading {
    Temperature temperature;
    Humidity humidity;
    Pressure pressure;
};

/** @brief Injectable source for one complete temperature, humidity, and pressure output. */
class TemperatureHumidityPressureSource {
  public:
    virtual ~TemperatureHumidityPressureSource() = default;

    virtual esp_err_t init() = 0;
    virtual esp_err_t read(TemperatureHumidityPressureReading& out) = 0;
};

} // namespace redmole::environment
