#include "environment_measurements.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>

#include "bme280/bme280_sensor.hpp"
#include "environment_manager.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "sim/simulated_bme280_sensor.hpp"

extern "C" {
#include "board_i2c.h"
}

#ifndef CONFIG_REDMOLE_ENVIRONMENT_READING_INTERVAL_SEC
#define CONFIG_REDMOLE_ENVIRONMENT_READING_INTERVAL_SEC 1
#endif

#ifndef CONFIG_REDMOLE_ENVIRONMENT_DETECT_INTERVAL_SEC
#define CONFIG_REDMOLE_ENVIRONMENT_DETECT_INTERVAL_SEC 2
#endif

namespace {

using redmole::environment::Bme280Source;
using redmole::environment::EnvironmentCapability;
using redmole::environment::EnvironmentLocation;
using redmole::environment::EnvironmentManager;
using redmole::environment::EnvironmentMeasurement;
using redmole::environment::kUsPerMs;
using redmole::environment::SimulatedEnvironmentSource;
using redmole::environment::SourceBinding;
using redmole::environment::SourceKind;
using redmole::environment::SourceState;

constexpr uint32_t kTaskStackBytes = 4096U;
constexpr UBaseType_t kTaskPriority = 5U;
constexpr const char* kTag = "ENV_MEASURE";
constexpr uint32_t kReadingIntervalMs = CONFIG_REDMOLE_ENVIRONMENT_READING_INTERVAL_SEC * 1000U;
constexpr uint32_t kDetectionIntervalMs = CONFIG_REDMOLE_ENVIRONMENT_DETECT_INTERVAL_SEC * 1000U;
constexpr int64_t kHistoryIntervalMs = 60000LL;
constexpr size_t kHistoryCapacity = 180U;

int64_t now_ms() {
    return esp_timer_get_time() / kUsPerMs;
}

TickType_t milliseconds_to_ticks(int64_t milliseconds) {
    if (milliseconds <= 0LL) {
        return 1U;
    }

    const uint64_t bounded = milliseconds > static_cast<int64_t>(UINT32_MAX)
                                 ? UINT32_MAX
                                 : static_cast<uint32_t>(milliseconds);
    TickType_t ticks = pdMS_TO_TICKS(static_cast<uint32_t>(bounded));
    return ticks == 0U ? 1U : ticks;
}

/**
 * @brief Process-lifetime production controller and synchronized public store.
 *
 * Owns product source composition, the statically allocated polling task,
 * cooperative lifecycle synchronization, compatibility latest state, and
 * fixed-capacity history. EnvironmentManager policy runs only in the polling
 * task; public callers access copied state through dedicated mutexes.
 */
class EnvironmentMeasurements {
  public:
    EnvironmentMeasurements()
        : bindings_{{
              {&onboard_bme280_, "onboard BME280", EnvironmentLocation::Indoor,
               SourceKind::Hardware, 10U, SourceState::Disabled, 0LL},
              {&alternate_bme280_, "alternate-address BME280", EnvironmentLocation::Indoor,
               SourceKind::Hardware, 20U, SourceState::Disabled, 0LL},
              {&simulated_indoor_, "simulated indoor", EnvironmentLocation::Indoor,
               SourceKind::Simulation, 100U, SourceState::Disabled, 0LL},
              {&simulated_outdoor_, "simulated outdoor", EnvironmentLocation::Outdoor,
               SourceKind::Simulation, 100U, SourceState::Disabled, 0LL},
          }},
          manager_(bindings_.data(), bindings_.size()) {
        manager_.set_state_changed_callback(&EnvironmentMeasurements::source_state_changed);
    }

    EnvironmentMeasurements(const EnvironmentMeasurements&) = delete;
    EnvironmentMeasurements& operator=(const EnvironmentMeasurements&) = delete;

    esp_err_t init() {
        ensure_synchronization();
        take_lifecycle();
        if (initialized_) {
            give_lifecycle();
            return ESP_OK;
        }

        esp_err_t result = board_i2c_init();
        if (result == ESP_OK) {
            result = manager_.init_sources(now_ms());
        }

        if (result == ESP_OK) {
            reset_latest();
            initialized_ = true;
        }
        give_lifecycle();
        return result;
    }

    esp_err_t start() {
        ensure_synchronization();
        take_lifecycle();
        if (!initialized_) {
            give_lifecycle();
            return ESP_ERR_INVALID_STATE;
        }

        if (running_) {
            give_lifecycle();
            return ESP_OK;
        }

        stop_requested_.store(false, std::memory_order_release);
        running_ = true;
        if (task_ == nullptr) {
            task_ = xTaskCreateStatic(&EnvironmentMeasurements::task_entry, "environment",
                                      kTaskStackBytes, this, kTaskPriority, task_stack_,
                                      &task_control_block_);
            if (task_ == nullptr) {
                running_ = false;
                give_lifecycle();
                return ESP_ERR_NO_MEM;
            }
        } else {
            xTaskNotifyGive(task_);
        }

        give_lifecycle();
        return ESP_OK;
    }

    void stop() {
        ensure_synchronization();
        take_lifecycle();
        if (!running_ || (task_ == nullptr)) {
            give_lifecycle();
            return;
        }

        stop_requested_.store(true, std::memory_order_release);
        xTaskNotifyGive(task_);
        xSemaphoreTake(stopped_, portMAX_DELAY);
        running_ = false;
        give_lifecycle();
    }

    bool get_latest(environment_measurement_sample_t* out) const {
        if ((out == nullptr) || !initialized_) {
            return false;
        }

        if (xSemaphoreTake(latest_mutex_, portMAX_DELAY) != pdTRUE) {
            return false;
        }

        *out = latest_;
        xSemaphoreGive(latest_mutex_);
        if (!out->valid || (out->timestamp_ms > now_ms()) ||
            ((now_ms() - out->timestamp_ms) > EnvironmentManager::kStaleTimeoutMs)) {
            out->valid = false;
            return false;
        }

        return true;
    }

    bool is_fresh(uint32_t max_age_ms) const {
        environment_measurement_sample_t sample{};
        if (!get_latest(&sample)) {
            return false;
        }

        return (now_ms() - sample.timestamp_ms) <= static_cast<int64_t>(max_age_ms);
    }

    uint32_t get_update_count() const {
        return initialized_ ? update_count_.load(std::memory_order_relaxed) : 0U;
    }

    size_t copy_history(environment_location_t location, environment_capability_t capability,
                        environment_history_sample_t* out, size_t max_count) const {
        if ((out == nullptr) || (max_count == 0U) || !initialized_ ||
            (location >= ENVIRONMENT_LOCATION_COUNT) ||
            (capability >= ENVIRONMENT_CAPABILITY_COUNT)) {
            return 0U;
        }

        xSemaphoreTake(history_mutex_, portMAX_DELAY);
        size_t copied = 0U;
        for (const HistoryChannel& channel : history_) {
            if ((channel.location != static_cast<EnvironmentLocation>(location)) ||
                (channel.capability != static_cast<EnvironmentCapability>(capability))) {
                continue;
            }

            copied = channel.count < max_count ? channel.count : max_count;
            for (size_t index = 0U; index < copied; index++) {
                out[index] = channel.samples[(channel.start + index) % kHistoryCapacity];
            }
            break;
        }
        xSemaphoreGive(history_mutex_);
        return copied;
    }

  private:
    /** @brief Fixed-capacity circular history for one configured graph channel. */
    struct HistoryChannel {
        EnvironmentLocation location;
        EnvironmentCapability capability;
        std::array<environment_history_sample_t, kHistoryCapacity> samples{};
        size_t start = 0U;
        size_t count = 0U;
    };

    static void task_entry(void* context) {
        static_cast<EnvironmentMeasurements*>(context)->task_loop();
    }

    static void source_state_changed(const SourceBinding& binding,
                                     SourceState previous_state,
                                     SourceState state) {
        if (binding.kind == SourceKind::Simulation) {
            return;
        }

        if (state == SourceState::Active) {
            ESP_LOGI(kTag, "%s connected and active", binding.name);
        } else if ((previous_state == SourceState::Active) &&
                   ((state == SourceState::Failed) || (state == SourceState::Unavailable))) {
            ESP_LOGW(kTag, "%s disconnected or read failed; using simulation fallback",
                     binding.name);
        } else if (state == SourceState::Disabled) {
            ESP_LOGE(kTag, "%s permanently disabled during initialization", binding.name);
        }
    }

    void task_loop() {
        const int64_t started_at_ms = now_ms();
        int64_t next_read_ms = started_at_ms;
        int64_t next_detection_ms = started_at_ms;
        while (true) {
            while (!stop_requested_.load(std::memory_order_acquire)) {
                const int64_t current_ms = now_ms();
                if (current_ms >= next_detection_ms) {
                    manager_.maintain_sources(current_ms);
                    next_detection_ms = current_ms + kDetectionIntervalMs;
                }

                if (current_ms >= next_read_ms) {
                    manager_.poll_active_sources(current_ms);
                    next_read_ms = current_ms + kReadingIntervalMs;
                } else {
                    manager_.refresh_selection(current_ms);
                }

                const int64_t publish_ms = now_ms();
                manager_.refresh_selection(publish_ms);
                publish_compatibility(publish_ms);
                retain_history(publish_ms);
                const int64_t freshness_deadline = manager_.next_freshness_deadline_ms();
                const int64_t next_source_ms =
                    next_detection_ms < next_read_ms ? next_detection_ms : next_read_ms;
                const int64_t next_policy_ms =
                    freshness_deadline < next_source_ms ? freshness_deadline : next_source_ms;
                const int64_t next_wake_ms =
                    next_history_ms_ < next_policy_ms ? next_history_ms_ : next_policy_ms;
                ulTaskNotifyTake(pdTRUE, milliseconds_to_ticks(next_wake_ms - now_ms()));
            }

            xSemaphoreGive(stopped_);
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }
    }

    void publish_compatibility(int64_t current_ms) {
        environment_measurement_sample_t sample{};
        manager_.copy_compatibility(current_ms, sample);

        xSemaphoreTake(latest_mutex_, portMAX_DELAY);
        const bool changed = std::memcmp(&latest_, &sample, sizeof(sample)) != 0;
        latest_ = sample;
        xSemaphoreGive(latest_mutex_);
        if (changed && sample.valid) {
            update_count_.fetch_add(1U, std::memory_order_relaxed);
        }
    }

    void reset_latest() {
        xSemaphoreTake(latest_mutex_, portMAX_DELAY);
        latest_ = {};
        xSemaphoreGive(latest_mutex_);
        update_count_.store(0U, std::memory_order_relaxed);
    }

    void retain_history(int64_t current_ms) {
        if (current_ms < next_history_ms_) {
            return;
        }

        xSemaphoreTake(history_mutex_, portMAX_DELAY);
        for (HistoryChannel& channel : history_) {
            EnvironmentMeasurement measurement{};
            if (!manager_.copy_selected(channel.location, channel.capability, current_ms,
                                        measurement)) {
                continue;
            }

            environment_history_sample_t sample{};
            sample.timestamp_ms = current_ms;
            static_assert(sizeof(sample.value) == sizeof(measurement.value));
            std::memcpy(&sample.value, &measurement.value, sizeof(sample.value));

            const size_t insert = channel.count < kHistoryCapacity
                                      ? (channel.start + channel.count) % kHistoryCapacity
                                      : channel.start;
            channel.samples[insert] = sample;
            if (channel.count < kHistoryCapacity) {
                channel.count++;
            } else {
                channel.start = (channel.start + 1U) % kHistoryCapacity;
            }
        }
        xSemaphoreGive(history_mutex_);
        next_history_ms_ = current_ms + kHistoryIntervalMs;
    }

    void ensure_synchronization() {
        if (latest_mutex_ == nullptr) {
            latest_mutex_ = xSemaphoreCreateMutexStatic(&latest_mutex_storage_);
            history_mutex_ = xSemaphoreCreateMutexStatic(&history_mutex_storage_);
            lifecycle_mutex_ = xSemaphoreCreateMutexStatic(&lifecycle_mutex_storage_);
            stopped_ = xSemaphoreCreateBinaryStatic(&stopped_storage_);
        }
    }

    void take_lifecycle() {
        xSemaphoreTake(lifecycle_mutex_, portMAX_DELAY);
    }

    void give_lifecycle() {
        xSemaphoreGive(lifecycle_mutex_);
    }

    Bme280Source onboard_bme280_{0x76U};
    Bme280Source alternate_bme280_{0x77U};
    SimulatedEnvironmentSource simulated_indoor_{};
    SimulatedEnvironmentSource simulated_outdoor_{};
    std::array<SourceBinding, EnvironmentManager::kMaxBindings> bindings_;
    EnvironmentManager manager_;

    mutable StaticSemaphore_t latest_mutex_storage_{};
    mutable SemaphoreHandle_t latest_mutex_ = nullptr;
    mutable StaticSemaphore_t history_mutex_storage_{};
    mutable SemaphoreHandle_t history_mutex_ = nullptr;
    StaticSemaphore_t lifecycle_mutex_storage_{};
    SemaphoreHandle_t lifecycle_mutex_ = nullptr;
    StaticSemaphore_t stopped_storage_{};
    SemaphoreHandle_t stopped_ = nullptr;

    StaticTask_t task_control_block_{};
    StackType_t task_stack_[kTaskStackBytes]{};
    TaskHandle_t task_ = nullptr;
    environment_measurement_sample_t latest_{};
    std::array<HistoryChannel, 3> history_{{
        {EnvironmentLocation::Indoor, EnvironmentCapability::Temperature},
        {EnvironmentLocation::Indoor, EnvironmentCapability::Humidity},
        {EnvironmentLocation::Indoor, EnvironmentCapability::Pressure},
    }};
    int64_t next_history_ms_ = 0LL;
    std::atomic_uint update_count_{0U};
    std::atomic_bool stop_requested_{false};
    bool initialized_ = false;
    bool running_ = false;
};

EnvironmentMeasurements g_environment_measurements;

} // namespace

extern "C" esp_err_t environment_measurements_init(void) {
    return g_environment_measurements.init();
}

extern "C" esp_err_t environment_measurements_start(void) {
    return g_environment_measurements.start();
}

extern "C" void environment_measurements_stop(void) {
    g_environment_measurements.stop();
}

extern "C" void environment_measurements_deinit(void) {
    g_environment_measurements.stop();
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

extern "C" size_t environment_measurements_copy_history(environment_location_t location,
                                                        environment_capability_t capability,
                                                        environment_history_sample_t* out,
                                                        size_t max_count) {
    return g_environment_measurements.copy_history(location, capability, out, max_count);
}
