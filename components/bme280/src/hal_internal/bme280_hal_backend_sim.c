#include "hal_internal/bme280_hal_backend.h"

#include <stdlib.h>
#include <string.h>

#include "esp_timer.h"

#define BME280_SIM_US_PER_MS 1000LL

typedef struct {
    uint32_t sample_index;
} bme280_sim_backend_state;

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

    bme280_sim_backend_state* state = calloc(1, sizeof(*state));
    if (state == NULL) {
        return ESP_ERR_NO_MEM;
    }

    self->backend_state = state;
    self->hardware_present = false;
    self->period_ms = BME280_HAL_DEFAULT_PERIOD_MS;
    return ESP_OK;
}

esp_err_t bme280_hal_backend_read(bme280_hal* self, bme280_measurement* out) {
    if ((self == NULL) || (out == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    bme280_sim_backend_state* state = (bme280_sim_backend_state*)self->backend_state;
    if (state == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    out->timestamp_ms = esp_timer_get_time() / BME280_SIM_US_PER_MS;
    out->temperature_deci_c = triangle_wave(state->sample_index, 225, 22);
    out->humidity_deci_pct = triangle_wave(state->sample_index + 11U, 470, 80);
    out->pressure_deci_hpa = triangle_wave(state->sample_index + 23U, 10120, 65);

    state->sample_index++;
    return ESP_OK;
}

void bme280_hal_backend_deinit(bme280_hal* self) {
    if (self == NULL) {
        return;
    }

    free(self->backend_state);
    self->backend_state = NULL;
    self->period_ms = 0U;
}
