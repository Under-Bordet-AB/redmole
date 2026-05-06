#ifndef BME280_HAL_H
#define BME280_HAL_H

/**
 * @file
 * @brief Public BME280 environmental sensor HAL API.
 *
 * The HAL hides whether samples come from the hardware backend or simulator
 * backend. Callers own the context but must treat backend_state as private.
 */

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/** Default polling period when a backend does not provide one. */
#define BME280_HAL_DEFAULT_PERIOD_MS 1000U

/**
 * @brief One compensated BME280 environmental measurement.
 *
 * Values use scaled integers to avoid floating-point ownership in higher
 * layers:
 * - temperature_deci_c: 231 means 23.1 C
 * - humidity_deci_pct: 453 means 45.3 %
 * - pressure_deci_hpa: 10134 means 1013.4 hPa
 */
typedef struct {
    int64_t timestamp_ms;       /*!< Measurement timestamp in milliseconds from esp_timer. */
    int32_t temperature_deci_c; /*!< Temperature in deci-degrees Celsius. */
    int32_t humidity_deci_pct;  /*!< Relative humidity in deci-percent. */
    int32_t pressure_deci_hpa;  /*!< Local pressure in deci-hectopascals. */
} bme280_measurement;

/**
 * @brief Caller-owned BME280 HAL context.
 *
 * The public HAL is stable across simulator and hardware backends. Backend
 * details are stored behind backend_state and must only be accessed by the
 * backend implementation.
 */
typedef struct {
    bool initialized;      /*!< True after bme280_hal_init() succeeds. */
    bool hardware_present; /*!< True when the hardware backend has verified a physical BME280. */
    const char* tag;       /*!< Logging tag used by backend code. */
    uint32_t period_ms;    /*!< Recommended polling period in milliseconds. */
    void* backend_state;   /*!< Private backend-owned state. */
} bme280_hal;

/**
 * @brief Initialize a BME280 HAL context.
 *
 * In hardware mode this probes/configures the physical sensor when available,
 * but absence is not fatal; later reads may initialize the sensor after it is
 * plugged in.
 *
 * @param self Context to initialize.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t bme280_hal_init(bme280_hal* self);

/**
 * @brief Read one compensated BME280 measurement.
 *
 * In hardware mode this verifies the device, reads raw registers, applies
 * compensation, and returns scaled integer values. In simulator mode it returns
 * generated sample data.
 *
 * @param self Initialized HAL context.
 * @param out Output measurement.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t bme280_hal_read(bme280_hal* self, bme280_measurement* out);

/**
 * @brief Return the backend-recommended polling period.
 *
 * @param self HAL context, or NULL for the default period.
 * @return Polling period in milliseconds.
 */
uint32_t bme280_hal_get_period_ms(const bme280_hal* self);

/**
 * @brief Deinitialize a BME280 HAL context.
 *
 * Releases backend-owned state and marks the context uninitialized.
 *
 * @param self Context to deinitialize.
 */
void bme280_hal_deinit(bme280_hal* self);

#endif // BME280_HAL_H
