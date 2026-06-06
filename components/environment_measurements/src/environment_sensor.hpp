#pragma once

#include "environment_measurements.h"

#include "esp_err.h"

namespace redmole::environment {

constexpr int64_t kMicrosecondsPerMillisecond = 1000LL;

class EnvironmentSensor {
  public:
    virtual ~EnvironmentSensor() = default;
    virtual esp_err_t init() = 0;
    virtual esp_err_t read(environment_measurement_sample_t& sample) = 0;
};

} // namespace redmole::environment
