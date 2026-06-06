#include "environment_measurements.h"

#include "bme280/bme280_sensor.hpp"
#include "environment_measurements_internal.hpp"
#include "esp_timer.h"
#include "reading_types.hpp"
#include "sdkconfig.h"

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

namespace {

using redmole::environment::EnvironmentMeasurements;
using redmole::environment::kMicrosecondsPerMillisecond;
using redmole::environment::bme280::Bme280Filter;
using redmole::environment::bme280::Bme280Mode;
using redmole::environment::bme280::Bme280Oversampling;
using redmole::environment::bme280::Bme280Sensor;
using redmole::environment::bme280::Bme280Settings;
using redmole::environment::bme280::Bme280Standby;

constexpr const char* kIndoorSensorName = "indoor BME280";

#if CONFIG_REDMOLE_BME280_ADDRESS_0X76
constexpr uint8_t kIndoorBme280Address = 0x76U;
#else
constexpr uint8_t kIndoorBme280Address = 0x77U;
#endif

constexpr Bme280Settings kIndoorBme280Settings = {
    static_cast<Bme280Mode>(CONFIG_REDMOLE_BME280_MODE),
    static_cast<Bme280Oversampling>(CONFIG_REDMOLE_BME280_OVERSAMPLING_TEMPERATURE),
    static_cast<Bme280Oversampling>(CONFIG_REDMOLE_BME280_OVERSAMPLING_PRESSURE),
    static_cast<Bme280Oversampling>(CONFIG_REDMOLE_BME280_OVERSAMPLING_HUMIDITY),
    static_cast<Bme280Filter>(CONFIG_REDMOLE_BME280_IIR_FILTER),
    static_cast<Bme280Standby>(CONFIG_REDMOLE_BME280_STANDBY_TIME),
};

int64_t now_ms() {
    return esp_timer_get_time() / kMicrosecondsPerMillisecond;
}

Bme280Sensor s_indoor_sensor(kIndoorBme280Address, kIndoorBme280Settings);
EnvironmentMeasurements s_environment_measurements(kIndoorSensorName, s_indoor_sensor, now_ms);

} // namespace

extern "C" esp_err_t environment_measurements_init(void) {
    return s_environment_measurements.init();
}

extern "C" esp_err_t environment_measurements_start(void) {
    return s_environment_measurements.start();
}

extern "C" void environment_measurements_stop(void) {
    s_environment_measurements.stop();
}

extern "C" void environment_measurements_deinit(void) {
    s_environment_measurements.stop();
}

extern "C" bool environment_measurements_get_latest(environment_measurement_sample_t* out) {
    return s_environment_measurements.get_latest(out);
}

extern "C" bool environment_measurements_is_fresh(uint32_t max_age_ms) {
    return s_environment_measurements.is_fresh(max_age_ms);
}

extern "C" uint32_t environment_measurements_get_update_count(void) {
    return s_environment_measurements.get_update_count();
}
