#ifndef GUI_CONTROL_H
#define GUI_CONTROL_H

#include "../gui_defs.h"

typedef struct {
    gui_panel_id_t active_panel;
    gui_sensor_state_t sensor;
    gui_wifi_settings_t wifi;
    gui_appearance_settings_t appearance;
} gui_control_t;

void gui_control_init(gui_control_t *control);
void gui_control_select_panel(gui_control_t *control, gui_panel_id_t panel);
void gui_control_scan_wifi(gui_control_t *control);
void gui_control_select_wifi_network(gui_control_t *control, uint8_t network_index);
void gui_control_connect_known_wifi(gui_control_t *control, uint8_t network_index);
void gui_control_set_wifi_password(gui_control_t *control, const char *password);
void gui_control_connect_wifi(gui_control_t *control);
void gui_control_build_model(gui_control_t *control, gui_view_model_t *model);

#endif