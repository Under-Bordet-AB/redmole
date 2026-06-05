#include "environment_sensor.hpp"

namespace redmole::environment {
namespace {

constexpr int32_t kMinTemperatureDeciC = -1000;
constexpr int32_t kMaxTemperatureDeciC = 1000;
constexpr int32_t kMinHumidityDeciPct = 0;
constexpr int32_t kMaxHumidityDeciPct = 1000;
constexpr int32_t kMinPressureDeciHpa = 3000;
constexpr int32_t kMaxPressureDeciHpa = 12000;

bool make_signed(EnvironmentCapability capability,
                 int32_t value,
                 int32_t minimum,
                 int32_t maximum,
                 int64_t timestamp_ms,
                 EnvironmentMeasurement& out) {
    if ((value < minimum) || (value > maximum) || (timestamp_ms < 0)) {
        return false;
    }

    EnvironmentMeasurement measurement{};
    measurement.capability = capability;
    measurement.timestamp_ms = timestamp_ms;
    switch (capability) {
    case EnvironmentCapability::Temperature:
        measurement.value.temperature_deci_c = value;
        break;
    case EnvironmentCapability::Humidity:
        measurement.value.humidity_deci_pct = value;
        break;
    case EnvironmentCapability::Pressure:
        measurement.value.pressure_deci_hpa = value;
        break;
    default:
        return false;
    }

    out = measurement;
    return true;
}

} // namespace

bool make_temperature(int32_t value, int64_t timestamp_ms, EnvironmentMeasurement& out) {
    return make_signed(EnvironmentCapability::Temperature, value, kMinTemperatureDeciC,
                       kMaxTemperatureDeciC, timestamp_ms, out);
}

bool make_humidity(int32_t value, int64_t timestamp_ms, EnvironmentMeasurement& out) {
    return make_signed(EnvironmentCapability::Humidity, value, kMinHumidityDeciPct,
                       kMaxHumidityDeciPct, timestamp_ms, out);
}

bool make_pressure(int32_t value, int64_t timestamp_ms, EnvironmentMeasurement& out) {
    return make_signed(EnvironmentCapability::Pressure, value, kMinPressureDeciHpa,
                       kMaxPressureDeciHpa, timestamp_ms, out);
}

bool validate_measurement(const EnvironmentMeasurement& measurement) {
    EnvironmentMeasurement unused{};
    switch (measurement.capability) {
    case EnvironmentCapability::Temperature:
        return make_temperature(measurement.value.temperature_deci_c, measurement.timestamp_ms,
                                unused);
    case EnvironmentCapability::Humidity:
        return make_humidity(measurement.value.humidity_deci_pct, measurement.timestamp_ms, unused);
    case EnvironmentCapability::Pressure:
        return make_pressure(measurement.value.pressure_deci_hpa, measurement.timestamp_ms, unused);
    case EnvironmentCapability::CarbonDioxide:
        return (measurement.timestamp_ms >= 0) &&
               (measurement.value.carbon_dioxide_ppm <= 100000U);
    case EnvironmentCapability::VolatileOrganicCompounds:
        return (measurement.timestamp_ms >= 0) &&
               (measurement.value.volatile_organic_compounds_ppb <= 1000000U);
    case EnvironmentCapability::ParticulateMatter1:
        return (measurement.timestamp_ms >= 0) &&
               (measurement.value.particulate_matter_1_ug_m3 <= 100000U);
    case EnvironmentCapability::ParticulateMatter2_5:
        return (measurement.timestamp_ms >= 0) &&
               (measurement.value.particulate_matter_2_5_ug_m3 <= 100000U);
    case EnvironmentCapability::ParticulateMatter10:
        return (measurement.timestamp_ms >= 0) &&
               (measurement.value.particulate_matter_10_ug_m3 <= 100000U);
    case EnvironmentCapability::Illuminance:
        return (measurement.timestamp_ms >= 0) && (measurement.value.illuminance_lux <= 200000U);
    case EnvironmentCapability::SoundLevelDeciDbA:
        return (measurement.timestamp_ms >= 0) &&
               (measurement.value.sound_level_deci_dba <= 2000U);
    case EnvironmentCapability::RainfallSinceMidnight:
        return (measurement.timestamp_ms >= 0) &&
               (measurement.value.rainfall_since_midnight_deci_mm <= 100000U);
    case EnvironmentCapability::WindSpeed:
        return (measurement.timestamp_ms >= 0) &&
               (measurement.value.wind_speed_deci_m_s <= 10000U);
    case EnvironmentCapability::WindDirection:
        return (measurement.timestamp_ms >= 0) &&
               (measurement.value.wind_direction_degrees <= 359U);
    case EnvironmentCapability::UltravioletIndex:
        return (measurement.timestamp_ms >= 0) &&
               (measurement.value.ultraviolet_index_deci <= 300U);
    case EnvironmentCapability::SoilMoistureRelative:
        return (measurement.timestamp_ms >= 0) &&
               (measurement.value.soil_moisture_relative_deci_pct <= 1000U);
    case EnvironmentCapability::Count:
        return false;
    }

    return false;
}

void MeasurementBatch::clear() {
    count_ = 0U;
}

bool MeasurementBatch::report(const EnvironmentMeasurement& measurement) {
    if ((count_ >= measurements_.size()) || !validate_measurement(measurement)) {
        return false;
    }

    for (size_t index = 0U; index < count_; index++) {
        if (measurements_[index].capability == measurement.capability) {
            return false;
        }
    }

    measurements_[count_] = measurement;
    count_++;
    return true;
}

size_t MeasurementBatch::size() const {
    return count_;
}

const EnvironmentMeasurement& MeasurementBatch::operator[](size_t index) const {
    return measurements_[index];
}

} // namespace redmole::environment
