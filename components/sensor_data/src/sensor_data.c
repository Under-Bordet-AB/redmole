#include "sensor_data.h"
#include <string.h>
#include <stdatomic.h>
#include "esp_timer.h"

typedef struct {
    sensor_data_sample latest_local;
    atomic_uint version;
    atomic_uint local_update_count;
    bool initialized;
} sensor_data_state;

static sensor_data_state s_state = {0};

esp_err_t sensor_data_init(void) {
    if (s_state.initialized) {
        return ESP_OK;
    }

    memset(&s_state.latest_local, 0, sizeof(s_state.latest_local));
    atomic_init(&s_state.version, 0U);
    atomic_init(&s_state.local_update_count, 0U);
    s_state.initialized = true;
    return ESP_OK;
}

esp_err_t sensor_data_deinit(void) {
    if (!s_state.initialized) {
        return ESP_OK;
    }

    memset(&s_state, 0, sizeof(s_state));
    return ESP_OK;
}

esp_err_t sensor_data_submit_local(const sensor_data_sample* sample) {
    if (sample == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_state.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Odd version means write in progress, even means published snapshot. */
    atomic_fetch_add_explicit(&s_state.version, 1U, memory_order_relaxed);
    s_state.latest_local = *sample;
    atomic_fetch_add_explicit(&s_state.local_update_count, 1U, memory_order_relaxed);
    atomic_fetch_add_explicit(&s_state.version, 1U, memory_order_release);
    return ESP_OK;
}

bool sensor_data_get_latest_local(sensor_data_sample* out) {
    unsigned int version_before = 0U;
    unsigned int version_after = 0U;

    if (out == NULL || !s_state.initialized) {
        return false;
    }

    do {
        version_before = atomic_load_explicit(&s_state.version, memory_order_acquire);
        if ((version_before & 1U) != 0U) {
            continue;
        }

        *out = s_state.latest_local;
        version_after = atomic_load_explicit(&s_state.version, memory_order_acquire);
    } while (version_before != version_after);

    return out->valid;
}

bool sensor_data_is_local_fresh(uint32_t max_age_ms) {
    sensor_data_sample sample = {0};
    int64_t now_ms = esp_timer_get_time() / 1000LL;

    if (!sensor_data_get_latest_local(&sample)) {
        return false;
    }

    if (sample.timestamp_ms > now_ms) {
        return false;
    }

    return (uint64_t)(now_ms - sample.timestamp_ms) <= (uint64_t)max_age_ms;
}

uint32_t sensor_data_get_local_update_count(void) {
    if (!s_state.initialized) {
        return 0U;
    }

    return atomic_load_explicit(&s_state.local_update_count, memory_order_relaxed);
}
