#pragma once

/**
 * @file
 * @brief Fixed product composition for environment sensors.
 */

#include <array>

#include "bme280/bme280_sensor.hpp"
#include "environment_sensor.hpp"
#include "sdkconfig.h"
#include "sim/simulated_bme280_sensor.hpp"

namespace redmole::environment {

/** @brief Physical area represented by a sensor's measurements. */
enum class EnvironmentLocation {
    Indoor,  /*!< Measurements describe the indoor environment. */
    Outdoor, /*!< Measurements describe the outdoor environment. */
};

/**
 * @brief Product details and runtime health for one owned sensor.
 *
 * The name is used in logs. The location decides where successful readings
 * are published. last_read_failed suppresses repeated error logs.
 */
struct EnvironmentSensorBinding {
    const char* name;             /*!< Stable human-readable name used in logs. */
    EnvironmentLocation location; /*!< Product meaning attached outside the driver. */
    EnvironmentSensor* sensor;    /*!< Sensor owned by EnvironmentSensors. */
    bool last_read_failed;        /*!< Prevents the same read error from flooding logs. */
};

/**
 * @brief Own the sensors installed in this product and describe their purpose.
 *
 * Adding hardware is intentionally explicit: add the concrete object and its
 * binding here so the software composition matches the circuit.
 */
class EnvironmentSensors {
  public:
    /** @brief Fixed-size list that makes the installed sensor count explicit. */
    using Bindings = std::array<EnvironmentSensorBinding, 1>;

    /** @brief Construct the fixed sensor list selected by the build configuration. */
    EnvironmentSensors();

    /**
     * @brief Access the fixed sensor list used during initialization and polling.
     * @return Mutable bindings owned for the lifetime of the application.
     */
    Bindings& bindings();

  private:
#if CONFIG_REDMOLE_ENVIRONMENT_SOURCE_SIMULATOR
    SimulatedEnvironmentSensor indoor_sensor_; /*!< Explicit substitute for hardware builds. */
#else
    Bme280Sensor indoor_sensor_; /*!< Sensor wired to the configured indoor address. */
#endif

    Bindings bindings_; /*!< Product meaning and health attached to each owned driver. */
};

} // namespace redmole::environment
