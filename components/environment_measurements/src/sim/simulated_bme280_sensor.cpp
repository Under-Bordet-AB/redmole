#include "sim/simulated_bme280_sensor.hpp"

#include "esp_timer.h"

namespace redmole::environment {
namespace {

int32_t triangle_wave(uint32_t phase, int32_t midpoint, int32_t amplitude) {
    uint32_t segment;
    int32_t offset;

    segment = phase % 40U;
    if (segment < 10U) {
        offset = (static_cast<int32_t>(segment) * amplitude) / 10;
        return midpoint + offset;
    }

    if (segment < 20U) {
        offset = (static_cast<int32_t>(20U - segment) * amplitude) / 10;
        return midpoint + offset;
    }

    if (segment < 30U) {
        offset = -((static_cast<int32_t>(segment) - 20) * amplitude) / 10;
        return midpoint + offset;
    }

    offset = -((static_cast<int32_t>(40U - segment)) * amplitude) / 10;
    return midpoint + offset;
}

} // namespace

esp_err_t SimulatedEnvironmentSensor::init() {
    sample_index_ = 0U;
    return ESP_OK;
}

esp_err_t SimulatedEnvironmentSensor::read(EnvironmentSensorReading& reading) {
    reading.timestamp_ms = esp_timer_get_time() / kMicrosecondsPerMillisecond;
    reading.temperature_deci_c = triangle_wave(sample_index_, 225, 22);
    reading.humidity_deci_pct = triangle_wave(sample_index_ + 11U, 470, 80);
    reading.pressure_deci_hpa = triangle_wave(sample_index_ + 23U, 10120, 65);
    reading.has_temperature = true;
    reading.has_humidity = true;
    reading.has_pressure = true;
    sample_index_++;
    return ESP_OK;
}

} // namespace redmole::environment
