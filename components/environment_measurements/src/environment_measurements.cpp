#include "environment_measurements.h"

#include <atomic>
#include <array>
#include <cstdint>
#include <cstring>

#include "bme280/bme280_sensor.hpp"
#include "environment_sensor.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
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

using redmole::environment::Bme280Sensor;
using redmole::environment::EnvironmentSensor;
using redmole::environment::SimulatedBme280Sensor;
using redmole::environment::kUsPerMs;

constexpr uint32_t kTaskStackBytes = 4096U;
constexpr UBaseType_t kTaskPriority = 5U;
constexpr const char* kTag = "ENV_MEASURE";
constexpr size_t kMaxPhysicalSensors = 2U;

static TickType_t seconds_to_ticks(uint32_t seconds) {
    TickType_t ticks = pdMS_TO_TICKS(seconds * 1000U);
    if (ticks == 0U) {
        ticks = 1U;
    }

    return ticks;
}

static bool tick_elapsed(TickType_t now, TickType_t last, TickType_t interval) {
    return (now - last) >= interval;
}

class EnvironmentMeasurements {
public:
    EnvironmentMeasurements()
        : physical_sensors_{&real_primary_, &real_alternate_}, active_inside_(&simulator_) {
    }

    EnvironmentMeasurements(const EnvironmentMeasurements&) = delete;
    EnvironmentMeasurements& operator=(const EnvironmentMeasurements&) = delete;
    EnvironmentMeasurements(EnvironmentMeasurements&&) = delete;
    EnvironmentMeasurements& operator=(EnvironmentMeasurements&&) = delete;

    esp_err_t init() {
        if (initialized_) {
            return ESP_OK;
        }

        reset_store();

        esp_err_t rv = board_i2c_init();
        if (rv != ESP_OK) {
            ESP_LOGE(kTag, "board_i2c_init failed: %s", esp_err_to_name(rv));
            return rv;
        }

        for (EnvironmentSensor* sensor : physical_sensors_) {
            (void)sensor->init();
        }

        rv = simulator_.init();
        if (rv != ESP_OK) {
            return rv;
        }

        environment_measurement_sample_t unused = {};
        EnvironmentSensor* discovered_sensor = nullptr;
        if (try_read_physical(unused, discovered_sensor)) {
            active_inside_ = discovered_sensor;
        } else {
            active_inside_ = &simulator_;
        }
        log_active_sensor();

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

        active_inside_ = &simulator_;
        reset_store();
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
        const int64_t now_ms = esp_timer_get_time() / kUsPerMs;

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
        const TickType_t reading_ticks =
            seconds_to_ticks(CONFIG_REDMOLE_ENVIRONMENT_READING_INTERVAL_SEC);
        const TickType_t detect_ticks =
            seconds_to_ticks(CONFIG_REDMOLE_ENVIRONMENT_DETECT_INTERVAL_SEC);
        TickType_t last_wake = xTaskGetTickCount();
        TickType_t last_publish = last_wake - reading_ticks;

        ESP_LOGI(kTag, "environment task cadence: reading=%ds detect=%ds active=%s",
                 CONFIG_REDMOLE_ENVIRONMENT_READING_INTERVAL_SEC,
                 CONFIG_REDMOLE_ENVIRONMENT_DETECT_INTERVAL_SEC, sensor_name(active_inside_));

        while (true) {
            const TickType_t now = xTaskGetTickCount();
            const bool published_on_detection = update_sensor_presence(now, last_publish);

            if (!published_on_detection && tick_elapsed(now, last_publish, reading_ticks)) {
                if (read_and_publish()) {
                    last_publish = xTaskGetTickCount();
                }
            }

            vTaskDelayUntil(&last_wake, detect_ticks);
        }
    }

    bool update_sensor_presence(TickType_t now, TickType_t& last_publish) {
        environment_measurement_sample_t sample = {};
        EnvironmentSensor* sensor = nullptr;

        if (active_inside_->is_simulated()) {
            if (!try_read_physical(sample, sensor)) {
                return false;
            }

            active_inside_ = sensor;
            ESP_LOGI(kTag, "physical environment sensor detected: %s; using hardware readings",
                     sensor_name(active_inside_));
            if (publish(sample) == ESP_OK) {
                last_publish = now;
                return true;
            }

            return false;
        }

        if (active_inside_->probe()) {
            return false;
        }

        ESP_LOGW(kTag, "active physical environment sensor lost: %s", sensor_name(active_inside_));

        if (try_read_physical(sample, sensor)) {
            active_inside_ = sensor;
            ESP_LOGI(kTag, "switched to alternate physical environment sensor: %s",
                     sensor_name(active_inside_));
            if (publish(sample) == ESP_OK) {
                last_publish = now;
                return true;
            }

            return false;
        }

        active_inside_ = &simulator_;
        ESP_LOGW(kTag, "physical environment sensor unplugged; falling back to simulation: %s",
                 sensor_name(active_inside_));
        esp_err_t rv = active_inside_->read(sample);
        if (rv != ESP_OK) {
            ESP_LOGW(kTag, "simulated environment sensor read failed: %s", esp_err_to_name(rv));
            return false;
        }

        if (publish(sample) == ESP_OK) {
            last_publish = now;
            return true;
        }

        return false;
    }

    bool read_and_publish() {
        environment_measurement_sample_t sample = {};

        esp_err_t rv = active_inside_->read(sample);
        if ((rv != ESP_OK) && !active_inside_->is_simulated()) {
            ESP_LOGW(kTag, "physical environment sensor read failed for %s: %s",
                     sensor_name(active_inside_), esp_err_to_name(rv));
            active_inside_ = &simulator_;
            ESP_LOGW(kTag, "physical environment sensor unplugged; falling back to simulation: %s",
                     sensor_name(active_inside_));
            rv = active_inside_->read(sample);
        }

        if (rv != ESP_OK) {
            ESP_LOGW(kTag, "environment sensor read failed: %s", esp_err_to_name(rv));
            return false;
        }

        rv = publish(sample);
        if (rv != ESP_OK) {
            ESP_LOGE(kTag, "publish failed: %s", esp_err_to_name(rv));
            return false;
        }

        return true;
    }

    bool try_read_physical(environment_measurement_sample_t& sample, EnvironmentSensor*& out_sensor) {
        for (EnvironmentSensor* sensor : physical_sensors_) {
            if (sensor->read(sample) == ESP_OK) {
                out_sensor = sensor;
                return true;
            }
        }

        out_sensor = nullptr;
        return false;
    }

    void log_active_sensor() const {
        if (active_inside_->is_simulated()) {
            ESP_LOGW(kTag, "BME280 not detected; using simulated inside environment sensor: %s",
                     sensor_name(active_inside_));
        } else {
            ESP_LOGI(kTag, "BME280 detected; using physical inside environment sensor: %s",
                     sensor_name(active_inside_));
        }
    }

    const char* sensor_name(const EnvironmentSensor* sensor) const {
        if (sensor == &real_primary_) {
            return "BME280@0x76";
        }

        if (sensor == &real_alternate_) {
            return "BME280@0x77";
        }

        if (sensor == &simulator_) {
            return "simulated BME280";
        }

        return "unknown sensor";
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

    Bme280Sensor real_primary_{0x76U};
    Bme280Sensor real_alternate_{0x77U};
    SimulatedBme280Sensor simulator_{};
    std::array<EnvironmentSensor*, kMaxPhysicalSensors> physical_sensors_{};
    EnvironmentSensor* active_inside_ = nullptr;
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
