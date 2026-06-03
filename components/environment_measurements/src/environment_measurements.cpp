#include "environment_measurements.h"

#include <atomic>
#include <cstdint>
#include <cstring>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" {
#include "bme280_hal.h"
#include "board_i2c.h"
}

namespace {

constexpr uint32_t kTaskStackBytes = 4096U;
constexpr UBaseType_t kTaskPriority = 5U;
constexpr const char* kTag = "ENV_MEASURE";

class EnvironmentMeasurements {
public:
    esp_err_t init() {
        if (initialized_) {
            return ESP_OK;
        }

        reset_store();
        std::memset(&sensor_hal_, 0, sizeof(sensor_hal_));

        esp_err_t rv = bme280_hal_init(&sensor_hal_);
        if (rv != ESP_OK) {
            ESP_LOGE(kTag, "bme280_hal_init failed: %s", esp_err_to_name(rv));
            return rv;
        }

        initialized_ = true;
        return ESP_OK;
    }

    esp_err_t start() {
        if (!initialized_) {
            return ESP_ERR_INVALID_STATE;
        }

        if (task_ != nullptr) {
            return ESP_OK;
        }

        BaseType_t created =
            xTaskCreate(&EnvironmentMeasurements::task_entry, "env_measure_task",
                        kTaskStackBytes, this, kTaskPriority, &task_);
        if (created != pdPASS) {
            ESP_LOGE(kTag, "Could not spawn env_measure_task");
            task_ = nullptr;
            return ESP_FAIL;
        }

        ESP_LOGI(kTag, "env_measure_task started");
        return ESP_OK;
    }

    void deinit() {
        if (task_ != nullptr) {
            vTaskDelete(task_);
            task_ = nullptr;
        }

        if (initialized_) {
            bme280_hal_deinit(&sensor_hal_);
        }

        reset_store();
        std::memset(&sensor_hal_, 0, sizeof(sensor_hal_));
        initialized_ = false;
    }

    bool get_latest(environment_measurement_sample_t* out) const {
        if ((out == nullptr) || !initialized_) {
            return false;
        }

        unsigned int version_before = 0U;
        unsigned int version_after = 0U;

        do {
            version_before = version_.load(std::memory_order_acquire);
            if ((version_before & 1U) != 0U) {
                continue;
            }

            *out = latest_;
            version_after = version_.load(std::memory_order_acquire);
        } while (version_before != version_after);

        return out->valid;
    }

    bool is_fresh(uint32_t max_age_ms) const {
        environment_measurement_sample_t sample = {};
        const int64_t now_ms = esp_timer_get_time() / 1000LL;

        if (!get_latest(&sample)) {
            return false;
        }

        if (sample.timestamp_ms > now_ms) {
            return false;
        }

        return static_cast<uint64_t>(now_ms - sample.timestamp_ms) <=
               static_cast<uint64_t>(max_age_ms);
    }

    uint32_t get_update_count() const {
        if (!initialized_) {
            return 0U;
        }

        return update_count_.load(std::memory_order_relaxed);
    }

private:
    static void task_entry(void* context) {
        auto* self = static_cast<EnvironmentMeasurements*>(context);
        self->task_loop();
    }

    void task_loop() {
        bme280_measurement measurement = {};
        environment_measurement_sample_t sample = {};
        uint32_t period_ms = bme280_hal_get_period_ms(&sensor_hal_);
        if (period_ms == 0U) {
            period_ms = BME280_HAL_DEFAULT_PERIOD_MS;
        }

        bool last_hardware_present = board_i2c_bme280_present();
        log_hardware_presence(last_hardware_present);

        while (true) {
            const bool hardware_present = board_i2c_bme280_present();
            if (hardware_present != last_hardware_present) {
                log_hardware_presence(hardware_present);
                last_hardware_present = hardware_present;
            }

            esp_err_t rv = bme280_hal_read(&sensor_hal_, &measurement);
            if (rv == ESP_OK) {
                sample.timestamp_ms = measurement.timestamp_ms;
                sample.temperature_deci_c = measurement.temperature_deci_c;
                sample.humidity_deci_pct = measurement.humidity_deci_pct;
                sample.pressure_deci_hpa = measurement.pressure_deci_hpa;
                sample.valid = true;

                rv = publish(sample);
                if (rv != ESP_OK) {
                    ESP_LOGE(kTag, "publish failed: %s", esp_err_to_name(rv));
                }
            } else if (hardware_present) {
                ESP_LOGW(kTag, "bme280_hal_read failed: %s", esp_err_to_name(rv));
            }

            vTaskDelay(pdMS_TO_TICKS(period_ms));
        }
    }

    esp_err_t publish(const environment_measurement_sample_t& sample) {
        if (!initialized_) {
            return ESP_ERR_INVALID_STATE;
        }

        version_.fetch_add(1U, std::memory_order_relaxed);
        latest_ = sample;
        update_count_.fetch_add(1U, std::memory_order_relaxed);
        version_.fetch_add(1U, std::memory_order_release);
        return ESP_OK;
    }

    void reset_store() {
        latest_ = {};
        version_.store(0U, std::memory_order_relaxed);
        update_count_.store(0U, std::memory_order_relaxed);
    }

    static void log_hardware_presence(bool present) {
        if (present) {
            ESP_LOGI(kTag, "BME280 detected on board I2C");
        } else {
            ESP_LOGW(kTag, "BME280 not detected on board I2C");
        }
    }

    bme280_hal sensor_hal_ = {};
    TaskHandle_t task_ = nullptr;
    environment_measurement_sample_t latest_ = {};
    std::atomic_uint version_{0U};
    std::atomic_uint update_count_{0U};
    bool initialized_ = false;
};

EnvironmentMeasurements g_environment_measurements;

} // namespace

extern "C" esp_err_t environment_measurements_init(void) {
    return g_environment_measurements.init();
}

extern "C" esp_err_t environment_measurements_start(void) {
    return g_environment_measurements.start();
}

extern "C" void environment_measurements_deinit(void) {
    g_environment_measurements.deinit();
}

extern "C" bool environment_measurements_get_latest(environment_measurement_sample_t* out) {
    return g_environment_measurements.get_latest(out);
}

extern "C" bool environment_measurements_is_fresh(uint32_t max_age_ms) {
    return g_environment_measurements.is_fresh(max_age_ms);
}

extern "C" uint32_t environment_measurements_get_update_count(void) {
    return g_environment_measurements.get_update_count();
}
