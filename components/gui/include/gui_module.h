#ifndef GUI_MODULE_H
#define GUI_MODULE_H

#include <stdbool.h>
#include "freertos/FreeRTOS.h"

#include "gui_types.h"

typedef struct gui_ctx {
    bool is_ready;
    void *frame_buffer; // Pointer to PSRAM allocated memory
    void *module_state;
} gui_ctx_t;

typedef struct {
    void *user_data;
    void (*on_panel_changed)(gui_ctx_t *self, gui_panel_id_t panel, void *user_data);
    bool (*on_wifi_scan_requested)(gui_ctx_t *self, void *user_data);
    void (*on_wifi_network_selected)(gui_ctx_t *self, const gui_wifi_network_t *network, void *user_data);
    bool (*on_wifi_known_network_requested)(gui_ctx_t *self, const gui_wifi_network_t *network, void *user_data);
    bool (*on_wifi_connect_requested)(gui_ctx_t *self, const char *ssid, const char *password, void *user_data);
    bool (*on_wifi_disconnect_requested)(gui_ctx_t *self, void *user_data);
} gui_module_bindings_t;

void gui_init(gui_ctx_t *self);
void gui_run(gui_ctx_t *self); // Task to process LVGL timers and render
void gui_deinit(gui_ctx_t *self);
void gui_set_bindings(gui_ctx_t *self, const gui_module_bindings_t *bindings);
void gui_refresh(gui_ctx_t *self);
void gui_set_active_panel(gui_ctx_t *self, gui_panel_id_t panel);
bool gui_get_active_panel(gui_ctx_t *self, gui_panel_id_t *panel);
void gui_set_sensor_state(gui_ctx_t *self, const gui_sensor_state_t *sensor);
bool gui_get_sensor_state(gui_ctx_t *self, gui_sensor_state_t *sensor);
void gui_set_wifi_settings(gui_ctx_t *self, const gui_wifi_settings_t *wifi);
bool gui_get_wifi_settings(gui_ctx_t *self, gui_wifi_settings_t *wifi);
void gui_show_wifi_network_dialog(gui_ctx_t *self);
void gui_show_wifi_password_dialog(gui_ctx_t *self);
void gui_hide_wifi_dialogs(gui_ctx_t *self);

#endif // GUI_MODULE_H
