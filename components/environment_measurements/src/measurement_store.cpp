#include "measurement_store.hpp"

namespace redmole::environment {

esp_err_t MeasurementStore::init() {
    if (mutex_ != nullptr) {
        return ESP_OK;
    }

    mutex_ = xSemaphoreCreateMutexStatic(&mutex_storage_);
    return mutex_ != nullptr ? ESP_OK : ESP_ERR_NO_MEM;
}

esp_err_t MeasurementStore::publish_batch(const MeasurementBatch& batch, int64_t timestamp_ms) {
    if (mutex_ == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!batch_is_valid(batch)) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(mutex_, portMAX_DELAY);
    const uint64_t publication_version = next_publication_version_++;
    for (size_t index = 0; index < batch.count; index++) {
        const Measurement& measurement = batch.measurements[index];
        StoredMeasurement& stored = latest_[measurement_channel_index(measurement.channel)];
        stored.timestamp_ms = timestamp_ms;
        stored.value = measurement.value;
        stored.publication_version = publication_version;
        stored.valid = true;
    }
    xSemaphoreGive(mutex_);
    return ESP_OK;
}

esp_err_t MeasurementStore::invalidate_channels(const MeasurementChannel* channels, size_t count) {
    if (mutex_ == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!channels_are_valid(channels, count)) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(mutex_, portMAX_DELAY);
    for (size_t index = 0; index < count; index++) {
        latest_[measurement_channel_index(channels[index])].valid = false;
    }
    xSemaphoreGive(mutex_);
    return ESP_OK;
}

bool MeasurementStore::copy_channels(const MeasurementChannel* channels, size_t count,
                                     StoredMeasurement* out) const {
    if (mutex_ == nullptr || out == nullptr || !channels_are_valid(channels, count)) {
        return false;
    }

    xSemaphoreTake(mutex_, portMAX_DELAY);
    for (size_t index = 0; index < count; index++) {
        out[index] = latest_[measurement_channel_index(channels[index])];
    }
    xSemaphoreGive(mutex_);
    return true;
}

bool MeasurementStore::batch_is_valid(const MeasurementBatch& batch) {
    if (batch.count == 0U || batch.count > batch.measurements.size()) {
        return false;
    }

    for (size_t index = 0; index < batch.count; index++) {
        const MeasurementChannel channel = batch.measurements[index].channel;
        if (!measurement_channel_is_valid(channel)) {
            return false;
        }
        for (size_t previous = 0; previous < index; previous++) {
            if (batch.measurements[previous].channel == channel) {
                return false;
            }
        }
    }
    return true;
}

bool MeasurementStore::channels_are_valid(const MeasurementChannel* channels, size_t count) {
    if (channels == nullptr || count == 0U) {
        return false;
    }

    for (size_t index = 0; index < count; index++) {
        if (!measurement_channel_is_valid(channels[index])) {
            return false;
        }
    }
    return true;
}

} // namespace redmole::environment
