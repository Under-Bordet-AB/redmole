#include "hal_internal/bme280_hal_backend.h"

#include <string.h>

#include "esp_timer.h"

static int32_t triangle_wave(uint32_t phase, int32_t midpoint, int32_t amplitude) {
    uint32_t segment = phase % 40U;
    int32_t offset = 0;

    if (segment < 10U) {
        offset = ((int32_t)segment * amplitude) / 10;
    } else if (segment < 20U) {
        offset = ((int32_t)(20U - segment) * amplitude) / 10;
    } else if (segment < 30U) {
        offset = -(((int32_t)segment - 20) * amplitude) / 10;
    } else {
        offset = -(((int32_t)(40U - segment)) * amplitude) / 10;
    }

    return midpoint + offset;
}

esp_err_t bme280_hal_backend_init(bme280_hal* self) {
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(&self->backend, 0, sizeof(self->backend));
    self->hardware_present = false;
    self->backend.period_ms = 1000U;
    return ESP_OK;
}

esp_err_t bme280_hal_backend_read(bme280_hal* self, bme280_measurement* out) {
    if ((self == NULL) || (out == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    out->timestamp_ms = esp_timer_get_time() / 1000LL;
    out->temperature_deci_c = triangle_wave(self->backend.sample_index, 225, 22);
    out->humidity_deci_pct = triangle_wave(self->backend.sample_index + 11U, 470, 80);
    out->pressure_deci_hpa = triangle_wave(self->backend.sample_index + 23U, 10120, 65);

    self->backend.sample_index++;
    return ESP_OK;
}

void bme280_hal_backend_deinit(bme280_hal* self) {
    if (self == NULL) {
        return;
    }

    memset(&self->backend, 0, sizeof(self->backend));
}
