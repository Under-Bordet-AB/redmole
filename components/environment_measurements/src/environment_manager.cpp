#include "environment_manager.hpp"

#include <algorithm>

namespace redmole::environment {

EnvironmentManager::EnvironmentManager(SourceBinding* bindings, size_t binding_count)
    : bindings_(bindings),
      binding_count_(std::min(binding_count, kMaxBindings)) {
}

void EnvironmentManager::set_state_changed_callback(SourceStateChangedCallback callback) {
    state_changed_callback_ = callback;
}

esp_err_t EnvironmentManager::init_sources(int64_t now_ms) {
    if ((bindings_ == nullptr) || (binding_count_ == 0U) || (binding_count_ > kMaxBindings)) {
        return ESP_ERR_INVALID_ARG;
    }

    for (size_t index = 0U; index < binding_count_; index++) {
        SourceBinding& binding = bindings_[index];
        if ((binding.source == nullptr) || (binding.source->init() != ESP_OK)) {
            set_state(index, SourceState::Disabled, now_ms);
        } else if (binding.source->probe()) {
            set_state(index, SourceState::Activating, now_ms);
        } else {
            set_state(index, SourceState::Unavailable, now_ms);
        }
    }

    refresh_selection(now_ms);
    return ESP_OK;
}

void EnvironmentManager::maintain_sources(int64_t now_ms) {
    for (size_t index = 0U; index < binding_count_; index++) {
        SourceBinding& binding = bindings_[index];
        switch (binding.state) {
        case SourceState::Disabled:
            break;
        case SourceState::Unavailable:
        case SourceState::Failed:
            if ((now_ms >= binding.retry_at_ms) && binding.source->probe()) {
                set_state(index, SourceState::Activating, now_ms);
            }
            break;
        case SourceState::Activating:
            set_state(index,
                      binding.source->activate() == ESP_OK ? SourceState::Active
                                                          : SourceState::Failed,
                      now_ms);
            break;
        case SourceState::Active:
            break;
        }
    }

    refresh_selection(now_ms);
}

void EnvironmentManager::poll_active_sources(int64_t now_ms) {
    for (size_t index = 0U; index < binding_count_; index++) {
        SourceBinding& binding = bindings_[index];
        if (binding.state != SourceState::Active) {
            continue;
        }

        poll_batch_.clear();
        if ((binding.source->poll(poll_batch_) != ESP_OK) || (poll_batch_.size() == 0U) ||
            !batch_is_valid(poll_batch_, now_ms)) {
            set_state(index, SourceState::Failed, now_ms);
        } else {
            commit(index, poll_batch_);
        }
    }

    refresh_selection(now_ms);
}

void EnvironmentManager::refresh_selection(int64_t now_ms) {
    for (size_t location = 0U; location < kLocationCount; location++) {
        for (size_t capability = 0U; capability < kCapabilityCount; capability++) {
            select(static_cast<EnvironmentLocation>(location),
                   static_cast<EnvironmentCapability>(capability), now_ms);
        }
    }
}

bool EnvironmentManager::copy_selected(EnvironmentLocation location,
                                       EnvironmentCapability capability,
                                       int64_t now_ms,
                                       EnvironmentMeasurement& out) {
    if ((location >= EnvironmentLocation::Count) || (capability >= EnvironmentCapability::Count)) {
        return false;
    }

    select(location, capability, now_ms);
    const SelectedMeasurement& selected =
        selected_[static_cast<size_t>(location)][static_cast<size_t>(capability)];
    if (!selected.valid) {
        return false;
    }

    out.capability = capability;
    out.value = selected.value;
    out.timestamp_ms = selected.timestamp_ms;
    return true;
}

bool EnvironmentManager::copy_compatibility(int64_t now_ms, environment_measurement_sample_t& out) {
    refresh_selection(now_ms);

    const auto& temperature =
        selected_[static_cast<size_t>(EnvironmentLocation::Indoor)]
                 [static_cast<size_t>(EnvironmentCapability::Temperature)];
    const auto& humidity =
        selected_[static_cast<size_t>(EnvironmentLocation::Indoor)]
                 [static_cast<size_t>(EnvironmentCapability::Humidity)];
    const auto& pressure =
        selected_[static_cast<size_t>(EnvironmentLocation::Indoor)]
                 [static_cast<size_t>(EnvironmentCapability::Pressure)];

    out = {};
    if (!temperature.valid || !humidity.valid || !pressure.valid) {
        return false;
    }

    out.temperature_deci_c = temperature.value.temperature_deci_c;
    out.humidity_deci_pct = humidity.value.humidity_deci_pct;
    out.pressure_deci_hpa = pressure.value.pressure_deci_hpa;
    out.timestamp_ms = std::min({temperature.timestamp_ms, humidity.timestamp_ms,
                                 pressure.timestamp_ms});
    out.valid = true;
    return true;
}

int64_t EnvironmentManager::next_freshness_deadline_ms() const {
    int64_t deadline = INT64_MAX;
    for (const auto& location : selected_) {
        for (const SelectedMeasurement& selected : location) {
            if (selected.valid) {
                deadline = std::min(deadline, selected.timestamp_ms + kStaleTimeoutMs + 1LL);
            }
        }
    }

    return deadline;
}

bool EnvironmentManager::batch_is_valid(const MeasurementBatch& batch, int64_t now_ms) const {
    for (size_t index = 0U; index < batch.size(); index++) {
        const EnvironmentMeasurement& measurement = batch[index];
        if (!validate_measurement(measurement) ||
            ((measurement.timestamp_ms - now_ms) > kMaximumPollTimestampLeadMs) ||
            ((now_ms - measurement.timestamp_ms) > kMaximumAcquisitionAgeMs)) {
            return false;
        }
    }

    return true;
}

void EnvironmentManager::commit(size_t binding_index, const MeasurementBatch& batch) {
    auto& binding_candidates = candidates_[binding_index];
    for (CandidateMeasurement& candidate : binding_candidates) {
        candidate.valid = false;
    }

    for (size_t index = 0U; index < batch.size(); index++) {
        const EnvironmentMeasurement& measurement = batch[index];
        CandidateMeasurement& candidate =
            binding_candidates[static_cast<size_t>(measurement.capability)];
        candidate.value = measurement.value;
        candidate.timestamp_ms = measurement.timestamp_ms;
        candidate.valid = true;
    }
}

void EnvironmentManager::set_state(size_t binding_index, SourceState state, int64_t now_ms) {
    SourceBinding& binding = bindings_[binding_index];
    const SourceState previous_state = binding.state;
    binding.state = state;
    binding.retry_at_ms =
        (state == SourceState::Failed || state == SourceState::Unavailable)
            ? now_ms + kRetryDelayMs
            : now_ms;
    if ((previous_state != state) && (state_changed_callback_ != nullptr)) {
        state_changed_callback_(binding, previous_state, state);
    }
}

void EnvironmentManager::select(EnvironmentLocation location,
                                EnvironmentCapability capability,
                                int64_t now_ms) {
    SelectedMeasurement selected{};
    bool selected_is_hardware = false;
    uint8_t selected_priority = UINT8_MAX;

    for (size_t index = 0U; index < binding_count_; index++) {
        const SourceBinding& binding = bindings_[index];
        const CandidateMeasurement& candidate =
            candidates_[index][static_cast<size_t>(capability)];
        if ((binding.location != location) || (binding.state != SourceState::Active) ||
            !candidate_is_fresh(candidate, now_ms)) {
            continue;
        }

        const bool is_hardware = binding.kind == SourceKind::Hardware;
        if (!selected.valid || (is_hardware && !selected_is_hardware) ||
            (is_hardware == selected_is_hardware && binding.priority < selected_priority)) {
            selected.value = candidate.value;
            selected.timestamp_ms = candidate.timestamp_ms;
            selected.binding_index = index;
            selected.valid = true;
            selected_is_hardware = is_hardware;
            selected_priority = binding.priority;
        }
    }

    selected_[static_cast<size_t>(location)][static_cast<size_t>(capability)] = selected;
}

bool EnvironmentManager::candidate_is_fresh(const CandidateMeasurement& candidate,
                                            int64_t now_ms) const {
    return candidate.valid && (candidate.timestamp_ms <= now_ms) &&
           ((now_ms - candidate.timestamp_ms) <= kStaleTimeoutMs);
}

} // namespace redmole::environment
