#ifndef APP_GUI_BINDINGS_H
#define APP_GUI_BINDINGS_H

#include "esp_err.h"
#include "gui_module.h"

esp_err_t app_gui_bindings_init(gui_ctx_t *gui);
void app_gui_bindings_sync(gui_ctx_t *gui);

#endif