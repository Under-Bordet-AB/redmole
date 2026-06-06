#include "sim/simulated_bme280_sensor.hpp"

#include "esp_timer.h"

namespace redmole::environment {
namespace {

int32_t triangle_wave(uint32_t phase, int32_t midpoint, int32_t amplitude) {
    const uint32_t segment = phase % 40U;
    int32_t offset = 0;

    if (segment < 10U) {
        offset = (static_cast<int32_t>(segment) * amplitude) / 10;
    } else if (segment < 20U) {
        offset = (static_cast<int32_t>(20U - segment) * amplitude) / 10;
    } else if (segment < 30U) {
        offset = -((static_cast<int32_t>(segment) - 20) * amplitude) / 10;
    } else {
        offset = -((static_cast<int32_t>(40U - segment)) * amplitude) / 10;
    }

    return midpoint + offset;
}

} // namespace

esp_err_t SimulatedEnvironmentSensor::init() {
    sample_index_ = 0U;
    return ESP_OK;
}

esp_err_t SimulatedEnvironmentSensor::read(environment_measurement_sample_t& sample) {
    sample.timestamp_ms = esp_timer_get_time() / kMicrosecondsPerMillisecond;
    sample.temperature_deci_c = triangle_wave(sample_index_, 225, 22);
    sample.humidity_deci_pct = triangle_wave(sample_index_ + 11U, 470, 80);
    sample.pressure_deci_hpa = triangle_wave(sample_index_ + 23U, 10120, 65);
    sample.valid = true;
    sample_index_++;
    return ESP_OK;
}

} // namespace redmole::environment
