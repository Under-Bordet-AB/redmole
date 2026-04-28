#ifndef GUI_STATE_H
#define GUI_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "gui_defs.h"

typedef struct {
    gui_panel_id_t active_panel;
    gui_sensor_state_t sensor;
    gui_wifi_settings_t wifi;
    gui_wifi_state_t wifi_state;
    gui_bluetooth_state_t bluetooth_state;
    gui_sd_card_state_t sd_card_state;
    gui_appearance_settings_t appearance;
} gui_state_t;

void gui_state_init(gui_state_t *state);
bool gui_state_set_active_panel(gui_state_t *state, gui_panel_id_t panel);
bool gui_state_set_sensor(gui_state_t *state, const gui_sensor_state_t *sensor);
bool gui_state_set_wifi_settings(gui_state_t *state, const gui_wifi_settings_t *wifi);
bool gui_state_set_wifi_state(gui_state_t *state, gui_wifi_state_t wifi_state);
bool gui_state_set_bluetooth_state(gui_state_t *state, gui_bluetooth_state_t bluetooth_state);
bool gui_state_set_sd_card_state(gui_state_t *state, gui_sd_card_state_t sd_card_state);
bool gui_state_set_theme(gui_state_t *state, gui_view_theme_t theme);
bool gui_state_set_background_image_enabled(gui_state_t *state, bool enabled);
bool gui_state_set_night_variant_enabled(gui_state_t *state, bool enabled);

void gui_state_scan_wifi(gui_state_t *state);
bool gui_state_select_wifi_network(gui_state_t *state, uint8_t network_index);
bool gui_state_connect_known_wifi(gui_state_t *state, uint8_t network_index);
bool gui_state_set_wifi_password(gui_state_t *state, const char *password);
bool gui_state_connect_wifi(gui_state_t *state);
bool gui_state_disconnect_wifi(gui_state_t *state);
int8_t gui_state_find_known_wifi_network(const gui_state_t *state, const char *ssid);

void gui_state_build_screen_model(const gui_state_t *state, gui_view_model_t *model);

#endif
