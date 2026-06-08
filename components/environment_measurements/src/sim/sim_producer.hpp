#pragma once

/**
 * @file
 * @brief Deterministic hardware-free environment measurement producer.
 */

#include <cstdint>

#include "measurement_producer.hpp"

namespace redmole::environment::sim {

/**
 * @brief Generate a repeating ramp of indoor environment measurements.
 *
 * This producer follows the same contract as hardware producers, making it
 * useful for GUI development and manual testing without an attached BME280.
 */
class SimProducer final : public MeasurementProducer {
  public:
    /**
     * @brief Construct a simulator that writes to three logical channels.
     * @param temperature_channel Logical destination for temperature.
     * @param humidity_channel Logical destination for relative humidity.
     * @param pressure_channel Logical destination for pressure.
     */
    SimProducer(MeasurementChannel temperature_channel, MeasurementChannel humidity_channel,
                MeasurementChannel pressure_channel);

    /** @brief Reset the deterministic ramp to its first sample. */
    esp_err_t init() override;

    /** @brief Populate one complete three-channel simulated batch. */
    esp_err_t read(MeasurementBatch& out) override;

  private:
    MeasurementChannel temperature_channel_; /*!< Temperature batch destination. */
    MeasurementChannel humidity_channel_;    /*!< Humidity batch destination. */
    MeasurementChannel pressure_channel_;    /*!< Pressure batch destination. */
    uint32_t sample_index_ = 0U;             /*!< Selects the next position in the ramp. */
};

} // namespace redmole::environment::sim
