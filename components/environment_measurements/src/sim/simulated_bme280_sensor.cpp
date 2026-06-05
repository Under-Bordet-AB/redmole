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

esp_err_t SimulatedEnvironmentSource::init() {
    sample_index_ = 0U;
    return ESP_OK;
}

bool SimulatedEnvironmentSource::probe() {
    return true;
}

esp_err_t SimulatedEnvironmentSource::activate() {
    return ESP_OK;
}

esp_err_t SimulatedEnvironmentSource::poll(MeasurementBatch& batch) {
    const int64_t timestamp_ms = esp_timer_get_time() / kUsPerMs;
    EnvironmentMeasurement temperature{};
    EnvironmentMeasurement humidity{};
    EnvironmentMeasurement pressure{};

    if (!make_temperature(triangle_wave(sample_index_, 225, 22), timestamp_ms, temperature) ||
        !make_humidity(triangle_wave(sample_index_ + 11U, 470, 80), timestamp_ms, humidity) ||
        !make_pressure(triangle_wave(sample_index_ + 23U, 10120, 65), timestamp_ms, pressure) ||
        !batch.report(temperature) || !batch.report(humidity) || !batch.report(pressure)) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    sample_index_++;
    return ESP_OK;
}

} // namespace redmole::environment
