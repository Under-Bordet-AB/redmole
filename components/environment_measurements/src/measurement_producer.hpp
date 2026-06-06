#pragma once

#include "esp_err.h"
#include "measurement_types.hpp"

namespace redmole::environment {

class MeasurementProducer {
  public:
    virtual ~MeasurementProducer() = default;

    virtual esp_err_t init() = 0;
    virtual esp_err_t read(MeasurementBatch& out) = 0;
};

} // namespace redmole::environment
