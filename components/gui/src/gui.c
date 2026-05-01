#include "gui_module.h"

#include <stdio.h>
#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "gui_internal.h"
#include "gui_platform.h"
#include "lvgl_port.h"
#include "view/gui_theme_defs.h"

static gui_runtime_t s_runtime;

static int32_t gui_clamp_brightness(int32_t brightness_percent)
{
    if (brightness_percent < 5) {
        return 5;
    }

    if (brightness_percent > 100) {
        return 100;
    }

    return brightness_percent;
}

static void gui_apply_init_config(gui_runtime_t *runtime,
                                  const gui_init_config_t *config)
{
    if ((runtime == NULL) || (config == NULL)) {
        return;
    }

    if (config->has_theme) {
        runtime->state.appearance.theme = config->theme;
    }

    if (config->has_background_image) {
        runtime->state.appearance.show_background_image =
            config->show_background_image;
    }

    if (config->has_night_variant) {
        runtime->state.appearance.night_variant_enabled =
            config->night_variant_enabled;
    }

    if (config->has_brightness) {
        gui_platform_set_brightness(
            gui_clamp_brightness(config->brightness_percent));
    }
}

static void gui_notify_panel_changed(gui_runtime_t *runtime, gui_panel_id_t panel)
{
    if ((runtime == NULL) || (runtime->owner == NULL) ||
        (runtime->bindings.on_panel_changed == NULL)) {
        return;
    }

    runtime->bindings.on_panel_changed(runtime->owner, panel, runtime->bindings.user_data);
}

static bool gui_request_wifi_scan(gui_runtime_t *runtime)
{
    if ((runtime == NULL) || (runtime->owner == NULL) ||
        (runtime->bindings.on_wifi_scan_requested == NULL)) {
        return false;
    }

    return runtime->bindings.on_wifi_scan_requested(runtime->owner,
                                                    runtime->bindings.user_data);
}

static void gui_notify_wifi_network_selected(gui_runtime_t *runtime,
                                             const gui_wifi_network_t *network)
{
    if ((runtime == NULL) || (runtime->owner == NULL) || (network == NULL) ||
        (runtime->bindings.on_wifi_network_selected == NULL)) {
        return;
    }

    runtime->bindings.on_wifi_network_selected(runtime->owner, network,
                                               runtime->bindings.user_data);
}

static bool gui_request_known_wifi(gui_runtime_t *runtime, const gui_wifi_network_t *network)
{
    if ((runtime == NULL) || (runtime->owner == NULL) || (network == NULL) ||
        (runtime->bindings.on_wifi_known_network_requested == NULL)) {
        return false;
    }

    return runtime->bindings.on_wifi_known_network_requested(runtime->owner, network,
                                                             runtime->bindings.user_data);
}

static bool gui_request_wifi_connect(gui_runtime_t *runtime, const char *ssid,
                                     const char *password)
{
    if ((runtime == NULL) || (runtime->owner == NULL) || (ssid == NULL) ||
        (runtime->bindings.on_wifi_connect_requested == NULL)) {
        return false;
    }

    return runtime->bindings.on_wifi_connect_requested(runtime->owner, ssid, password,
                                                       runtime->bindings.user_data);
}

static bool gui_request_wifi_disconnect(gui_runtime_t *runtime)
{
    if ((runtime == NULL) || (runtime->owner == NULL) ||
        (runtime->bindings.on_wifi_disconnect_requested == NULL)) {
        return false;
    }

    return runtime->bindings.on_wifi_disconnect_requested(runtime->owner,
                                                          runtime->bindings.user_data);
}

static gui_panel_id_t gui_screen_target_to_panel(gui_screen_t *screen, lv_obj_t *target)
{
    if ((screen == NULL) || (target == NULL)) {
        return GUI_PANEL_BME280;
    }

    if (target == screen->bme280_button) {
        return GUI_PANEL_BME280;
    }
    if (target == screen->energy_plan_button) {
        return GUI_PANEL_ENERGY_PLAN;
    }
    if (target == screen->forecast_button) {
        return GUI_PANEL_FORECAST;
    }
    if (target == screen->settings_button) {
        return GUI_PANEL_SETTINGS;
    }

    return (gui_panel_id_t)-1;
}

static void gui_handle_nav_event(lv_event_t *event)
{
    gui_runtime_t *runtime;
    gui_panel_id_t selected_panel;

    if (lv_event_get_code(event) != LV_EVENT_PRESSED) {
        return;
    }

    runtime = (gui_runtime_t *)lv_event_get_user_data(event);
    if (runtime == NULL) {
        return;
    }

    selected_panel = gui_screen_target_to_panel(&runtime->screen, lv_event_get_target(event));
    if ((int)selected_panel < 0) {
        return;
    }

    if (!gui_state_set_active_panel(&runtime->state, selected_panel)) {
        return;
    }

    gui_render_runtime(runtime);
    gui_notify_panel_changed(runtime, selected_panel);
}

static void gui_handle_theme_event(gui_runtime_t *runtime, lv_obj_t *target,
                                   lv_event_code_t event_code)
{
    if ((target == runtime->screen.theme_dropdown) &&
        (event_code == LV_EVENT_VALUE_CHANGED)) {
        uint16_t selected_theme = lv_dropdown_get_selected(runtime->screen.theme_dropdown);
        gui_view_theme_t theme;

        if (!gui_theme_dropdown_index_to_theme(selected_theme, &theme)) {
            return;
        }

        if (gui_state_set_theme(&runtime->state, theme)) {
            gui_render_runtime(runtime);
        }
        return;
    }

    if ((target == runtime->screen.theme_background_switch) &&
        (event_code == LV_EVENT_VALUE_CHANGED)) {
        if (gui_state_set_background_image_enabled(
                &runtime->state,
                lv_obj_has_state(runtime->screen.theme_background_switch,
                                 LV_STATE_CHECKED))) {
            gui_render_runtime(runtime);
        }
        return;
    }

    if ((target == runtime->screen.theme_night_switch) &&
        (event_code == LV_EVENT_VALUE_CHANGED)) {
        if (gui_state_set_night_variant_enabled(
                &runtime->state,
                lv_obj_has_state(runtime->screen.theme_night_switch, LV_STATE_CHECKED))) {
            gui_render_runtime(runtime);
        }
    }
}

static void gui_handle_brightness_event(gui_runtime_t *runtime, lv_obj_t *target,
                                        lv_event_code_t event_code)
{
    int32_t brightness_percent;

    if ((target != runtime->screen.brightness_slider) ||
        (event_code != LV_EVENT_VALUE_CHANGED)) {
        return;
    }

    brightness_percent = lv_slider_get_value(runtime->screen.brightness_slider);
    gui_platform_set_brightness(brightness_percent);
    gui_screen_sync_brightness(&runtime->screen, brightness_percent);
}

static void gui_handle_password_textarea_event(gui_runtime_t *runtime, lv_obj_t *target,
                                               lv_event_code_t event_code)
{
    if (target != runtime->screen.wifi_password_textarea) {
        return;
    }

    if ((event_code == LV_EVENT_PRESSED) || (event_code == LV_EVENT_CLICKED) ||
        (event_code == LV_EVENT_FOCUSED)) {
        lv_obj_add_state(runtime->screen.wifi_password_textarea, LV_STATE_FOCUSED);
        lv_keyboard_set_textarea(runtime->screen.wifi_keyboard,
                                 runtime->screen.wifi_password_textarea);
        lv_obj_clear_flag(runtime->screen.wifi_keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(runtime->screen.wifi_keyboard);
        return;
    }

    if ((event_code == LV_EVENT_READY) || (event_code == LV_EVENT_CANCEL)) {
        lv_obj_clear_state(runtime->screen.wifi_password_textarea, LV_STATE_FOCUSED);
        lv_indev_reset(NULL, runtime->screen.wifi_password_textarea);
        gui_state_set_wifi_password(
            &runtime->state,
            lv_textarea_get_text(runtime->screen.wifi_password_textarea));
        gui_render_runtime(runtime);
    }
}

static void gui_handle_wifi_action_event(gui_runtime_t *runtime, lv_obj_t *target,
                                         lv_event_code_t event_code)
{
    if ((target == runtime->screen.scan_button) && (event_code == LV_EVENT_CLICKED)) {
        gui_state_scan_wifi(&runtime->state);
        gui_request_wifi_scan(runtime);
        gui_render_runtime(runtime);
        gui_screen_show_network_dialog(&runtime->screen);
        return;
    }

    if ((target == runtime->screen.password_dialog_connect_button) &&
        (event_code == LV_EVENT_CLICKED)) {
        gui_state_set_wifi_password(
            &runtime->state,
            lv_textarea_get_text(runtime->screen.wifi_password_textarea));
        if (!gui_request_wifi_connect(runtime, runtime->state.wifi.selected_ssid,
                                      runtime->state.wifi.password)) {
            gui_state_connect_wifi(&runtime->state);
        }
        gui_render_runtime(runtime);
        return;
    }

    if (((target == runtime->screen.disconnect_button) ||
         (target == runtime->screen.password_dialog_disconnect_button)) &&
        (event_code == LV_EVENT_CLICKED)) {
        if (!gui_request_wifi_disconnect(runtime)) {
            gui_state_disconnect_wifi(&runtime->state);
        }
        gui_render_runtime(runtime);
        return;
    }

    if ((target == runtime->screen.network_dialog_cancel_button) &&
        (event_code == LV_EVENT_CLICKED)) {
        gui_screen_hide_wifi_dialogs(&runtime->screen);
        return;
    }

    if ((target == runtime->screen.password_dialog_cancel_button) &&
        (event_code == LV_EVENT_CLICKED)) {
        gui_screen_show_network_dialog(&runtime->screen);
    }
}

static bool gui_handle_network_dialog_selection(gui_runtime_t *runtime, lv_obj_t *target,
                                                lv_event_code_t event_code)
{
    uint8_t network_index;

    if (event_code != LV_EVENT_CLICKED) {
        return false;
    }

    for (network_index = 0; network_index < GUI_WIFI_NETWORK_COUNT; network_index++) {
        int8_t known_network_index;

        if (target != runtime->screen.network_dialog_buttons[network_index]) {
            continue;
        }

        known_network_index = gui_state_find_known_wifi_network(
            &runtime->state, runtime->state.wifi.networks[network_index].ssid);

        if (known_network_index >= 0) {
            if (!gui_request_known_wifi(
                    runtime,
                    &runtime->state.wifi.known_networks[(uint8_t)known_network_index])) {
                gui_state_connect_known_wifi(&runtime->state, (uint8_t)known_network_index);
                gui_render_runtime(runtime);
            }
        } else {
            gui_state_select_wifi_network(&runtime->state, network_index);
            gui_render_runtime(runtime);
            gui_screen_show_password_dialog(&runtime->screen);
            gui_notify_wifi_network_selected(runtime,
                                             &runtime->state.wifi.networks[network_index]);
        }
        return true;
    }

    return false;
}

static void gui_handle_settings_event(lv_event_t *event)
{
    gui_runtime_t *runtime;
    lv_event_code_t event_code;
    lv_obj_t *target;

    runtime = (gui_runtime_t *)lv_event_get_user_data(event);
    if (runtime == NULL) {
        return;
    }

    event_code = lv_event_get_code(event);
    target = lv_event_get_target(event);

    gui_handle_theme_event(runtime, target, event_code);
    gui_handle_brightness_event(runtime, target, event_code);
    gui_handle_password_textarea_event(runtime, target, event_code);
    gui_handle_wifi_action_event(runtime, target, event_code);
    (void)gui_handle_network_dialog_selection(runtime, target, event_code);
}

gui_runtime_t *gui_get_runtime(gui_ctx_t *self)
{
    if ((self == NULL) || (self->module_state == NULL)) {
        return NULL;
    }

    return (gui_runtime_t *)self->module_state;
}

void gui_render_runtime(gui_runtime_t *runtime)
{
    gui_screen_model_t model = { 0 };

    if (runtime == NULL) {
        return;
    }

    gui_state_build_screen_model(&runtime->state, &model);
    gui_screen_apply(&runtime->screen, &model);
}

void gui_init(gui_ctx_t *self, const gui_init_config_t *config)
{
    gui_screen_model_t model = { 0 };
    gui_runtime_t *runtime;

    ESP_RETURN_VOID_ON_FALSE(self != NULL, GUI_MODULE_TAG, "gui context is null");

    if (self->is_ready) {
        return;
    }

    runtime = &s_runtime;
    memset(runtime, 0, sizeof(*runtime));

    self->is_ready = false;
    self->frame_buffer = NULL;
    self->module_state = runtime;
    runtime->owner = self;

    ESP_ERROR_CHECK(gui_platform_init_display());

    gui_state_init(&runtime->state);
    gui_apply_init_config(runtime, config);
    gui_state_build_screen_model(&runtime->state, &model);

    if (lvgl_port_lock(-1)) {
        gui_screen_init(&runtime->screen, &model, gui_handle_nav_event,
                        gui_handle_settings_event, runtime);
        gui_platform_start_refresh_timer(self, runtime);
        gui_screen_sync_brightness(&runtime->screen, gui_platform_get_brightness());
        lvgl_port_unlock();
    }

    self->is_ready = true;
    ESP_LOGI(GUI_MODULE_TAG, "GUI BME280 screen initialized");
}

void gui_run(gui_ctx_t *self)
{
    (void)self;
}

void gui_set_bindings(gui_ctx_t *self, const gui_module_bindings_t *bindings)
{
    gui_runtime_t *runtime = gui_get_runtime(self);

    if (runtime == NULL) {
        return;
    }

    if (bindings == NULL) {
        memset(&runtime->bindings, 0, sizeof(runtime->bindings));
        return;
    }

    runtime->bindings = *bindings;
}

void gui_refresh(gui_ctx_t *self)
{
    gui_runtime_t *runtime = gui_get_runtime(self);

    if ((runtime == NULL) || !lvgl_port_lock(-1)) {
        return;
    }

    gui_render_runtime(runtime);
    lvgl_port_unlock();
}

void gui_set_active_panel(gui_ctx_t *self, gui_panel_id_t panel)
{
    gui_runtime_t *runtime = gui_get_runtime(self);

    if ((runtime == NULL) || !lvgl_port_lock(-1)) {
        return;
    }

    if (gui_state_set_active_panel(&runtime->state, panel)) {
        gui_render_runtime(runtime);
    }
    lvgl_port_unlock();
}

bool gui_get_active_panel(gui_ctx_t *self, gui_panel_id_t *panel)
{
    gui_runtime_t *runtime = gui_get_runtime(self);

    if ((runtime == NULL) || (panel == NULL)) {
        return false;
    }

    *panel = runtime->state.active_panel;
    return true;
}

void gui_set_sensor_state(gui_ctx_t *self, const gui_sensor_state_t *sensor)
{
    gui_runtime_t *runtime = gui_get_runtime(self);

    if ((runtime == NULL) || (sensor == NULL) || !lvgl_port_lock(-1)) {
        return;
    }

    if (gui_state_set_sensor(&runtime->state, sensor)) {
        gui_render_runtime(runtime);
    }
    lvgl_port_unlock();
}

bool gui_get_sensor_state(gui_ctx_t *self, gui_sensor_state_t *sensor)
{
    gui_runtime_t *runtime = gui_get_runtime(self);

    if ((runtime == NULL) || (sensor == NULL)) {
        return false;
    }

    *sensor = runtime->state.sensor;
    return true;
}

void gui_set_wifi_settings(gui_ctx_t *self, const gui_wifi_settings_t *wifi)
{
    gui_runtime_t *runtime = gui_get_runtime(self);

    if ((runtime == NULL) || (wifi == NULL) || !lvgl_port_lock(-1)) {
        return;
    }

    if (gui_state_set_wifi_settings(&runtime->state, wifi)) {
        gui_render_runtime(runtime);
    }
    lvgl_port_unlock();
}

bool gui_get_wifi_settings(gui_ctx_t *self, gui_wifi_settings_t *wifi)
{
    gui_runtime_t *runtime = gui_get_runtime(self);

    if ((runtime == NULL) || (wifi == NULL)) {
        return false;
    }

    *wifi = runtime->state.wifi;
    return true;
}

void gui_set_wifi_state(gui_ctx_t *self, gui_wifi_state_t state)
{
    gui_runtime_t *runtime = gui_get_runtime(self);

    if ((runtime == NULL) || !lvgl_port_lock(-1)) {
        return;
    }

    if (gui_state_set_wifi_state(&runtime->state, state)) {
        gui_render_runtime(runtime);
    }
    lvgl_port_unlock();
}

bool gui_get_wifi_state(gui_ctx_t *self, gui_wifi_state_t *state)
{
    gui_runtime_t *runtime = gui_get_runtime(self);

    if ((runtime == NULL) || (state == NULL)) {
        return false;
    }

    *state = runtime->state.wifi_state;
    return true;
}

void gui_set_bluetooth_state(gui_ctx_t *self, gui_bluetooth_state_t state)
{
    gui_runtime_t *runtime = gui_get_runtime(self);

    if ((runtime == NULL) || !lvgl_port_lock(-1)) {
        return;
    }

    if (gui_state_set_bluetooth_state(&runtime->state, state)) {
        gui_render_runtime(runtime);
    }
    lvgl_port_unlock();
}

bool gui_get_bluetooth_state(gui_ctx_t *self, gui_bluetooth_state_t *state)
{
    gui_runtime_t *runtime = gui_get_runtime(self);

    if ((runtime == NULL) || (state == NULL)) {
        return false;
    }

    *state = runtime->state.bluetooth_state;
    return true;
}

void gui_set_sd_card_state(gui_ctx_t *self, gui_sd_card_state_t state)
{
    gui_runtime_t *runtime = gui_get_runtime(self);

    if ((runtime == NULL) || !lvgl_port_lock(-1)) {
        return;
    }

    if (gui_state_set_sd_card_state(&runtime->state, state)) {
        gui_render_runtime(runtime);
    }
    lvgl_port_unlock();
}

bool gui_get_sd_card_state(gui_ctx_t *self, gui_sd_card_state_t *state)
{
    gui_runtime_t *runtime = gui_get_runtime(self);

    if ((runtime == NULL) || (state == NULL)) {
        return false;
    }

    *state = runtime->state.sd_card_state;
    return true;
}

void gui_set_appearance_settings(gui_ctx_t *self, const gui_appearance_settings_t *appearance) {
    gui_runtime_t *runtime = gui_get_runtime(self);
    bool changed = false;

    if ((runtime == NULL) || (appearance == NULL) || !lvgl_port_lock(-1)) {
        return;
    }

    changed |= gui_state_set_theme(&runtime->state, appearance->theme);
    changed |= gui_state_set_background_image_enabled(&runtime->state, appearance->show_background_image);
    changed |= gui_state_set_night_variant_enabled(&runtime->state, appearance->night_variant_enabled);

    if (changed) {
        gui_render_runtime(runtime);
    }

    lvgl_port_unlock();
}

bool gui_get_appearance_settings(gui_ctx_t *self, gui_appearance_settings_t *appearance) {
    gui_runtime_t *runtime = gui_get_runtime(self);

    if ((runtime == NULL) || (appearance == NULL)) {
        return false;
    }

    *appearance = runtime->state.appearance;
    return true;
}

void gui_set_brightness(gui_ctx_t *self, int32_t brightness_percent) {
    gui_runtime_t *runtime = gui_get_runtime(self);

    if ((runtime == NULL) || !lvgl_port_lock(-1)) {
        return;
    }

    brightness_percent = gui_clamp_brightness(brightness_percent);

    gui_platform_set_brightness(brightness_percent);
    gui_screen_sync_brightness(&runtime->screen, brightness_percent);
    lvgl_port_unlock();
}

bool gui_get_brightness(gui_ctx_t *self, int32_t *brightness_percent) {
    if (brightness_percent == NULL) {
        return false;
    }

    *brightness_percent = gui_platform_get_brightness();
    return true;
}

void gui_show_wifi_network_dialog(gui_ctx_t *self)
{
    gui_runtime_t *runtime = gui_get_runtime(self);

    if ((runtime == NULL) || !lvgl_port_lock(-1)) {
        return;
    }

    gui_screen_show_network_dialog(&runtime->screen);
    lvgl_port_unlock();
}

void gui_show_wifi_password_dialog(gui_ctx_t *self)
{
    gui_runtime_t *runtime = gui_get_runtime(self);

    if ((runtime == NULL) || !lvgl_port_lock(-1)) {
        return;
    }

    gui_screen_show_password_dialog(&runtime->screen);
    lvgl_port_unlock();
}

void gui_hide_wifi_dialogs(gui_ctx_t *self)
{
    gui_runtime_t *runtime = gui_get_runtime(self);

    if ((runtime == NULL) || !lvgl_port_lock(-1)) {
        return;
    }

    gui_screen_hide_wifi_dialogs(&runtime->screen);
    lvgl_port_unlock();
}

void gui_deinit(gui_ctx_t *self)
{
    gui_runtime_t *runtime;

    if (self == NULL) {
        return;
    }

    runtime = gui_get_runtime(self);
    gui_platform_stop_refresh_timer(runtime);

    self->is_ready = false;
    self->frame_buffer = NULL;
    self->module_state = NULL;
}
