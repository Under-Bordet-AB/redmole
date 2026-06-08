#include "bme280_producer.hpp"

/**
 * @file
 * @brief Implementation of BME280-to-logical-channel batch mapping.
 */

namespace redmole::environment::bme280 {

Bme280Producer::Bme280Producer(Bme280Sensor& sensor, MeasurementChannel temperature_channel,
                               MeasurementChannel humidity_channel,
                               MeasurementChannel pressure_channel)
    : sensor_(sensor), temperature_channel_(temperature_channel),
      humidity_channel_(humidity_channel), pressure_channel_(pressure_channel) {
}

esp_err_t Bme280Producer::init() {
    return sensor_.init();
}

esp_err_t Bme280Producer::read(MeasurementBatch& out) {
    Bme280Reading reading = {};
    const esp_err_t result = sensor_.read(reading);
    if (result != ESP_OK) {
        // A failed read must never leave a previous batch looking publishable.
        out.count = 0U;
        return result;
    }

    // Preserve the coherent hardware acquisition by returning all three values together.
    out.measurements[0] = {
        temperature_channel_,
        reading.temperature.milli_c,
    };
    out.measurements[1] = {
        humidity_channel_,
        reading.humidity.milli_pct,
    };
    out.measurements[2] = {
        pressure_channel_,
        reading.pressure.pa,
    };
    out.count = 3U;
    return ESP_OK;
}

} // namespace redmole::environment::bme280
