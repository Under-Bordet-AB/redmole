#include "bme280_hal.h"
#include <string.h>

#include "hal_internal/bme280_hal_backend.h"

esp_err_t bme280_hal_init(bme280_hal* self) {
    esp_err_t rv = ESP_OK;

    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(self, 0, sizeof(*self));
    self->initialized = true;
    self->tag = "BME280_HAL";
    rv = bme280_hal_backend_init(self);
    if (rv != ESP_OK) {
        self->initialized = false;
    }
    return rv;
}

esp_err_t bme280_hal_read(bme280_hal* self, bme280_measurement* out) {
    if (self == NULL || out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!self->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    return bme280_hal_backend_read(self, out);
}

uint32_t bme280_hal_get_period_ms(const bme280_hal* self) {
    if (self == NULL) {
        return 1000U;
    }

    if (self->backend.period_ms == 0U) {
        return 1000U;
    }

    return self->backend.period_ms;
}

void bme280_hal_deinit(bme280_hal* self) {
    if (self == NULL) {
        return;
    }

    bme280_hal_backend_deinit(self);
    self->initialized = false;
}
