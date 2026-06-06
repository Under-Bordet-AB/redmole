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
class SimulatedEnvironmentSensor final : public EnvironmentSensor {
  public:
    esp_err_t init() override;
    esp_err_t read(environment_measurement_sample_t& sample) override;

  private:
    uint32_t sample_index_ = 0U; /*!< Phase used to generate deterministic changing values. */
};

} // namespace redmole::environment
