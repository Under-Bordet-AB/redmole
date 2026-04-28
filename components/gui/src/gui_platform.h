#ifndef GUI_PLATFORM_H
#define GUI_PLATFORM_H

#include <stdint.h>

#include "esp_err.h"

#include "gui_internal.h"

void gui_platform_set_brightness(int32_t brightness_percent);
int32_t gui_platform_get_brightness(void);
esp_err_t gui_platform_init_display(void);
void gui_platform_start_refresh_timer(gui_ctx_t *self, gui_runtime_t *runtime);
void gui_platform_stop_refresh_timer(gui_runtime_t *runtime);

#endif
