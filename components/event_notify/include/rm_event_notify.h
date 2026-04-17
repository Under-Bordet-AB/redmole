#ifndef RM_EVENT_NOTIFY_H
#define RM_EVENT_NOTIFY_H

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_err.h"

#define RM_EVENT_NOTIFY_BIT_INIT_DONE     BIT0
#define RM_EVENT_NOTIFY_BIT_SENSOR_READY  BIT1
#define RM_EVENT_NOTIFY_BIT_GUI_READY     BIT2

esp_err_t rm_event_notify_init(void);
void rm_event_notify_deinit(void);
esp_err_t rm_event_notify_signal(EventBits_t bits);
bool rm_event_notify_wait_all(EventBits_t bits, TickType_t timeout);
void rm_event_notify_wait(EventBits_t bits);

#endif // RM_EVENT_NOTIFY_H
