#include "rm_event_notify.h"

#include "freertos/event_groups.h"

static EventGroupHandle_t s_event_group = NULL;

esp_err_t rm_event_notify_init(void) {
    if (s_event_group != NULL) {
        return ESP_OK;
    }

    s_event_group = xEventGroupCreate();
    if (s_event_group == NULL) {
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

void rm_event_notify_deinit(void) {
    if (s_event_group != NULL) {
        vEventGroupDelete(s_event_group);
        s_event_group = NULL;
    }
}

esp_err_t rm_event_notify_signal(EventBits_t bits) {
    if (s_event_group == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    xEventGroupSetBits(s_event_group, bits);
    return ESP_OK;
}

bool rm_event_notify_wait_all(EventBits_t bits, TickType_t timeout) {
    EventBits_t actual_bits;

    if (s_event_group == NULL) {
        return false;
    }

    actual_bits = xEventGroupWaitBits(s_event_group, bits, pdFALSE, pdTRUE, timeout);
    return (actual_bits & bits) == bits;
}

void rm_event_notify_wait(EventBits_t bits) {
    if (s_event_group == NULL) {
        return;
    }

    xEventGroupWaitBits(s_event_group, bits, pdFALSE, pdTRUE, portMAX_DELAY);
}
