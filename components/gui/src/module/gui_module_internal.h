#ifndef GUI_MODULE_INTERNAL_H
#define GUI_MODULE_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "gui_module.h"
#include "lvgl.h"

#include "../control/gui_control.h"
#include "../view/gui_view.h"

#define GUI_MODULE_TAG "gui"
#define GUI_MODULE_DEFAULT_BRIGHTNESS 82

typedef struct {
    gui_control_t control;
    gui_view_t view;
    lv_timer_t *refresh_timer;
    gui_ctx_t *owner;
    gui_module_bindings_t bindings;
} gui_module_runtime_t;

gui_module_runtime_t *gui_module_get_singleton(void);
gui_module_runtime_t *gui_module_get_runtime(gui_ctx_t *self);
void gui_module_apply_model(gui_module_runtime_t *runtime);

void gui_module_event_nav_cb(lv_event_t *event);
void gui_module_event_settings_cb(lv_event_t *event);
void gui_module_refresh_timer_cb(lv_timer_t *timer);

void gui_module_apply_brightness(int32_t brightness_percent);
esp_err_t gui_module_platform_init_display(void);
void gui_module_platform_start_refresh_timer(gui_ctx_t *self, gui_module_runtime_t *runtime);
void gui_module_platform_stop_refresh_timer(gui_module_runtime_t *runtime);

#endif