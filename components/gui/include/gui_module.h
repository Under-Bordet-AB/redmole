#ifndef GUI_MODULE_H
#define GUI_MODULE_H

#include "esp_err.h"

esp_err_t gui_init(void);
esp_err_t gui_start(void);
bool gui_is_ready(void);
void gui_deinit(void);

#endif // GUI_MODULE_H
