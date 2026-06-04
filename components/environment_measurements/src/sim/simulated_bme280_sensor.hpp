#pragma once

#include <cstdint>

#include "environment_sensor.hpp"

namespace redmole::environment {

class SimulatedBme280Sensor final : public EnvironmentSensor {
public:
    esp_err_t init() override;
    bool probe() override;
    esp_err_t read(environment_measurement_sample_t& out) override;
    SensorLocation location() const override;
    bool is_simulated() const override;

private:
    uint32_t sample_index_ = 0U;
};

} // namespace redmole::environment
