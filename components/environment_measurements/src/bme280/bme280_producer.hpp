#pragma once

/**
 * @file
 * @brief Adapter from the BME280 driver to the generic producer interface.
 */

#include "bme280_sensor.hpp"
#include "measurement_producer.hpp"

namespace redmole::environment::bme280 {

/**
 * @brief Convert one complete BME280 reading into a logical measurement batch.
 *
 * The producer borrows the sensor and maps its physical values onto channels
 * selected by product composition.
 */
class Bme280Producer final : public MeasurementProducer {
  public:
    /**
     * @brief Construct a producer around a borrowed BME280 sensor.
     * @param sensor Sensor used for every acquisition.
     * @param temperature_channel Logical destination for temperature.
     * @param humidity_channel Logical destination for relative humidity.
     * @param pressure_channel Logical destination for pressure.
     */
    Bme280Producer(Bme280Sensor& sensor, MeasurementChannel temperature_channel,
                   MeasurementChannel humidity_channel, MeasurementChannel pressure_channel);

    /** @brief Initialize the borrowed BME280 sensor. */
    esp_err_t init() override;

    /** @brief Read and map one complete three-channel BME280 acquisition. */
    esp_err_t read(MeasurementBatch& out) override;

  private:
    Bme280Sensor& sensor_;                   /*!< Borrowed hardware driver. */
    MeasurementChannel temperature_channel_; /*!< Temperature batch destination. */
    MeasurementChannel humidity_channel_;    /*!< Humidity batch destination. */
    MeasurementChannel pressure_channel_;    /*!< Pressure batch destination. */
};

} // namespace redmole::environment::bme280
