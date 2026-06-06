#include "environment_measurements_internal.hpp"

#include "esp_log.h"
#include "sdkconfig.h"

#ifndef CONFIG_REDMOLE_ENVIRONMENT_READING_INTERVAL_SEC
#define CONFIG_REDMOLE_ENVIRONMENT_READING_INTERVAL_SEC 1
#endif

namespace redmole::environment {
namespace {

constexpr const char* kTag = "ENV_MEASURE";
constexpr uint32_t kReadingIntervalMs = CONFIG_REDMOLE_ENVIRONMENT_READING_INTERVAL_SEC * 1000U;
constexpr UBaseType_t kTaskPriority = 5U;
constexpr int64_t kStaleTimeoutMs = 5000LL;

} // namespace

EnvironmentMeasurements::EnvironmentMeasurements(const char* source_name,
                                                 TemperatureHumidityPressureSource& source,
                                                 NowMilliseconds now_ms)
    : source_name_(source_name), source_(source), now_ms_(now_ms) {
}

esp_err_t EnvironmentMeasurements::init() {
    esp_err_t result;

    if (initialized_) {
        return ESP_OK;
    }

    latest_mutex_ = xSemaphoreCreateMutexStatic(&latest_mutex_storage_);
    stopped_ = xSemaphoreCreateBinaryStatic(&stopped_storage_);
    if (latest_mutex_ == nullptr || stopped_ == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    result = source_.init();
    if (result != ESP_OK) {
        ESP_LOGE(kTag, "%s initialization failed: %s", source_name_, esp_err_to_name(result));
        read_failed_ = true;
    }

    initialized_ = true;
    return ESP_OK;
}

esp_err_t EnvironmentMeasurements::start() {
    if (!initialized_) {
        return ESP_ERR_INVALID_STATE;
    }

    if (running_) {
        return ESP_OK;
    }

    stop_requested_.store(false, std::memory_order_release);
    if (task_ == nullptr) {
        task_ = xTaskCreateStatic(task_entry, "environment", kTaskStackDepth, this, kTaskPriority,
                                  task_stack_, &task_control_block_);
        if (task_ == nullptr) {
            return ESP_ERR_NO_MEM;
        }
    } else {
        xTaskNotifyGive(task_);
    }

    running_ = true;
    return ESP_OK;
}

void EnvironmentMeasurements::stop() {
    if (!running_) {
        return;
    }

    stop_requested_.store(true, std::memory_order_release);
    xTaskNotifyGive(task_);
    xSemaphoreTake(stopped_, portMAX_DELAY);
    running_ = false;
}

bool EnvironmentMeasurements::get_latest(environment_measurement_sample_t* out) const {
    if (out == nullptr || !initialized_) {
        return false;
    }

    const int64_t current_ms = now_ms_();
    xSemaphoreTake(latest_mutex_, portMAX_DELAY);
    *out = latest_;
    xSemaphoreGive(latest_mutex_);

    if (!sample_is_fresh(*out, current_ms, kStaleTimeoutMs)) {
        out->valid = false;
        return false;
    }

    return true;
}

bool EnvironmentMeasurements::is_fresh(uint32_t max_age_ms) const {
    if (!initialized_) {
        return false;
    }

    const int64_t current_ms = now_ms_();
    xSemaphoreTake(latest_mutex_, portMAX_DELAY);
    const bool fresh = sample_is_fresh(latest_, current_ms, static_cast<int64_t>(max_age_ms));
    xSemaphoreGive(latest_mutex_);
    return fresh;
}

uint32_t EnvironmentMeasurements::get_update_count() const {
    return update_count_.load(std::memory_order_relaxed);
}

esp_err_t EnvironmentMeasurements::poll_once() {
    TemperatureHumidityPressureReading reading = {};
    const esp_err_t result = source_.read(reading);

    if (result != ESP_OK) {
        invalidate_snapshot();
        if (!read_failed_) {
            ESP_LOGE(kTag, "%s read failed: %s", source_name_, esp_err_to_name(result));
            read_failed_ = true;
        }
        return result;
    }

    if (read_failed_) {
        ESP_LOGI(kTag, "%s recovered", source_name_);
        read_failed_ = false;
    }

    publish(reading);
    return ESP_OK;
}

void EnvironmentMeasurements::task_entry(void* context) {
    static_cast<EnvironmentMeasurements*>(context)->task_loop();
}

void EnvironmentMeasurements::task_loop() {
    while (true) {
        if (stop_requested_.load(std::memory_order_acquire)) {
            xSemaphoreGive(stopped_);
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            continue;
        }

        poll_once();
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(kReadingIntervalMs));
    }
}

void EnvironmentMeasurements::publish(const TemperatureHumidityPressureReading& reading) {
    environment_measurement_sample_t sample = {};

    sample.timestamp_ms = now_ms_();
    sample.temperature_deci_c = reading.temperature.deci_c;
    sample.humidity_deci_pct = reading.humidity.deci_pct;
    sample.pressure_deci_hpa = reading.pressure.deci_hpa;
    sample.valid = true;

    xSemaphoreTake(latest_mutex_, portMAX_DELAY);
    latest_ = sample;
    xSemaphoreGive(latest_mutex_);
    update_count_.fetch_add(1U, std::memory_order_relaxed);
}

void EnvironmentMeasurements::invalidate_snapshot() {
    xSemaphoreTake(latest_mutex_, portMAX_DELAY);
    latest_.valid = false;
    xSemaphoreGive(latest_mutex_);
}

bool EnvironmentMeasurements::sample_is_fresh(const environment_measurement_sample_t& sample,
                                              int64_t current_ms, int64_t max_age_ms) {
    if (!sample.valid || sample.timestamp_ms > current_ms) {
        return false;
    }

    return current_ms - sample.timestamp_ms <= max_age_ms;
}

} // namespace redmole::environment
