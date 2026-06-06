#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "measurement_types.hpp"

namespace redmole::environment {

class MeasurementStore {
  public:
    esp_err_t init();
    esp_err_t publish_batch(const MeasurementBatch& batch, int64_t timestamp_ms);
    esp_err_t invalidate_channels(const MeasurementChannel* channels, size_t count);
    bool copy_channels(const MeasurementChannel* channels, size_t count,
                       StoredMeasurement* out) const;

  private:
    static bool batch_is_valid(const MeasurementBatch& batch);
    static bool channels_are_valid(const MeasurementChannel* channels, size_t count);

    mutable StaticSemaphore_t mutex_storage_ = {};
    mutable SemaphoreHandle_t mutex_ = nullptr;
    std::array<StoredMeasurement, measurement_channel_index(MeasurementChannel::Count)> latest_ =
        {};
    uint64_t next_publication_version_ = 1U;
};

} // namespace redmole::environment
