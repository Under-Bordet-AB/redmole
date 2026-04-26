#include "gui_module_internal.h"

#include <stdio.h>
#include <string.h>

#include "../view/gui_theme_defs.h"

static void gui_module_notify_panel_changed(gui_module_runtime_t *runtime, gui_panel_id_t panel)
{
    if ((runtime == NULL) || (runtime->owner == NULL) ||
        (runtime->bindings.on_panel_changed == NULL)) {
        return;
    }

    runtime->bindings.on_panel_changed(runtime->owner, panel, runtime->bindings.user_data);
}

static bool gui_module_handle_wifi_scan_request(gui_module_runtime_t *runtime)
{
    if ((runtime == NULL) || (runtime->owner == NULL) ||
        (runtime->bindings.on_wifi_scan_requested == NULL)) {
        return false;
    }

    return runtime->bindings.on_wifi_scan_requested(runtime->owner, runtime->bindings.user_data);
}

static void gui_module_notify_wifi_network_selected(gui_module_runtime_t *runtime,
                                                    const gui_wifi_network_t *network)
{
    if ((runtime == NULL) || (runtime->owner == NULL) || (network == NULL) ||
        (runtime->bindings.on_wifi_network_selected == NULL)) {
        return;
    }

    runtime->bindings.on_wifi_network_selected(runtime->owner, network,
                                               runtime->bindings.user_data);
}

static bool gui_module_handle_known_wifi_request(gui_module_runtime_t *runtime,
                                                 const gui_wifi_network_t *network)
{
    if ((runtime == NULL) || (runtime->owner == NULL) || (network == NULL) ||
        (runtime->bindings.on_wifi_known_network_requested == NULL)) {
        return false;
    }

    return runtime->bindings.on_wifi_known_network_requested(runtime->owner, network,
                                                             runtime->bindings.user_data);
}

static bool gui_module_handle_wifi_connect_request(gui_module_runtime_t *runtime, const char *ssid,
                                                   const char *password)
{
    if ((runtime == NULL) || (runtime->owner == NULL) || (ssid == NULL) ||
        (runtime->bindings.on_wifi_connect_requested == NULL)) {
        return false;
    }

    return runtime->bindings.on_wifi_connect_requested(runtime->owner, ssid, password,
                                                       runtime->bindings.user_data);
}

static bool gui_module_handle_wifi_disconnect_request(gui_module_runtime_t *runtime)
{
    if ((runtime == NULL) || (runtime->owner == NULL) ||
        (runtime->bindings.on_wifi_disconnect_requested == NULL)) {
        return false;
    }

    return runtime->bindings.on_wifi_disconnect_requested(runtime->owner,
                                                          runtime->bindings.user_data);
}

void gui_module_event_nav_cb(lv_event_t *event)
{
    gui_module_runtime_t *runtime;
    lv_obj_t *target;
    gui_panel_id_t selected_panel;

    if (lv_event_get_code(event) != LV_EVENT_PRESSED) {
        return;
    }

    runtime = (gui_module_runtime_t *)lv_event_get_user_data(event);
    target = lv_event_get_target(event);
    if (runtime == NULL) {
        return;
    }

    if (target == runtime->view.bme280_button) {
        selected_panel = GUI_PANEL_BME280;
    } else if (target == runtime->view.energy_plan_button) {
        selected_panel = GUI_PANEL_ENERGY_PLAN;
    } else if (target == runtime->view.forecast_button) {
        selected_panel = GUI_PANEL_FORECAST;
    } else if (target == runtime->view.settings_button) {
        selected_panel = GUI_PANEL_SETTINGS;
    } else {
        return;
    }

    if (runtime->control.active_panel == selected_panel) {
        return;
    }

    gui_control_select_panel(&runtime->control, selected_panel);
    gui_module_apply_model(runtime);
    gui_module_notify_panel_changed(runtime, selected_panel);
}

void gui_module_event_settings_cb(lv_event_t *event)
{
    gui_module_runtime_t *runtime;
    lv_event_code_t event_code;
    lv_obj_t *target;
    char brightness_text[8];
    int8_t known_network_index;
    uint8_t network_index;

    runtime = (gui_module_runtime_t *)lv_event_get_user_data(event);
    if (runtime == NULL) {
        return;
    }

    event_code = lv_event_get_code(event);
    target = lv_event_get_target(event);

    if ((target == runtime->view.theme_dropdown) && (event_code == LV_EVENT_VALUE_CHANGED)) {
        uint16_t selected_theme = lv_dropdown_get_selected(runtime->view.theme_dropdown);
        gui_view_theme_t theme;

        if (!gui_theme_dropdown_index_to_theme(selected_theme, &theme)) {
            return;
        }

        runtime->control.appearance.theme = theme;
        gui_module_apply_model(runtime);
        return;
    }

    if ((target == runtime->view.theme_background_switch) &&
        (event_code == LV_EVENT_VALUE_CHANGED)) {
        runtime->control.appearance.show_background_image =
            lv_obj_has_state(runtime->view.theme_background_switch, LV_STATE_CHECKED);
        gui_module_apply_model(runtime);
        return;
    }

    if ((target == runtime->view.theme_night_switch) &&
        (event_code == LV_EVENT_VALUE_CHANGED)) {
        runtime->control.appearance.night_variant_enabled =
            lv_obj_has_state(runtime->view.theme_night_switch, LV_STATE_CHECKED);
        gui_module_apply_model(runtime);
        return;
    }

    if ((target == runtime->view.brightness_slider) && (event_code == LV_EVENT_VALUE_CHANGED)) {
        int32_t brightness_percent = lv_slider_get_value(runtime->view.brightness_slider);

        gui_module_apply_brightness(brightness_percent);
        if (runtime->view.brightness_value_label != NULL) {
            snprintf(brightness_text, sizeof(brightness_text), "%ld%%",
                     (long)brightness_percent);
            lv_label_set_text(runtime->view.brightness_value_label, brightness_text);
        }
        return;
    }

    if (target == runtime->view.wifi_password_textarea) {
        if ((event_code == LV_EVENT_PRESSED) || (event_code == LV_EVENT_CLICKED) ||
            (event_code == LV_EVENT_FOCUSED)) {
            lv_obj_add_state(runtime->view.wifi_password_textarea, LV_STATE_FOCUSED);
            lv_keyboard_set_textarea(runtime->view.wifi_keyboard,
                                     runtime->view.wifi_password_textarea);
            lv_obj_clear_flag(runtime->view.wifi_keyboard, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(runtime->view.wifi_keyboard);
        } else if ((event_code == LV_EVENT_READY) || (event_code == LV_EVENT_CANCEL)) {
            lv_obj_clear_state(runtime->view.wifi_password_textarea, LV_STATE_FOCUSED);
            lv_indev_reset(NULL, runtime->view.wifi_password_textarea);
            gui_control_set_wifi_password(
                &runtime->control, lv_textarea_get_text(runtime->view.wifi_password_textarea));
            gui_module_apply_model(runtime);
        }
        return;
    }

    if ((target == runtime->view.scan_button) && (event_code == LV_EVENT_CLICKED)) {
        gui_control_scan_wifi(&runtime->control);
        gui_module_handle_wifi_scan_request(runtime);
        gui_module_apply_model(runtime);
        gui_view_show_network_dialog(&runtime->view);
        return;
    }

    if ((target == runtime->view.password_dialog_connect_button) &&
        (event_code == LV_EVENT_CLICKED)) {
        gui_control_set_wifi_password(&runtime->control,
                                      lv_textarea_get_text(runtime->view.wifi_password_textarea));
        if (!gui_module_handle_wifi_connect_request(runtime, runtime->control.wifi.selected_ssid,
                                                    runtime->control.wifi.password)) {
            gui_control_connect_wifi(&runtime->control);
        }
        gui_module_apply_model(runtime);
        return;
    }

    if (((target == runtime->view.disconnect_button) ||
         (target == runtime->view.password_dialog_disconnect_button)) &&
        (event_code == LV_EVENT_CLICKED)) {
        if (!gui_module_handle_wifi_disconnect_request(runtime)) {
            gui_control_disconnect_wifi(&runtime->control);
        }
        gui_module_apply_model(runtime);
        return;
    }

    if ((target == runtime->view.network_dialog_cancel_button) &&
        (event_code == LV_EVENT_CLICKED)) {
        gui_view_hide_wifi_dialogs(&runtime->view);
        return;
    }

    if ((target == runtime->view.password_dialog_cancel_button) &&
        (event_code == LV_EVENT_CLICKED)) {
        gui_view_show_network_dialog(&runtime->view);
        return;
    }

    for (network_index = 0; network_index < GUI_WIFI_NETWORK_COUNT; network_index++) {
        if ((target == runtime->view.network_dialog_buttons[network_index]) &&
            (event_code == LV_EVENT_CLICKED)) {
            known_network_index = -1;
            for (uint8_t known_index = 0; known_index < runtime->control.wifi.known_network_count;
                 known_index++) {
                if (strcmp(runtime->control.wifi.networks[network_index].ssid,
                           runtime->control.wifi.known_networks[known_index].ssid) == 0) {
                    known_network_index = (int8_t)known_index;
                    break;
                }
            }

            if (known_network_index >= 0) {
                if (!gui_module_handle_known_wifi_request(
                        runtime, &runtime->control.wifi.known_networks[(uint8_t)known_network_index])) {
                    gui_control_connect_known_wifi(&runtime->control, (uint8_t)known_network_index);
                    gui_module_apply_model(runtime);
                }
            } else {
                gui_control_select_wifi_network(&runtime->control, network_index);
                gui_module_apply_model(runtime);
                gui_view_show_password_dialog(&runtime->view);
                gui_module_notify_wifi_network_selected(runtime,
                                                        &runtime->control.wifi.networks[network_index]);
            }
            return;
        }
    }
}
