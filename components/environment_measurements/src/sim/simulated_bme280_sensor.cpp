#include "sim/simulated_bme280_sensor.hpp"

#include "esp_timer.h"

namespace redmole::environment {
namespace {

static int32_t triangle_wave(uint32_t phase, int32_t midpoint, int32_t amplitude) {
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

esp_err_t SimulatedBme280Sensor::init() {
    sample_index_ = 0U;
    return ESP_OK;
}

bool SimulatedBme280Sensor::probe() {
    return true;
}

esp_err_t SimulatedBme280Sensor::read(environment_measurement_sample_t& out) {
    out.timestamp_ms = esp_timer_get_time() / kUsPerMs;
    out.temperature_deci_c = triangle_wave(sample_index_, 225, 22);
    out.humidity_deci_pct = triangle_wave(sample_index_ + 11U, 470, 80);
    out.pressure_deci_hpa = triangle_wave(sample_index_ + 23U, 10120, 65);
    out.valid = true;
    sample_index_++;
    return ESP_OK;
}

SensorLocation SimulatedBme280Sensor::location() const {
    return SensorLocation::Inside;
}

bool SimulatedBme280Sensor::is_simulated() const {
    return true;
}

} // namespace redmole::environment
