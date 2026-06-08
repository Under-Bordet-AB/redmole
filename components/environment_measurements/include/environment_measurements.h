#ifndef ENVIRONMENT_MEASUREMENTS_H
#define ENVIRONMENT_MEASUREMENTS_H

/**
 * @file
 * @brief Public API for board-local environment measurements.
 *
 * This module owns environment sensing as one responsibility: sensor polling,
 * latest sample storage, and freshness checks. Application code should use this
 * API instead of depending on individual sensor drivers.
 */

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Latest board-local environment measurement.
 *
 * Values use scaled integers:
 * - temperature_deci_c: 231 means 23.1 C
 * - humidity_deci_pct: 453 means 45.3 %
 * - pressure_deci_hpa: 10134 means 1013.4 hPa
 */
typedef struct {
    int64_t timestamp_ms;       /*!< Sample timestamp in milliseconds from esp_timer. */
    int32_t temperature_deci_c; /*!< Temperature in deci-degrees Celsius. */
    int32_t humidity_deci_pct;  /*!< Relative humidity in deci-percent. */
    int32_t pressure_deci_hpa;  /*!< Local pressure in deci-hectopascals. */
    bool valid;                 /*!< True when this sample contains publishable data. */
} environment_measurement_sample_t;

/**
 * @brief Initialize the environment measurements module.
 *
 * Initializes the shared board I2C bus, the selected BME280 backend, and the
 * internal latest-sample store. Polling does not start until
 * environment_measurements_start() is called.
 *
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t environment_measurements_init(void);

/**
 * @brief Start the environment measurement polling task.
 *
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t environment_measurements_start(void);

/**
 * @brief Stop polling and release owned sensor state.
 */
void environment_measurements_deinit(void);

/**
 * @brief Copy the latest stable environment measurement.
 *
 * @param out Output sample populated from the latest stable snapshot.
 * @return True when a valid sample has been published, false otherwise.
 */
bool environment_measurements_get_latest(environment_measurement_sample_t* out);

/**
 * @brief Check whether the latest valid sample is recent enough.
 *
 * @param max_age_ms Maximum accepted sample age in milliseconds.
 * @return True when the latest valid sample exists and is no older than max_age_ms.
 */
bool environment_measurements_is_fresh(uint32_t max_age_ms);

/**
 * @brief Return the number of samples published since initialization.
 *
 * @return Monotonic update counter, or 0 if the module is not initialized.
 */
uint32_t environment_measurements_get_update_count(void);

#ifdef __cplusplus
}
#endif

#endif // ENVIRONMENT_MEASUREMENTS_H
