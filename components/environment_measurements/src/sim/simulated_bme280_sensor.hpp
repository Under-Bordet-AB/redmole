#pragma once

/**
 * @file
 * @brief Allocation-free simulated environmental source.
 */

#include <cstdint>

#include "environment_sensor.hpp"

namespace redmole::environment {

/**
 * @brief Process-lifetime fallback source producing plausible changing values.
 */
class SimulatedEnvironmentSource final : public EnvironmentSource {
  public:
    SimulatedEnvironmentSource() = default;
    SimulatedEnvironmentSource(const SimulatedEnvironmentSource&) = delete;
    SimulatedEnvironmentSource& operator=(const SimulatedEnvironmentSource&) = delete;
    SimulatedEnvironmentSource(SimulatedEnvironmentSource&&) = delete;
    SimulatedEnvironmentSource& operator=(SimulatedEnvironmentSource&&) = delete;

    /**
     * @brief Reset the deterministic simulated sample sequence.
     * @return ESP_OK after resetting the deterministic sample sequence.
     */
    esp_err_t init() override;
    /**
     * @brief Check availability of the compiled-in simulator.
     * @return Always true because the compiled-in simulator is available.
     */
    bool probe() override;
    /**
     * @brief Activate the simulator without hardware configuration.
     * @return Always ESP_OK because no hardware configuration is required.
     */
    esp_err_t activate() override;

    /**
     * @brief Report one plausible changing temperature, humidity, and pressure batch.
     * @param batch Empty manager-owned batch receiving copied measurements.
     * @return ESP_OK on success, otherwise an ESP-IDF error code.
     */
    esp_err_t poll(MeasurementBatch& batch) override;

  private:
    uint32_t sample_index_ = 0U; /*!< Phase used to generate deterministic changing values. */
};

} // namespace redmole::environment
