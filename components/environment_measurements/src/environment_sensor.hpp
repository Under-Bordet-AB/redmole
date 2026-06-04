#pragma once

#include <cstdint>

#include "environment_measurements.h"
#include "esp_err.h"

namespace redmole::environment {

constexpr int64_t kUsPerMs = 1000LL;

enum class SensorLocation : uint8_t {
    Inside,
};

class EnvironmentSensor {
public:
    EnvironmentSensor(const EnvironmentSensor&) = delete;
    EnvironmentSensor& operator=(const EnvironmentSensor&) = delete;
    EnvironmentSensor(EnvironmentSensor&&) = delete;
    EnvironmentSensor& operator=(EnvironmentSensor&&) = delete;
    virtual ~EnvironmentSensor() = default;
    virtual esp_err_t init() = 0;
    virtual bool probe() = 0;
    virtual esp_err_t read(environment_measurement_sample_t& out) = 0;
    virtual SensorLocation location() const = 0;
    virtual bool is_simulated() const = 0;

protected:
    EnvironmentSensor() = default;
};

} // namespace redmole::environment
