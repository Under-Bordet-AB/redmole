#ifndef GUI_VIEW_H
#define GUI_VIEW_H

#include "lvgl.h"

#include "../gui_defs.h"

typedef enum {
    GUI_VIEW_THEME_LIGHT = 0,
    GUI_VIEW_THEME_DARK,
} gui_view_theme_t;

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *sidebar;
    lv_obj_t *content;
    lv_obj_t *bme280_button;
    lv_obj_t *energy_plan_button;
    lv_obj_t *settings_button;
    lv_obj_t *header_title;
    lv_obj_t *header_subtitle;
    lv_obj_t *update_label;
    lv_obj_t *bme280_panel;
    lv_obj_t *energy_plan_panel;
    lv_obj_t *settings_panel;
    lv_obj_t *settings_home_panel;
    lv_obj_t *connectivity_card;
    lv_obj_t *other_settings_card;
    lv_obj_t *wifi_card;
    lv_obj_t *bluetooth_card;
    lv_obj_t *bluetooth_status_label;
    lv_obj_t *brightness_card;
    lv_obj_t *scan_button;
    lv_obj_t *theme_card;
    lv_obj_t *theme_dropdown;
    lv_obj_t *brightness_slider;
    lv_obj_t *brightness_value_label;
    lv_obj_t *wifi_status_label;
    lv_obj_t *wifi_selected_label;
    lv_obj_t *dialog_scrim;
    lv_obj_t *network_dialog;
    lv_obj_t *network_dialog_title;
    lv_obj_t *network_dialog_subtitle;
    lv_obj_t *network_empty_label;
    lv_obj_t *network_dialog_buttons[GUI_WIFI_NETWORK_COUNT];
    lv_obj_t *network_dialog_button_labels[GUI_WIFI_NETWORK_COUNT];
    lv_obj_t *network_dialog_cancel_button;
    lv_obj_t *password_dialog;
    lv_obj_t *password_dialog_title;
    lv_obj_t *password_dialog_network_label;
    lv_obj_t *wifi_password_textarea;
    lv_obj_t *wifi_keyboard;
    lv_obj_t *password_dialog_cancel_button;
    lv_obj_t *password_dialog_connect_button;
    lv_obj_t *temperature_value;
    lv_obj_t *humidity_value;
    lv_obj_t *pressure_value;
    lv_obj_t *status_line;
    lv_obj_t *energy_plan_chart;
    lv_chart_series_t *buy_series;
    lv_chart_series_t *solar_series;
    lv_chart_series_t *charge_series;
    lv_chart_series_t *sell_series;
    gui_panel_id_t last_active_panel;
    bool has_last_active_panel;
    gui_energy_plan_t last_energy_plan;
    bool has_last_energy_plan;
    gui_wifi_settings_t last_wifi_settings;
    bool has_last_wifi_settings;
    gui_view_theme_t current_theme;
} gui_view_t;

void gui_view_init(gui_view_t *view, const gui_view_model_t *model, lv_event_cb_t nav_event_cb,
                   lv_event_cb_t settings_event_cb, void *event_user_data);
void gui_view_apply(gui_view_t *view, const gui_view_model_t *model);
void gui_view_apply_theme(gui_view_t *view, gui_view_theme_t theme);
void gui_view_hide_wifi_dialogs(gui_view_t *view);
void gui_view_show_network_dialog(gui_view_t *view);
void gui_view_show_password_dialog(gui_view_t *view);

#endif