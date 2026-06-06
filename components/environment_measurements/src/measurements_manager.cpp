#include "measurements_manager.hpp"

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

} // namespace

MeasurementsManager::MeasurementsManager(const ProducerRegistration* registrations,
                                         size_t registration_count, MeasurementStore& store,
                                         NowMilliseconds now_ms)
    : registrations_(registrations), registration_count_(registration_count), store_(store),
      now_ms_(now_ms) {
}

esp_err_t MeasurementsManager::init() {
    if (initialized_) {
        return ESP_OK;
    }
    if (!registrations_are_valid() || now_ms_ == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t result = store_.init();
    if (result != ESP_OK) {
        return result;
    }

    stopped_ = xSemaphoreCreateBinaryStatic(&stopped_storage_);
    if (stopped_ == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    for (size_t index = 0; index < registration_count_; index++) {
        result = registrations_[index].producer.init();
        if (result != ESP_OK) {
            mark_failed(index, result);
        }
    }

    initialized_ = true;
    return ESP_OK;
}

esp_err_t MeasurementsManager::start() {
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

void MeasurementsManager::stop() {
    if (!running_) {
        return;
    }

    stop_requested_.store(true, std::memory_order_release);
    xTaskNotifyGive(task_);
    xSemaphoreTake(stopped_, portMAX_DELAY);
    running_ = false;
}

esp_err_t MeasurementsManager::poll_once() {
    if (!initialized_) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t first_error = ESP_OK;
    for (size_t index = 0; index < registration_count_; index++) {
        const ProducerRegistration& registration = registrations_[index];
        MeasurementBatch batch = {};
        esp_err_t result = registration.producer.read(batch);

        if (result == ESP_OK && !batch_belongs_to(batch, registration)) {
            result = ESP_ERR_INVALID_RESPONSE;
        }
        if (result == ESP_OK) {
            result = store_.publish_batch(batch, now_ms_());
        }

        if (result != ESP_OK) {
            store_.invalidate_channels(registration.channels, registration.channel_count);
            mark_failed(index, result);
            if (first_error == ESP_OK) {
                first_error = result;
            }
            continue;
        }

        mark_recovered(index);
    }

    return first_error;
}

void MeasurementsManager::task_entry(void* context) {
    static_cast<MeasurementsManager*>(context)->task_loop();
}

void MeasurementsManager::task_loop() {
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

bool MeasurementsManager::registrations_are_valid() const {
    if (registrations_ == nullptr || registration_count_ == 0U ||
        registration_count_ > kMaxProducerCount) {
        return false;
    }

    std::array<bool, measurement_channel_index(MeasurementChannel::Count)> owned = {};
    for (size_t producer_index = 0; producer_index < registration_count_; producer_index++) {
        const ProducerRegistration& registration = registrations_[producer_index];
        if (registration.diagnostic_name == nullptr || registration.channels == nullptr ||
            registration.channel_count == 0U) {
            return false;
        }

        for (size_t channel_index = 0; channel_index < registration.channel_count;
             channel_index++) {
            const MeasurementChannel channel = registration.channels[channel_index];
            if (!measurement_channel_is_valid(channel) || owned[measurement_channel_index(channel)]) {
                return false;
            }
            owned[measurement_channel_index(channel)] = true;
        }
    }
    return true;
}

bool MeasurementsManager::batch_belongs_to(const MeasurementBatch& batch,
                                           const ProducerRegistration& registration) const {
    if (batch.count == 0U || batch.count > batch.measurements.size()) {
        return false;
    }

    for (size_t measurement_index = 0; measurement_index < batch.count; measurement_index++) {
        const MeasurementChannel channel = batch.measurements[measurement_index].channel;
        bool registered = false;
        for (size_t channel_index = 0; channel_index < registration.channel_count; channel_index++) {
            if (registration.channels[channel_index] == channel) {
                registered = true;
                break;
            }
        }
        if (!registered) {
            return false;
        }

        for (size_t previous = 0; previous < measurement_index; previous++) {
            if (batch.measurements[previous].channel == channel) {
                return false;
            }
        }
    }
    return true;
}

void MeasurementsManager::mark_failed(size_t producer_index, esp_err_t error) {
    if (!producer_failed_[producer_index]) {
        ESP_LOGE(kTag, "%s failed: %s", registrations_[producer_index].diagnostic_name,
                 esp_err_to_name(error));
        producer_failed_[producer_index] = true;
    }
}

void MeasurementsManager::mark_recovered(size_t producer_index) {
    if (producer_failed_[producer_index]) {
        ESP_LOGI(kTag, "%s recovered", registrations_[producer_index].diagnostic_name);
        producer_failed_[producer_index] = false;
    }
}

} // namespace redmole::environment
