#include "environment_sensors.hpp"

#include "sdkconfig.h"

namespace redmole::environment {
namespace {

#if !CONFIG_REDMOLE_ENVIRONMENT_SOURCE_SIMULATOR && CONFIG_REDMOLE_BME280_ADDRESS_0X76
constexpr uint8_t kIndoorBme280Address = 0x76U;
#elif !CONFIG_REDMOLE_ENVIRONMENT_SOURCE_SIMULATOR
constexpr uint8_t kIndoorBme280Address = 0x77U;
#endif

#if !CONFIG_REDMOLE_ENVIRONMENT_SOURCE_SIMULATOR
constexpr Bme280Settings kIndoorBme280Settings = {
    static_cast<Bme280Mode>(CONFIG_REDMOLE_BME280_MODE),
    static_cast<Bme280Oversampling>(CONFIG_REDMOLE_BME280_OVERSAMPLING_TEMPERATURE),
    static_cast<Bme280Oversampling>(CONFIG_REDMOLE_BME280_OVERSAMPLING_PRESSURE),
    static_cast<Bme280Oversampling>(CONFIG_REDMOLE_BME280_OVERSAMPLING_HUMIDITY),
    static_cast<Bme280Filter>(CONFIG_REDMOLE_BME280_IIR_FILTER),
    static_cast<Bme280Standby>(CONFIG_REDMOLE_BME280_STANDBY_TIME),
};
#endif

} // namespace

#if CONFIG_REDMOLE_ENVIRONMENT_SOURCE_SIMULATOR
EnvironmentSensors::EnvironmentSensors()
    : bindings_{{
          {"simulated indoor sensor", EnvironmentLocation::Indoor, &indoor_sensor_, false},
      }} {
}
#else
EnvironmentSensors::EnvironmentSensors()
    : indoor_sensor_(kIndoorBme280Address, kIndoorBme280Settings),
      bindings_{{
          {"indoor BME280", EnvironmentLocation::Indoor, &indoor_sensor_, false},
      }} {
}
#endif

EnvironmentSensors::Bindings& EnvironmentSensors::bindings() {
    return bindings_;
}

} // namespace redmole::environment
