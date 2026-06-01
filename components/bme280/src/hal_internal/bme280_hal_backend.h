#ifndef BME280_HAL_BACKEND_H
#define BME280_HAL_BACKEND_H

/**
 * @file
 * @brief Internal backend contract for the BME280 HAL.
 *
 * Exactly one backend implementation is selected at build time. Backend code
 * owns the opaque state stored in bme280_hal::backend_state.
 */

#include "esp_err.h"
#include "bme280_hal.h"

/**
 * @brief Initialize backend-owned state for a HAL context.
 *
 * Hardware backends may leave the HAL initialized but not hardware-present when
 * the physical sensor is missing.
 *
 * @param self HAL context.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t bme280_hal_backend_init(bme280_hal* self);

/**
 * @brief Read one measurement through the selected backend.
 *
 * @param self HAL context with backend state.
 * @param out Output measurement.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t bme280_hal_backend_read(bme280_hal* self, bme280_measurement* out);

/**
 * @brief Release backend-owned state for a HAL context.
 *
 * @param self HAL context.
 */
void bme280_hal_backend_deinit(bme280_hal* self);

#endif // BME280_HAL_BACKEND_H
