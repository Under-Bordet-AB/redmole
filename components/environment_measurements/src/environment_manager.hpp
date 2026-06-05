#pragma once

/**
 * @file
 * @brief Source lifecycle, candidate storage, and selection policy.
 *
 * EnvironmentManager is independent of FreeRTOS task scheduling and receives
 * explicit monotonic time so policy behavior can be tested deterministically.
 */

#include <array>
#include <cstddef>
#include <cstdint>

#include "environment_measurements.h"
#include "environment_sensor.hpp"
#include "esp_err.h"

namespace redmole::environment {

/** @brief Selection-policy category assigned by product composition. */
enum class SourceKind : uint8_t {
    Hardware,
    Simulation,
};

/** @brief Runtime lifecycle state owned by EnvironmentManager. */
enum class SourceState : uint8_t {
    Disabled,    /*!< Permanent setup failed; do not retry at runtime. */
    Unavailable, /*!< Source resources exist, but hardware is not detected. */
    Activating,  /*!< Detected source requires configuration before polling. */
    Active,      /*!< Source is configured and eligible for polling. */
    Failed,      /*!< Activation or polling failed; retry after a delay. */
};

/** @brief Product-owned source metadata and manager-owned runtime state. */
struct SourceBinding {
    EnvironmentSource* source;      /*!< Non-owning process-lifetime source pointer. */
    const char* name;               /*!< Non-owning process-lifetime diagnostic name. */
    EnvironmentLocation location;   /*!< Product location supplied by this source. */
    SourceKind kind;                /*!< Hardware or simulation selection category. */
    uint8_t priority;               /*!< Lower value wins within the same source kind. */
    SourceState state;              /*!< Current manager-owned lifecycle state. */
    int64_t retry_at_ms;            /*!< Earliest monotonic retry time in milliseconds. */
};

/** @brief Latest value reported by one binding for one capability. */
struct CandidateMeasurement {
    EnvironmentValue value; /*!< Canonically scaled candidate value. */
    int64_t timestamp_ms;    /*!< Monotonic acquisition time in milliseconds. */
    bool valid;              /*!< True after a complete successful source batch. */
};

/** @brief Preferred candidate selected for one location and capability. */
struct SelectedMeasurement {
    EnvironmentValue value; /*!< Canonically scaled selected value. */
    int64_t timestamp_ms;    /*!< Selected candidate acquisition time in milliseconds. */
    size_t binding_index;    /*!< Index of the selected source binding. */
    bool valid;              /*!< True while the selected candidate remains eligible. */
};

/**
 * @brief Callback invoked synchronously when a binding changes state.
 * @param binding Binding containing the new state and process-lifetime metadata.
 * @param previous_state State before the transition.
 * @param state State after the transition.
 */
using SourceStateChangedCallback =
    void (*)(const SourceBinding& binding, SourceState previous_state, SourceState state);

/**
 * @brief Deterministic controller for source lifecycle and value selection.
 *
 * The caller owns the binding array and every referenced source for the
 * manager's complete lifetime. Methods are not internally synchronized.
 */
class EnvironmentManager {
  public:
    static constexpr size_t kMaxBindings = 4U;                  /*!< Fixed binding capacity. */
    static constexpr int64_t kStaleTimeoutMs = 5000LL;         /*!< Candidate lifetime in ms. */
    static constexpr int64_t kMaximumAcquisitionAgeMs = 60000LL; /*!< Oldest accepted value, ms. */
    static constexpr int64_t kMaximumPollTimestampLeadMs = 1000LL; /*!< Allowed poll duration, ms. */
    static constexpr int64_t kRetryDelayMs = 2000LL;           /*!< Failed-source retry delay, ms. */

    /**
     * @brief Construct a manager over caller-owned fixed bindings.
     * @param bindings Non-NULL binding array that outlives the manager.
     * @param binding_count Number of bindings, up to kMaxBindings.
     */
    EnvironmentManager(SourceBinding* bindings, size_t binding_count);

    /**
     * @brief Register an optional synchronous source-state diagnostic callback.
     * @param callback Callback invoked during state changes, or NULL to disable callbacks.
     */
    void set_state_changed_callback(SourceStateChangedCallback callback);

    /**
     * @brief Initialize permanent source resources and initial states.
     * @param now_ms Current monotonic time in milliseconds.
     * @return ESP_OK on valid manager configuration, otherwise an error code.
     */
    esp_err_t init_sources(int64_t now_ms);

    /**
     * @brief Run due probe, retry, and activation state transitions.
     * @param now_ms Current monotonic time in milliseconds.
     */
    void maintain_sources(int64_t now_ms);

    /**
     * @brief Poll every active source once and atomically commit valid batches.
     * @param now_ms Monotonic time captured immediately before polling.
     */
    void poll_active_sources(int64_t now_ms);

    /**
     * @brief Recompute preferred values using current freshness and source states.
     * @param now_ms Current monotonic time in milliseconds.
     */
    void refresh_selection(int64_t now_ms);

    /**
     * @brief Copy one fresh selected value.
     * @param location Requested product location.
     * @param capability Requested environmental capability.
     * @param now_ms Current monotonic time in milliseconds.
     * @param out Output measurement, unchanged when no value is selected.
     * @return True when a fresh selected value was copied, false otherwise.
     */
    bool copy_selected(EnvironmentLocation location,
                       EnvironmentCapability capability,
                       int64_t now_ms,
                       EnvironmentMeasurement& out);
    /**
     * @brief Copy the complete preferred indoor compatibility payload.
     * @param now_ms Current monotonic time in milliseconds.
     * @param out Output payload, invalidated when any required value is missing.
     * @return True when all compatibility values are fresh, false otherwise.
     */
    bool copy_compatibility(int64_t now_ms, environment_measurement_sample_t& out);

    /**
     * @brief Return the next time selected state can become stale.
     * @return Nearest selected-value stale deadline in monotonic milliseconds.
     */
    int64_t next_freshness_deadline_ms() const;

  private:
    static constexpr size_t kCapabilityCount =
        static_cast<size_t>(EnvironmentCapability::Count);
    static constexpr size_t kLocationCount = static_cast<size_t>(EnvironmentLocation::Count);

    bool batch_is_valid(const MeasurementBatch& batch, int64_t now_ms) const;
    void commit(size_t binding_index, const MeasurementBatch& batch);
    void set_state(size_t binding_index, SourceState state, int64_t now_ms);
    void select(EnvironmentLocation location, EnvironmentCapability capability, int64_t now_ms);
    bool candidate_is_fresh(const CandidateMeasurement& candidate, int64_t now_ms) const;

    SourceBinding* bindings_;
    size_t binding_count_;
    SourceStateChangedCallback state_changed_callback_ = nullptr;
    std::array<std::array<CandidateMeasurement, kCapabilityCount>, kMaxBindings> candidates_{};
    std::array<std::array<SelectedMeasurement, kCapabilityCount>, kLocationCount> selected_{};
    MeasurementBatch poll_batch_{};
};

} // namespace redmole::environment
