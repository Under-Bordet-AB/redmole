#ifndef ENVIRONMENT_MEASUREMENTS_H
#define ENVIRONMENT_MEASUREMENTS_H

/**
 * @file
 * @brief Public API for board-local environment measurements.
 *
 * The module owns one configured sensor, its polling task, and the latest
 * coherent sample.
 */

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Latest preferred indoor temperature, humidity, and pressure values.
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
    bool valid;                 /*!< True only when all three values are present and fresh. */
} environment_measurement_sample_t;

/**
 * @brief Initialize the environment measurements module.
 *
 * Initializes fixed internal storage and attempts to initialize the configured
 * sensor. Sensor initialization errors are logged and retried by the polling
 * task. This function is idempotent; polling does not start until
 * environment_measurements_start().
 *
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t environment_measurements_init(void);

/**
 * @brief Start the environment measurement polling task.
 *
 * environment_measurements_init() must succeed first. The function is
 * idempotent and reuses the statically allocated task after a stop.
 *
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t environment_measurements_start(void);

/**
 * @brief Compatibility alias for environment_measurements_stop().
 *
 * Repeated calls are safe and permanent resources remain allocated.
 */
void environment_measurements_deinit(void);

/**
 * @brief Cooperatively stop environment polling.
 *
 * The call blocks until any active source operation finishes and the polling
 * task acknowledges the stop. Repeated calls are safe.
 */
void environment_measurements_stop(void);

/**
 * @brief Copy the latest stable environment measurement.
 *
 * The function briefly blocks on the latest-value mutex.
 *
 * @param out Caller-owned output sample, must not be NULL.
 * @return True when a fresh complete sample was copied, false otherwise.
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
 * @return Monotonic valid-publication counter, or 0 before initialization.
 */
uint32_t environment_measurements_get_update_count(void);

#ifdef __cplusplus
}
#endif

#endif // ENVIRONMENT_MEASUREMENTS_H
