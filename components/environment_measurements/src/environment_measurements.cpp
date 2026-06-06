#include "environment_measurements.h"

#include <atomic>
#include <cstdint>

#include "environment_sensors.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#ifndef CONFIG_REDMOLE_ENVIRONMENT_READING_INTERVAL_SEC
#define CONFIG_REDMOLE_ENVIRONMENT_READING_INTERVAL_SEC 1
#endif

namespace {

using redmole::environment::EnvironmentLocation;
using redmole::environment::EnvironmentSensorBinding;
using redmole::environment::EnvironmentSensorReading;
using redmole::environment::EnvironmentSensors;
using redmole::environment::kMicrosecondsPerMillisecond;

constexpr const char* kTag = "ENV_MEASURE";
constexpr uint32_t kReadingIntervalMs = CONFIG_REDMOLE_ENVIRONMENT_READING_INTERVAL_SEC * 1000U;
constexpr uint32_t kTaskStackDepth = 4096U;
constexpr UBaseType_t kTaskPriority = 5U;
constexpr int64_t kStaleTimeoutMs = 5000LL;

int64_t now_ms() {
    return esp_timer_get_time() / kMicrosecondsPerMillisecond;
}

class EnvironmentMeasurements {
  public:
    esp_err_t init() {
        esp_err_t result;

        if (initialized_) {
            return ESP_OK;
        }

        latest_mutex_ = xSemaphoreCreateMutexStatic(&latest_mutex_storage_);
        stopped_ = xSemaphoreCreateBinaryStatic(&stopped_storage_);
        if (latest_mutex_ == nullptr) {
            return ESP_ERR_NO_MEM;
        }

        if (stopped_ == nullptr) {
            return ESP_ERR_NO_MEM;
        }

        for (EnvironmentSensorBinding& binding : sensors_.bindings()) {
            result = binding.sensor->init();
            if (result != ESP_OK) {
                ESP_LOGE(kTag, "%s initialization failed: %s", binding.name,
                         esp_err_to_name(result));
                binding.last_read_failed = true;
            }
        }

        initialized_ = true;
        return ESP_OK;
    }

    esp_err_t start() {
        if (!initialized_) {
            return ESP_ERR_INVALID_STATE;
        }

        if (running_) {
            return ESP_OK;
        }

        stop_requested_.store(false, std::memory_order_release);

        if (task_ == nullptr) {
            task_ = xTaskCreateStatic(task_entry, "environment", kTaskStackDepth, this,
                                      kTaskPriority, task_stack_, &task_control_block_);
            if (task_ == nullptr) {
                return ESP_ERR_NO_MEM;
            }

            running_ = true;
            return ESP_OK;
        }

        xTaskNotifyGive(task_);
        running_ = true;
        return ESP_OK;
    }

    void stop() {
        if (!running_) {
            return;
        }

        stop_requested_.store(true, std::memory_order_release);
        xTaskNotifyGive(task_);
        xSemaphoreTake(stopped_, portMAX_DELAY);
        running_ = false;
    }

    bool get_latest(environment_measurement_sample_t* out) const {
        int64_t current_ms;
        int64_t sample_age_ms;

        if (out == nullptr) {
            return false;
        }

        if (!initialized_) {
            return false;
        }

        xSemaphoreTake(latest_mutex_, portMAX_DELAY);
        *out = latest_;
        xSemaphoreGive(latest_mutex_);

        current_ms = now_ms();
        if (!out->valid) {
            return false;
        }

        if (out->timestamp_ms > current_ms) {
            out->valid = false;
            return false;
        }

        sample_age_ms = current_ms - out->timestamp_ms;
        if (sample_age_ms > kStaleTimeoutMs) {
            out->valid = false;
            return false;
        }

        return true;
    }

    bool is_fresh(uint32_t max_age_ms) const {
        environment_measurement_sample_t sample = {};
        int64_t current_ms;
        int64_t sample_age_ms;

        if (!get_latest(&sample)) {
            return false;
        }

        current_ms = now_ms();
        sample_age_ms = current_ms - sample.timestamp_ms;
        if (sample_age_ms > static_cast<int64_t>(max_age_ms)) {
            return false;
        }

        return true;
    }

    uint32_t get_update_count() const {
        return update_count_.load(std::memory_order_relaxed);
    }

  private:
    static void task_entry(void* context) {
        EnvironmentMeasurements* measurements;

        measurements = static_cast<EnvironmentMeasurements*>(context);
        measurements->task_loop();
    }

    void task_loop() {
        EnvironmentSensorReading reading = {};
        esp_err_t result;

        while (true) {
            if (stop_requested_.load(std::memory_order_acquire)) {
                xSemaphoreGive(stopped_);
                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
                continue;
            }

            for (EnvironmentSensorBinding& binding : sensors_.bindings()) {
                reading = {};
                result = binding.sensor->read(reading);
                process_reading(binding, result, reading);
            }

            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(kReadingIntervalMs));
        }
    }

    void process_reading(EnvironmentSensorBinding& binding, esp_err_t result,
                         const EnvironmentSensorReading& reading) {
        environment_measurement_sample_t sample = {};

        if (result != ESP_OK) {
            if (binding.location == EnvironmentLocation::Indoor) {
                invalidate_latest();
            }

            if (!binding.last_read_failed) {
                ESP_LOGE(kTag, "%s read failed: %s", binding.name, esp_err_to_name(result));
                binding.last_read_failed = true;
            }
            return;
        }

        if (binding.last_read_failed) {
            ESP_LOGI(kTag, "%s recovered", binding.name);
            binding.last_read_failed = false;
        }

        if (binding.location != EnvironmentLocation::Indoor) {
            return;
        }

        if (!reading.has_temperature) {
            return;
        }

        if (!reading.has_humidity) {
            return;
        }

        if (!reading.has_pressure) {
            return;
        }

        sample.timestamp_ms = reading.timestamp_ms;
        sample.temperature_deci_c = reading.temperature_deci_c;
        sample.humidity_deci_pct = reading.humidity_deci_pct;
        sample.pressure_deci_hpa = reading.pressure_deci_hpa;
        sample.valid = true;
        publish(sample);
    }

    void publish(const environment_measurement_sample_t& sample) {
        xSemaphoreTake(latest_mutex_, portMAX_DELAY);
        latest_ = sample;
        xSemaphoreGive(latest_mutex_);
        update_count_.fetch_add(1U, std::memory_order_relaxed);
    }

    void invalidate_latest() {
        xSemaphoreTake(latest_mutex_, portMAX_DELAY);
        latest_.valid = false;
        xSemaphoreGive(latest_mutex_);
    }

    EnvironmentSensors sensors_;
    mutable StaticSemaphore_t latest_mutex_storage_ = {};
    mutable SemaphoreHandle_t latest_mutex_ = nullptr;
    StaticSemaphore_t stopped_storage_ = {};
    SemaphoreHandle_t stopped_ = nullptr;
    StaticTask_t task_control_block_ = {};
    StackType_t task_stack_[kTaskStackDepth] = {};
    TaskHandle_t task_ = nullptr;
    environment_measurement_sample_t latest_ = {};
    std::atomic_uint update_count_{0U};
    std::atomic_bool stop_requested_{false};
    bool initialized_ = false;
    bool running_ = false;
};

EnvironmentMeasurements s_environment_measurements;

} // namespace

extern "C" esp_err_t environment_measurements_init(void) {
    return s_environment_measurements.init();
}

extern "C" esp_err_t environment_measurements_start(void) {
    return s_environment_measurements.start();
}

extern "C" void environment_measurements_stop(void) {
    s_environment_measurements.stop();
}

extern "C" void environment_measurements_deinit(void) {
    s_environment_measurements.stop();
}

extern "C" bool environment_measurements_get_latest(environment_measurement_sample_t* out) {
    return s_environment_measurements.get_latest(out);
}

extern "C" bool environment_measurements_is_fresh(uint32_t max_age_ms) {
    return s_environment_measurements.is_fresh(max_age_ms);
}

extern "C" uint32_t environment_measurements_get_update_count(void) {
    return s_environment_measurements.get_update_count();
}
