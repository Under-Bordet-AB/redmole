#ifndef GUI_CONTROL_INTERNAL_H
#define GUI_CONTROL_INTERNAL_H

#include "gui_control.h"

void gui_control_copy_status(gui_wifi_settings_t *wifi, const char *text);
void gui_control_reset_wifi_scan(gui_control_t *control);
void gui_control_clear_energy_plan(gui_energy_plan_t *energy_plan);

#endif