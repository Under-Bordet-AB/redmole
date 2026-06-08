#pragma once

/**
 * @file
 * @brief Polling task and failure handling for registered measurement producers.
 */

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

/** @brief Injectable millisecond clock used for publication timestamps. */
using NowMillisecondsFunction = int64_t (*)();

/**
 * @brief Process-lifetime configuration for one producer.
 *
 * The manager borrows every referenced object and array. They must remain alive
 * for the complete manager lifetime.
 */
struct ProducerRegistration {
    const char* diagnostic_name;        /*!< Non-null name used only in logs. */
    MeasurementProducer& producer;      /*!< Producer borrowed by the manager. */
    const MeasurementChannel* channels; /*!< Channels exclusively owned by producer. */
    size_t channel_count;               /*!< Number of entries in channels. */
};

/**
 * @brief Initialize, poll, and publish a fixed list of measurement producers.
 *
 * One statically allocated FreeRTOS task polls producers sequentially. Producer
 * failures do not stop the task: owned channels are invalidated and later polls
 * retry the producer so disconnected hardware can recover.
 */
class MeasurementsManager {
  public:
    /**
     * @brief Construct a manager around borrowed process-lifetime dependencies.
     * @param registrations Fixed producer registration array.
     * @param registration_count Number of entries in registrations.
     * @param store Destination for successful measurement batches.
     * @param now_ms Clock called after each successful producer read.
     */
    MeasurementsManager(const ProducerRegistration* registrations, size_t registration_count,
                        MeasurementStore& store, NowMillisecondsFunction now_ms);

    /** @brief Validate registrations, initialize storage, and initialize every producer. */
    esp_err_t init();

    /** @brief Start or resume the statically allocated polling task. */
    esp_err_t start();

    /** @brief Block until the polling task acknowledges a cooperative stop request. */
    void stop();

    /** @brief Poll every producer once; primarily useful for deterministic tests. */
    esp_err_t poll_once();

  private:
    static constexpr size_t kMaxProducerCount = 4U;
    static constexpr uint32_t kTaskStackDepth = 4096U;

    static void task_entry(void* context);
    void task_loop();
    bool registrations_are_valid() const;
    bool batch_is_valid_for_registration(const MeasurementBatch& batch,
                                         const ProducerRegistration& registration) const;
    void mark_failed(size_t producer_index, esp_err_t error);
    void mark_recovered(size_t producer_index);

    const ProducerRegistration* registrations_; /*!< Borrowed fixed registration array. */
    size_t registration_count_;                 /*!< Number of registered producers. */
    MeasurementStore& store_;                   /*!< Borrowed publication destination. */
    NowMillisecondsFunction now_ms_;            /*!< Borrowed clock function. */
    std::array<bool, kMaxProducerCount> producer_failed_ = {}; /*!< Log suppression state. */
    StaticSemaphore_t stopped_storage_ = {}; /*!< FreeRTOS storage for stop acknowledgement. */
    SemaphoreHandle_t stopped_ = nullptr;    /*!< Signals that the task has stopped polling. */
    StaticTask_t task_control_block_ = {};   /*!< FreeRTOS task metadata storage. */
    StackType_t task_stack_[kTaskStackDepth] = {}; /*!< Statically allocated polling stack. */
    TaskHandle_t task_ = nullptr;            /*!< Created once, then suspended cooperatively. */
    std::atomic_bool stop_requested_{false}; /*!< Cross-task cooperative stop request. */
    bool initialized_ = false;               /*!< True after manager initialization succeeds. */
    bool running_ = false;                   /*!< Caller-side lifecycle state. */
};

} // namespace redmole::environment
