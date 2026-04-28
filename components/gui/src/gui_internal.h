#ifndef GUI_INTERNAL_H
#define GUI_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "gui_module.h"
#include "gui_screen.h"
#include "gui_state.h"
#include "lvgl.h"

#define GUI_MODULE_TAG "gui"
#define GUI_MODULE_DEFAULT_BRIGHTNESS 82

typedef struct {
    gui_state_t state;
    gui_screen_t screen;
    lv_timer_t *refresh_timer;
    gui_ctx_t *owner;
    gui_module_bindings_t bindings;
} gui_runtime_t;

gui_runtime_t *gui_get_runtime(gui_ctx_t *self);
void gui_render_runtime(gui_runtime_t *runtime);

#endif
