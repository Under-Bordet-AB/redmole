#include "environment_measurements.h"

#include <array>
#include <cstdint>
#include <limits>

#include "bme280/bme280_producer.hpp"
#include "bme280/bme280_sensor.hpp"
#include "esp_timer.h"
#include "measurement_store.hpp"
#include "measurement_types.hpp"
#include "measurements_manager.hpp"
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

using redmole::environment::MeasurementChannel;
using redmole::environment::MeasurementStore;
using redmole::environment::MeasurementsManager;
using redmole::environment::ProducerRegistration;
using redmole::environment::StoredMeasurement;
using redmole::environment::kMicrosecondsPerMillisecond;
using redmole::environment::bme280::Bme280Filter;
using redmole::environment::bme280::Bme280Mode;
using redmole::environment::bme280::Bme280Oversampling;
using redmole::environment::bme280::Bme280Producer;
using redmole::environment::bme280::Bme280Sensor;
using redmole::environment::bme280::Bme280Settings;
using redmole::environment::bme280::Bme280Standby;

constexpr int64_t kStaleTimeoutMs = 5000LL;
constexpr int64_t kMilliToDeci = 100LL;
constexpr int64_t kPascalsToDeciHectopascals = 10LL;

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

constexpr std::array<MeasurementChannel, 3> kIndoorBme280Channels = {
    MeasurementChannel::IndoorAmbientTemperature,
    MeasurementChannel::IndoorRelativeHumidity,
    MeasurementChannel::IndoorPressure,
};

int64_t now_ms() {
    return esp_timer_get_time() / kMicrosecondsPerMillisecond;
}

bool sample_is_fresh(const StoredMeasurement& sample, int64_t current_ms, int64_t max_age_ms) {
    return sample.valid && sample.timestamp_ms <= current_ms &&
           current_ms - sample.timestamp_ms <= max_age_ms;
}

bool convert_to_int32(int64_t value, int64_t divisor, int32_t& out) {
    int64_t converted = value / divisor;
    const int64_t remainder = value % divisor;
    if (remainder >= divisor / 2LL) {
        converted++;
    } else if (remainder <= -(divisor / 2LL)) {
        converted--;
    }

    if (converted < std::numeric_limits<int32_t>::min() ||
        converted > std::numeric_limits<int32_t>::max()) {
        return false;
    }

    out = static_cast<int32_t>(converted);
    return true;
}

Bme280Sensor s_indoor_sensor(kIndoorBme280Address, kIndoorBme280Settings);
Bme280Producer s_indoor_producer(s_indoor_sensor, MeasurementChannel::IndoorAmbientTemperature,
                                 MeasurementChannel::IndoorRelativeHumidity,
                                 MeasurementChannel::IndoorPressure);
MeasurementStore s_measurement_store;
const std::array<ProducerRegistration, 1> s_producers = {{
    {
        "indoor BME280",
        s_indoor_producer,
        kIndoorBme280Channels.data(),
        kIndoorBme280Channels.size(),
    },
}};
MeasurementsManager s_measurements_manager(s_producers.data(), s_producers.size(),
                                           s_measurement_store, now_ms);

bool copy_indoor_measurements(std::array<StoredMeasurement, 3>& out) {
    return s_measurement_store.copy_channels(kIndoorBme280Channels.data(),
                                             kIndoorBme280Channels.size(), out.data());
}

} // namespace

extern "C" esp_err_t environment_measurements_init(void) {
    return s_measurements_manager.init();
}

extern "C" esp_err_t environment_measurements_start(void) {
    return s_measurements_manager.start();
}

extern "C" void environment_measurements_stop(void) {
    s_measurements_manager.stop();
}

extern "C" void environment_measurements_deinit(void) {
    s_measurements_manager.stop();
}

extern "C" bool environment_measurements_get_latest(environment_measurement_sample_t* out) {
    if (out == nullptr) {
        return false;
    }

    std::array<StoredMeasurement, 3> stored = {};
    if (!copy_indoor_measurements(stored)) {
        return false;
    }

    const int64_t current_ms = now_ms();
    for (const StoredMeasurement& sample : stored) {
        if (!sample_is_fresh(sample, current_ms, kStaleTimeoutMs)) {
            out->valid = false;
            return false;
        }
    }

    out->timestamp_ms = stored[0].timestamp_ms;
    for (const StoredMeasurement& sample : stored) {
        if (sample.timestamp_ms < out->timestamp_ms) {
            out->timestamp_ms = sample.timestamp_ms;
        }
    }

    if (!convert_to_int32(stored[0].value, kMilliToDeci, out->temperature_deci_c) ||
        !convert_to_int32(stored[1].value, kMilliToDeci, out->humidity_deci_pct) ||
        !convert_to_int32(stored[2].value, kPascalsToDeciHectopascals,
                          out->pressure_deci_hpa)) {
        out->valid = false;
        return false;
    }

    out->valid = true;
    return true;
}

extern "C" bool environment_measurements_is_fresh(uint32_t max_age_ms) {
    std::array<StoredMeasurement, 3> stored = {};
    if (!copy_indoor_measurements(stored)) {
        return false;
    }

    const int64_t current_ms = now_ms();
    for (const StoredMeasurement& sample : stored) {
        if (!sample_is_fresh(sample, current_ms, static_cast<int64_t>(max_age_ms))) {
            return false;
        }
    }
    return true;
}

extern "C" uint32_t environment_measurements_get_update_count(void) {
    std::array<StoredMeasurement, 3> stored = {};
    if (!copy_indoor_measurements(stored)) {
        return 0U;
    }

    uint64_t publication_version = 0U;
    for (const StoredMeasurement& sample : stored) {
        if (sample.publication_version > publication_version) {
            publication_version = sample.publication_version;
        }
    }
    return static_cast<uint32_t>(publication_version);
}
