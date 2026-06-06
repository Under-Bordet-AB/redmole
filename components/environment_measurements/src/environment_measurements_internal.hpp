#pragma once

/**
 * @file
 * @brief Injectable controller behind the environment measurements public API.
 */

#include <atomic>
#include <cstdint>

#include "environment_measurements.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "temperature_humidity_pressure_source.hpp"

namespace redmole::environment {

using NowMilliseconds = int64_t (*)();

class EnvironmentMeasurements {
  public:
    EnvironmentMeasurements(const char* source_name, TemperatureHumidityPressureSource& source,
                            NowMilliseconds now_ms);

    esp_err_t init();
    esp_err_t start();
    void stop();
    bool get_latest(environment_measurement_sample_t* out) const;
    bool is_fresh(uint32_t max_age_ms) const;
    uint32_t get_update_count() const;

    /** @brief Perform one source read synchronously. */
    esp_err_t poll_once();

  private:
    static void task_entry(void* context);
    void task_loop();
    void publish(const TemperatureHumidityPressureReading& reading);
    void invalidate_snapshot();
    static bool sample_is_fresh(const environment_measurement_sample_t& sample, int64_t current_ms,
                                int64_t max_age_ms);

    const char* source_name_;
    TemperatureHumidityPressureSource& source_;
    NowMilliseconds now_ms_;
    mutable StaticSemaphore_t latest_mutex_storage_ = {};
    mutable SemaphoreHandle_t latest_mutex_ = nullptr;
    StaticSemaphore_t stopped_storage_ = {};
    SemaphoreHandle_t stopped_ = nullptr;
    StaticTask_t task_control_block_ = {};
    static constexpr uint32_t kTaskStackDepth = 4096U;
    StackType_t task_stack_[kTaskStackDepth] = {};
    TaskHandle_t task_ = nullptr;
    environment_measurement_sample_t latest_ = {};
    std::atomic_uint update_count_{0U};
    std::atomic_bool stop_requested_{false};
    bool initialized_ = false;
    bool running_ = false;
    bool read_failed_ = false;
};

} // namespace redmole::environment
