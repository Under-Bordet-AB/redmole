#pragma once

/**
 * @file
 * @brief Mutex-protected latest-value storage for logical measurements.
 */

#include <array>
#include <cstddef>
#include <cstdint>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "measurement_types.hpp"

namespace redmole::environment {

/**
 * @brief Store the latest record for every logical measurement channel.
 *
 * Each public operation takes the mutex once, so a complete producer batch is
 * published atomically and a multi-channel reader sees one coherent snapshot.
 * The object uses static FreeRTOS mutex storage and performs no heap allocation.
 */
class MeasurementStore {
  public:
    /** @brief Create the store mutex; repeated calls are safe. */
    esp_err_t init();

    /**
     * @brief Atomically publish every measurement in a validated batch.
     * @param batch Non-empty batch containing unique, valid channels.
     * @param timestamp_ms Publication timestamp applied to every batch entry.
     * @return ESP_OK on success, otherwise ESP_ERR_INVALID_STATE or ESP_ERR_INVALID_ARG.
     */
    esp_err_t publish_batch(const MeasurementBatch& batch, int64_t timestamp_ms);

    /**
     * @brief Atomically mark a list of channels invalid after a producer failure.
     * @param channels Non-null array of valid channels.
     * @param count Number of entries in channels; must be greater than zero.
     * @return ESP_OK on success, otherwise ESP_ERR_INVALID_STATE or ESP_ERR_INVALID_ARG.
     */
    esp_err_t invalidate_channels(const MeasurementChannel* channels, size_t count);

    /**
     * @brief Atomically copy the latest records for a list of channels.
     * @param channels Non-null array of valid channels.
     * @param count Number of requested channels; must be greater than zero.
     * @param out Caller-owned array with room for count records; must not be null.
     * @return True when the requested records were copied, false for invalid state or arguments.
     */
    bool copy_channels(const MeasurementChannel* channels, size_t count,
                       MeasurementRecord* out) const;

  private:
    static bool batch_is_valid(const MeasurementBatch& batch);
    static bool channels_are_valid(const MeasurementChannel* channels, size_t count);

    mutable StaticSemaphore_t mutex_storage_ = {}; /*!< FreeRTOS-owned storage behind mutex_. */
    mutable SemaphoreHandle_t mutex_ = nullptr;    /*!< Guards latest_ and version updates. */
    std::array<MeasurementRecord, measurement_channel_index(MeasurementChannel::Count)> latest_ =
        {};                                  /*!< One latest record per logical channel. */
    uint64_t next_publication_version_ = 1U; /*!< Version assigned to the next valid batch. */
};

} // namespace redmole::environment
