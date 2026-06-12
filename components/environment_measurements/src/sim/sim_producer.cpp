#include "sim_producer.hpp"

/**
 * @file
 * @brief Implementation of deterministic simulated environment measurements.
 */

namespace redmole::environment::sim {
namespace {

// The simulator walks from -10 through +10, then repeats.
constexpr uint32_t kRampStepCount = 21U;
constexpr int64_t kRampCenter = 10LL;

// Base values use the same canonical units as real producer output.
constexpr int64_t kBaseTemperatureMilliC = 23000LL;
constexpr int64_t kBaseHumidityMilliPct = 45000LL;
constexpr int64_t kBasePressurePa = 101325LL;
} // namespace

SimProducer::SimProducer(MeasurementChannel temperature_channel,
                         MeasurementChannel humidity_channel, MeasurementChannel pressure_channel)
    : temperature_channel_(temperature_channel), humidity_channel_(humidity_channel),
      pressure_channel_(pressure_channel) {
}

esp_err_t SimProducer::init() {
    sample_index_ = 0U;
    return ESP_OK;
}

esp_err_t SimProducer::read(MeasurementBatch& out) {
    // Modulo keeps the ramp bounded even after sample_index_ wraps naturally.
    const int64_t offset = static_cast<int64_t>(sample_index_ % kRampStepCount) - kRampCenter;

    // Return a complete batch so simulator behavior matches the real BME280 producer.
    out.measurements[0] = {
        temperature_channel_,
        kBaseTemperatureMilliC + offset * 100LL,
    };
    out.measurements[1] = {
        humidity_channel_,
        kBaseHumidityMilliPct + offset * 200LL,
    };
    out.measurements[2] = {
        pressure_channel_,
        kBasePressurePa + offset * 15LL,
    };
    out.count = 3U;

    sample_index_++;
    return ESP_OK;
}

} // namespace redmole::environment::sim
