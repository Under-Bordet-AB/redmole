#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "app_gui_bindings.h"
#include "task_scheduler.h"
#include "bme280_hal.h"
#include "gui_module.h"
#include "nac.h"
#include "rm_nvs.h"
#include "sensor_data.h"
#include "http_client.h"

static const char* TAG = "MAIN";
static bme280_hal s_sensor_hal = {0};
static gui_ctx_t s_gui = {0};

static esp_err_t init_single_instance_modules(void) {
    esp_err_t rv = rm_nvs_init("app");
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "rm_nvs_init failed: %s", esp_err_to_name(rv));
        return rv;
    }

    if (task_scheduler_init() != ESP_OK) {
        ESP_LOGE(TAG, "task_scheduler_init failed:", esp_err_to_name(rv));
        return rv;
    }

    if (nac_init() != ESP_OK) {
        ESP_LOGE(TAG, "nac_init failed: %s", esp_err_to_name(rv));
        return rv;
    }

    if (http_client_init(HTTP_CLIENT_TLS_NONE, NULL) != ESP_OK) {
        ESP_LOGE(TAG, "http_client_init failed: %s", esp_err_to_name(rv));
        return rv;
    }

    rv = sensor_data_init();
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "sensor_data_init failed: %s", esp_err_to_name(rv));
        return rv;
    }

    return ESP_OK;
}

static esp_err_t init_runtime_modules(void) {
    esp_err_t rv = bme280_hal_init(&s_sensor_hal);
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "bme280_hal_init failed: %s", esp_err_to_name(rv));
        return rv;
    }

    gui_init(&s_gui);
    rv = app_gui_bindings_init(&s_gui);
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "app_gui_bindings_init failed: %s", esp_err_to_name(rv));
        return rv;
    }

    return ESP_OK;
}

static void sensor_local_source_task(void* pvParameters) {
    bme280_measurement measurement = {0};
    sensor_data_sample sample = {0};
    uint32_t period_ms = 1000U;

    (void)pvParameters;

    period_ms = bme280_hal_get_period_ms(&s_sensor_hal);
    if (period_ms == 0U) {
        period_ms = 1000U;
    }

    while (true) {
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
        } else {
            ESP_LOGW(TAG, "bme280_hal_read failed: %s", esp_err_to_name(rv));
        }

        vTaskDelay(pdMS_TO_TICKS(period_ms));
    }
}

static esp_err_t start_runtime_modules() {

    if (task_scheduler_init() != ESP_OK) {
        ESP_LOGE(TAG, "task_scheduler_init failed");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "task_scheduler_init started");

    /*
    if(nac_request_wifi_connect() != ESP_OK) {
        ESP_LOGE(TAG, "nac_request_wifi_connect failed");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "nac_request_wifi_connect started");*/

    BaseType_t sensor =
        xTaskCreate(sensor_local_source_task, "sensor_local_source_task", 4096, NULL, 5, NULL);
    if (sensor != pdPASS) {
        ESP_LOGE(TAG, "Could not spawn sensor_local_source_task!");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "sensor_local_source_task started");

    if (!s_gui.is_ready) {
        ESP_LOGW(TAG, "GUI not ready after initialization");
    }

    return ESP_OK;
}

void app_main(void) {

    ESP_LOGI(TAG, "app_main entered");
    ESP_LOGI(TAG, "Initializing single-instance modules");
    if (init_single_instance_modules() != ESP_OK) {
        goto fatal_error;
    }

    ESP_LOGI(TAG, "Initializing runtime modules");
    if (init_runtime_modules() != ESP_OK) {
        goto fatal_error;
    }

    if (start_runtime_modules() != ESP_OK) {
        goto fatal_error;
    }

    //vTaskDelay(pdMS_TO_TICKS(5000));
    ESP_LOGI(TAG, "Startup complete");

    /* Pump the task scheduler to start tasks, needed to connect wifi in this mock code
     * GUI would trigger scheduler work in real project
     */
    while (1) {
        app_gui_bindings_sync(&s_gui);
        task_scheduler_work();
        vTaskDelay(pdMS_TO_TICKS(30));
    }
    return;

fatal_error:
    ESP_LOGE(TAG, "System halted during startup");
}
