#include "local_sensor_service.h"

#include <stdbool.h>
#include <string.h>

#include "bme280_hal.h"
#include "board_i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sensor_data.h"

static const char* TAG = "LOCAL_SENSOR";

#define LOCAL_SENSOR_TASK_STACK_BYTES 4096U
#define LOCAL_SENSOR_TASK_PRIORITY 5U

static bme280_hal s_sensor_hal;
static TaskHandle_t s_task;
static bool s_initialized;

static void local_sensor_log_hardware_presence(bool present) {
    if (present) {
        ESP_LOGI(TAG, "BME280 detected on board I2C");
    } else {
        ESP_LOGW(TAG, "BME280 not detected on board I2C");
    }
}

static void local_sensor_task(void* pvParameters) {
    bme280_measurement measurement = {0};
    sensor_data_sample sample = {0};
    uint32_t period_ms = BME280_HAL_DEFAULT_PERIOD_MS;
    bool last_hardware_present;

    (void)pvParameters;

    period_ms = bme280_hal_get_period_ms(&s_sensor_hal);
    if (period_ms == 0U) {
        period_ms = BME280_HAL_DEFAULT_PERIOD_MS;
    }

    last_hardware_present = board_i2c_bme280_present();
    local_sensor_log_hardware_presence(last_hardware_present);

    while (true) {
        bool hardware_present = board_i2c_bme280_present();
        if (hardware_present != last_hardware_present) {
            local_sensor_log_hardware_presence(hardware_present);
            last_hardware_present = hardware_present;
        }

        esp_err_t rv = bme280_hal_read(&s_sensor_hal, &measurement);
        if (rv == ESP_OK) {
            sample.timestamp_ms = measurement.timestamp_ms;
            sample.temperature_deci_c = measurement.temperature_deci_c;
            sample.humidity_deci_pct = measurement.humidity_deci_pct;
            sample.pressure_deci_hpa = measurement.pressure_deci_hpa;
            sample.valid = true;

            rv = sensor_data_submit_local(&sample);
            if (rv != ESP_OK) {
                ESP_LOGE(TAG, "sensor_data_submit_local failed: %s", esp_err_to_name(rv));
            }
        } else if (hardware_present) {
            ESP_LOGW(TAG, "bme280_hal_read failed: %s", esp_err_to_name(rv));
        }

        vTaskDelay(pdMS_TO_TICKS(period_ms));
    }
}

esp_err_t local_sensor_service_init(void) {
    if (s_initialized) {
        return ESP_OK;
    }

    memset(&s_sensor_hal, 0, sizeof(s_sensor_hal));

    esp_err_t rv = bme280_hal_init(&s_sensor_hal);
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "bme280_hal_init failed: %s", esp_err_to_name(rv));
        return rv;
    }

    s_initialized = true;
    return ESP_OK;
}

esp_err_t local_sensor_service_start(void) {
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (s_task != NULL) {
        return ESP_OK;
    }

    BaseType_t created =
        xTaskCreate(local_sensor_task, "local_sensor_task", LOCAL_SENSOR_TASK_STACK_BYTES, NULL,
                    LOCAL_SENSOR_TASK_PRIORITY, &s_task);
    if (created != pdPASS) {
        ESP_LOGE(TAG, "Could not spawn local_sensor_task");
        s_task = NULL;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "local_sensor_task started");
    return ESP_OK;
}

void local_sensor_service_deinit(void) {
    if (s_task != NULL) {
        vTaskDelete(s_task);
        s_task = NULL;
    }

    if (s_initialized) {
        bme280_hal_deinit(&s_sensor_hal);
        s_initialized = false;
    }
}
