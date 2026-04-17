#ifndef BME280_HAL_BACKEND_H
#define BME280_HAL_BACKEND_H

#include "esp_err.h"
#include "bme280_hal.h"

esp_err_t bme280_hal_backend_init(bme280_hal* self);
esp_err_t bme280_hal_backend_read(bme280_hal* self, bme280_measurement* out);
void bme280_hal_backend_deinit(bme280_hal* self);

#endif // BME280_HAL_BACKEND_H
