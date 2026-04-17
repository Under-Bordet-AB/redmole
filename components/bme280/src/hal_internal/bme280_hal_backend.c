#include "hal_internal/bme280_hal_backend.h"

#include <string.h>

#include "esp_log.h"

typedef struct {
    int32_t temperature_deci_c;
    int32_t humidity_deci_pct;
    int32_t pressure_deci_hpa;
} bme280_raw_sample;

static esp_err_t bme280_hw_bus_init_stub(bme280_hal* self) {
    (void)self;
    return ESP_ERR_NOT_SUPPORTED;
}

static esp_err_t bme280_hw_chip_setup_stub(bme280_hal* self) {
    (void)self;
    return ESP_ERR_NOT_SUPPORTED;
}

static esp_err_t bme280_hw_load_calibration_stub(bme280_hal* self) {
    (void)self;
    return ESP_ERR_NOT_SUPPORTED;
}

static esp_err_t bme280_hw_read_raw_stub(bme280_hal* self, bme280_raw_sample* out_raw) {
    (void)self;
    (void)out_raw;
    return ESP_ERR_NOT_SUPPORTED;
}

static esp_err_t bme280_hw_convert_stub(const bme280_raw_sample* raw,
                                        bme280_measurement* out_measurement) {
    (void)raw;
    (void)out_measurement;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bme280_hal_backend_init(bme280_hal* self) {
    esp_err_t rv = ESP_OK;

    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(&self->backend, 0, sizeof(self->backend));

    rv = bme280_hw_bus_init_stub(self);
    if (rv == ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGI(self->tag, "Hardware bus init is still stubbed");
        return ESP_OK;
    }
    if (rv != ESP_OK) {
        return rv;
    }

    rv = bme280_hw_chip_setup_stub(self);
    if (rv == ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGI(self->tag, "Chip setup is still stubbed");
        return ESP_OK;
    }
    if (rv != ESP_OK) {
        return rv;
    }

    rv = bme280_hw_load_calibration_stub(self);
    if (rv == ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGI(self->tag, "Calibration load is still stubbed");
        return ESP_OK;
    }
    if (rv != ESP_OK) {
        return rv;
    }

    self->hardware_present = true;
    return ESP_OK;
}

esp_err_t bme280_hal_backend_read(bme280_hal* self, bme280_measurement* out) {
    bme280_raw_sample raw = {0};
    esp_err_t rv = ESP_OK;

    if ((self == NULL) || (out == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    rv = bme280_hw_read_raw_stub(self, &raw);
    if (rv != ESP_OK) {
        return rv;
    }

    return bme280_hw_convert_stub(&raw, out);
}

void bme280_hal_backend_deinit(bme280_hal* self) {
    if (self == NULL) {
        return;
    }

    memset(&self->backend, 0, sizeof(self->backend));
}
