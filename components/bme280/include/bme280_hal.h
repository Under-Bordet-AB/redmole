#ifndef BME280_HAL_H
#define BME280_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

typedef struct {
    int64_t timestamp_ms;
    int32_t temperature_deci_c;
    int32_t humidity_deci_pct;
    int32_t pressure_deci_hpa;
} bme280_measurement;

typedef struct {
    bool initialized;
    bool hardware_present;
    const char* tag;
    struct {
        uint32_t sample_index;
        uint32_t period_ms;
    } backend;
} bme280_hal;

esp_err_t bme280_hal_init(bme280_hal* self);
esp_err_t bme280_hal_read(bme280_hal* self, bme280_measurement* out);
uint32_t bme280_hal_get_period_ms(const bme280_hal* self);
void bme280_hal_deinit(bme280_hal* self);

#endif // BME280_HAL_H
