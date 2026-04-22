#include "gui_module.h"

#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "gui_module_internal.h"
#include "lvgl_port.h"

static bool gui_module_sensor_state_equals(const gui_sensor_state_t *left,
                                           const gui_sensor_state_t *right)
{
    if ((left == NULL) || (right == NULL)) {
        return false;
    }

    return (left->temperature_deci_c == right->temperature_deci_c) &&
           (left->humidity_deci_pct == right->humidity_deci_pct) &&
           (left->pressure_deci_hpa == right->pressure_deci_hpa) &&
           (left->is_fresh == right->is_fresh) &&
           (left->update_count == right->update_count);
}

static bool gui_module_wifi_network_equals(const gui_wifi_network_t *left,
                                           const gui_wifi_network_t *right)
{
    if ((left == NULL) || (right == NULL)) {
        return false;
    }

    return (strcmp(left->ssid, right->ssid) == 0) &&
           (left->signal_strength_pct == right->signal_strength_pct) &&
           (left->secured == right->secured);
}

static bool gui_module_wifi_settings_equals(const gui_wifi_settings_t *left,
                                            const gui_wifi_settings_t *right)
{
    uint8_t index;

    if ((left == NULL) || (right == NULL)) {
        return false;
    }

    if ((left->network_count != right->network_count) ||
        (left->known_network_count != right->known_network_count) ||
        (left->selected_network_index != right->selected_network_index) ||
        (left->selected_known_network_index != right->selected_known_network_index) ||
        (left->state != right->state) ||
        (strcmp(left->selected_ssid, right->selected_ssid) != 0) ||
        (strcmp(left->password, right->password) != 0) ||
        (strcmp(left->status_text, right->status_text) != 0)) {
        return false;
    }

    for (index = 0; index < GUI_WIFI_NETWORK_COUNT; index++) {
        if (!gui_module_wifi_network_equals(&left->networks[index], &right->networks[index])) {
            return false;
        }
    }

    for (index = 0; index < GUI_WIFI_KNOWN_NETWORK_COUNT; index++) {
        if (!gui_module_wifi_network_equals(&left->known_networks[index],
                                            &right->known_networks[index])) {
            return false;
        }
    }

    return true;
}

void gui_init(gui_ctx_t *self)
{
    gui_view_model_t model = { 0 };
    gui_module_runtime_t *runtime;

    ESP_RETURN_VOID_ON_FALSE(self != NULL, GUI_MODULE_TAG, "gui context is null");

    if (self->is_ready) {
        return;
    }

    runtime = gui_module_get_singleton();
    memset(runtime, 0, sizeof(*runtime));

    self->is_ready = false;
    self->frame_buffer = NULL;
    self->module_state = runtime;
    runtime->owner = self;

    ESP_ERROR_CHECK(gui_module_platform_init_display());

    gui_control_init(&runtime->control);
    gui_control_build_model(&runtime->control, &model);

    if (lvgl_port_lock(-1)) {
        gui_view_init(&runtime->view, &model, gui_module_event_nav_cb,
                      gui_module_event_settings_cb, runtime);
        gui_module_platform_start_refresh_timer(self, runtime);
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
    gui_module_runtime_t *runtime = gui_module_get_runtime(self);

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
    gui_module_runtime_t *runtime = gui_module_get_runtime(self);

    if ((runtime == NULL) || !lvgl_port_lock(-1)) {
        return;
    }

    gui_module_apply_model(runtime);
    lvgl_port_unlock();
}

void gui_set_active_panel(gui_ctx_t *self, gui_panel_id_t panel)
{
    gui_module_runtime_t *runtime = gui_module_get_runtime(self);

    if ((runtime == NULL) || !lvgl_port_lock(-1)) {
        return;
    }

    gui_control_select_panel(&runtime->control, panel);
    gui_module_apply_model(runtime);
    lvgl_port_unlock();
}

bool gui_get_active_panel(gui_ctx_t *self, gui_panel_id_t *panel)
{
    gui_module_runtime_t *runtime = gui_module_get_runtime(self);

    if ((runtime == NULL) || (panel == NULL)) {
        return false;
    }

    *panel = runtime->control.active_panel;
    return true;
}

void gui_set_sensor_state(gui_ctx_t *self, const gui_sensor_state_t *sensor)
{
    gui_module_runtime_t *runtime = gui_module_get_runtime(self);

    if ((runtime == NULL) || (sensor == NULL) || !lvgl_port_lock(-1)) {
        return;
    }

    if (gui_module_sensor_state_equals(&runtime->control.sensor, sensor)) {
        lvgl_port_unlock();
        return;
    }

    runtime->control.sensor = *sensor;
    gui_module_apply_model(runtime);
    lvgl_port_unlock();
}

bool gui_get_sensor_state(gui_ctx_t *self, gui_sensor_state_t *sensor)
{
    gui_module_runtime_t *runtime = gui_module_get_runtime(self);

    if ((runtime == NULL) || (sensor == NULL)) {
        return false;
    }

    *sensor = runtime->control.sensor;
    return true;
}

void gui_set_wifi_settings(gui_ctx_t *self, const gui_wifi_settings_t *wifi)
{
    gui_module_runtime_t *runtime = gui_module_get_runtime(self);

    if ((runtime == NULL) || (wifi == NULL) || !lvgl_port_lock(-1)) {
        return;
    }

    if (gui_module_wifi_settings_equals(&runtime->control.wifi, wifi)) {
        lvgl_port_unlock();
        return;
    }

    runtime->control.wifi = *wifi;
    gui_module_apply_model(runtime);
    lvgl_port_unlock();
}

bool gui_get_wifi_settings(gui_ctx_t *self, gui_wifi_settings_t *wifi)
{
    gui_module_runtime_t *runtime = gui_module_get_runtime(self);

    if ((runtime == NULL) || (wifi == NULL)) {
        return false;
    }

    *wifi = runtime->control.wifi;
    return true;
}

void gui_show_wifi_network_dialog(gui_ctx_t *self)
{
    gui_module_runtime_t *runtime = gui_module_get_runtime(self);

    if ((runtime == NULL) || !lvgl_port_lock(-1)) {
        return;
    }

    gui_view_show_network_dialog(&runtime->view);
    lvgl_port_unlock();
}

void gui_show_wifi_password_dialog(gui_ctx_t *self)
{
    gui_module_runtime_t *runtime = gui_module_get_runtime(self);

    if ((runtime == NULL) || !lvgl_port_lock(-1)) {
        return;
    }

    gui_view_show_password_dialog(&runtime->view);
    lvgl_port_unlock();
}

void gui_hide_wifi_dialogs(gui_ctx_t *self)
{
    gui_module_runtime_t *runtime = gui_module_get_runtime(self);

    if ((runtime == NULL) || !lvgl_port_lock(-1)) {
        return;
    }

    gui_view_hide_wifi_dialogs(&runtime->view);
    lvgl_port_unlock();
}

void gui_deinit(gui_ctx_t *self)
{
    gui_module_runtime_t *runtime;

    if (self == NULL) {
        return;
    }

    runtime = gui_module_get_runtime(self);
    gui_module_platform_stop_refresh_timer(runtime);

    self->is_ready = false;
    self->frame_buffer = NULL;
    self->module_state = NULL;
}