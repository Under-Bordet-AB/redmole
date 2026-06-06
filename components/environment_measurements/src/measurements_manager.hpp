#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "measurement_producer.hpp"
#include "measurement_store.hpp"

namespace redmole::environment {

using NowMilliseconds = int64_t (*)();

struct ProducerRegistration {
    const char* diagnostic_name;
    MeasurementProducer& producer;
    const MeasurementChannel* channels;
    size_t channel_count;
};

class MeasurementsManager {
  public:
    MeasurementsManager(const ProducerRegistration* registrations, size_t registration_count,
                        MeasurementStore& store, NowMilliseconds now_ms);

    esp_err_t init();
    esp_err_t start();
    void stop();
    esp_err_t poll_once();

  private:
    static constexpr size_t kMaxProducerCount = 4U;
    static constexpr uint32_t kTaskStackDepth = 4096U;

    static void task_entry(void* context);
    void task_loop();
    bool registrations_are_valid() const;
    bool batch_belongs_to(const MeasurementBatch& batch,
                          const ProducerRegistration& registration) const;
    void mark_failed(size_t producer_index, esp_err_t error);
    void mark_recovered(size_t producer_index);

    const ProducerRegistration* registrations_;
    size_t registration_count_;
    MeasurementStore& store_;
    NowMilliseconds now_ms_;
    std::array<bool, kMaxProducerCount> producer_failed_ = {};
    StaticSemaphore_t stopped_storage_ = {};
    SemaphoreHandle_t stopped_ = nullptr;
    StaticTask_t task_control_block_ = {};
    StackType_t task_stack_[kTaskStackDepth] = {};
    TaskHandle_t task_ = nullptr;
    std::atomic_bool stop_requested_{false};
    bool initialized_ = false;
    bool running_ = false;
};

} // namespace redmole::environment
