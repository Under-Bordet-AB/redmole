#pragma once

/**
 * @file
 * @brief Interface implemented by every environment measurement source.
 */

#include "esp_err.h"
#include "measurement_types.hpp"

namespace redmole::environment {

/**
 * @brief Synchronous source of one or more logical environment measurements.
 *
 * The manager owns scheduling and storage. A producer only initializes its
 * source and performs bounded, synchronous reads into caller-owned batches.
 */
class MeasurementProducer {
  public:
    /** @brief Allow concrete producers to clean up through an interface pointer. */
    virtual ~MeasurementProducer() = default;

    /**
     * @brief Prepare the source for reads.
     * @return ESP_OK when ready, otherwise an ESP-IDF error code.
     */
    virtual esp_err_t init() = 0;

    /**
     * @brief Perform one acquisition and populate a caller-owned batch.
     * @param out Batch to populate; successful reads must set a non-zero count.
     * @return ESP_OK for a complete batch, otherwise an ESP-IDF error code.
     */
    virtual esp_err_t read(MeasurementBatch& out) = 0;
};

} // namespace redmole::environment
