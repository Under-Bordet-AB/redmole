#pragma once

/**
 * @file
 * @brief Allocation-free simulated environmental source.
 */

#include <cstdint>

#include "environment_sensor.hpp"

namespace redmole::environment {

/**
 * @brief Deterministic source used by builds that deliberately disable hardware.
 */
class SimulatedEnvironmentSensor final : public EnvironmentSensor {
  public:
    /**
     * @brief Reset the simulation to its first sample.
     * @return ESP_OK.
     */
    esp_err_t init() override;

    /**
     * @brief Produce one complete, slowly changing environment reading.
     * @param reading Cleared output record populated with simulated values.
     * @return ESP_OK.
     */
    esp_err_t read(EnvironmentSensorReading& reading) override;

  private:
    uint32_t sample_index_ = 0U; /*!< Position in the deterministic waveform. */
};

} // namespace redmole::environment
