#include "gui_state.h"

#include <stdio.h>
#include <string.h>

#include "view/gui_theme_defs.h"

static void gui_state_copy_status(gui_wifi_settings_t *wifi, const char *text)
{
    if ((wifi == NULL) || (text == NULL)) {
        return;
    }

    snprintf(wifi->status_text, sizeof(wifi->status_text), "%s", text);
}

static void gui_state_reset_wifi_scan(gui_state_t *state)
{
    if (state == NULL) {
        return;
    }

    memset(state->wifi.networks, 0, sizeof(state->wifi.networks));
    memset(state->wifi.known_networks, 0, sizeof(state->wifi.known_networks));
    state->wifi.network_count = 0;
    state->wifi.known_network_count = 0;
}

static void gui_state_init_forecast(gui_forecast_state_t *forecast)
{
    if (forecast == NULL) {
        return;
    }

    memset(forecast, 0, sizeof(*forecast));
    snprintf(forecast->title, sizeof(forecast->title), "%s", "Today");
    snprintf(forecast->condition, sizeof(forecast->condition), "%s", "Mostly cloudy");
    snprintf(forecast->current_temperature, sizeof(forecast->current_temperature), "%s", "18 C");
    snprintf(forecast->range_text, sizeof(forecast->range_text), "%s", "High 21 C  |  Low 13 C");
    snprintf(forecast->summary, sizeof(forecast->summary), "%s", "Stays mild later today.");
    snprintf(forecast->details.rain_chance, sizeof(forecast->details.rain_chance), "%s",
             "Rain chance: 20%%");
    snprintf(forecast->details.wind, sizeof(forecast->details.wind), "%s", "Wind: 4 m/s NW");
    snprintf(forecast->details.humidity, sizeof(forecast->details.humidity), "%s",
             "Humidity: 61%%");
    snprintf(forecast->details.uv_index, sizeof(forecast->details.uv_index), "%s",
             "UV index: 3");

    snprintf(forecast->days[0].label, sizeof(forecast->days[0].label), "%s", "Mon");
    snprintf(forecast->days[0].condition, sizeof(forecast->days[0].condition), "%s", "Cloudy");
    snprintf(forecast->days[0].range_text, sizeof(forecast->days[0].range_text), "%s",
             "20 / 12 C");
    snprintf(forecast->days[1].label, sizeof(forecast->days[1].label), "%s", "Tue");
    snprintf(forecast->days[1].condition, sizeof(forecast->days[1].condition), "%s",
             "Light rain");
    snprintf(forecast->days[1].range_text, sizeof(forecast->days[1].range_text), "%s",
             "17 / 10 C");
    snprintf(forecast->days[2].label, sizeof(forecast->days[2].label), "%s", "Wed");
    snprintf(forecast->days[2].condition, sizeof(forecast->days[2].condition), "%s", "Sunny");
    snprintf(forecast->days[2].range_text, sizeof(forecast->days[2].range_text), "%s",
             "22 / 11 C");
    snprintf(forecast->days[3].label, sizeof(forecast->days[3].label), "%s", "Thu");
    snprintf(forecast->days[3].condition, sizeof(forecast->days[3].condition), "%s", "Windy");
    snprintf(forecast->days[3].range_text, sizeof(forecast->days[3].range_text), "%s",
             "19 / 9 C");
    snprintf(forecast->days[4].label, sizeof(forecast->days[4].label), "%s", "Fri");
    snprintf(forecast->days[4].condition, sizeof(forecast->days[4].condition), "%s",
             "Partly sunny");
    snprintf(forecast->days[4].range_text, sizeof(forecast->days[4].range_text), "%s",
             "21 / 13 C");
}

static bool gui_state_sensor_equals(const gui_sensor_state_t *left,
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

static bool gui_state_energy_plan_equals(const gui_energy_plan_t *left,
                                         const gui_energy_plan_t *right)
{
    if ((left == NULL) || (right == NULL)) {
        return false;
    }

    return memcmp(left, right, sizeof(*left)) == 0;
}

static bool gui_state_forecast_equals(const gui_forecast_state_t *left,
                                      const gui_forecast_state_t *right)
{
    if ((left == NULL) || (right == NULL)) {
        return false;
    }

    return memcmp(left, right, sizeof(*left)) == 0;
}

static bool gui_state_wifi_network_equals(const gui_wifi_network_t *left,
                                          const gui_wifi_network_t *right)
{
    if ((left == NULL) || (right == NULL)) {
        return false;
    }

    return (strcmp(left->ssid, right->ssid) == 0) &&
           (left->signal_strength_pct == right->signal_strength_pct) &&
           (left->secured == right->secured);
}

static bool gui_state_wifi_settings_equals(const gui_wifi_settings_t *left,
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
        (left->connect_requested != right->connect_requested) ||
        (left->can_disconnect != right->can_disconnect) ||
        (strcmp(left->selected_ssid, right->selected_ssid) != 0) ||
        (strcmp(left->password, right->password) != 0) ||
        (strcmp(left->status_text, right->status_text) != 0)) {
        return false;
    }

    for (index = 0; index < GUI_WIFI_NETWORK_COUNT; index++) {
        if (!gui_state_wifi_network_equals(&left->networks[index], &right->networks[index])) {
            return false;
        }
    }

    for (index = 0; index < GUI_WIFI_KNOWN_NETWORK_COUNT; index++) {
        if (!gui_state_wifi_network_equals(&left->known_networks[index],
                                           &right->known_networks[index])) {
            return false;
        }
    }

    return true;
}

static bool gui_state_location_settings_equals(const gui_location_settings_t *left,
                                               const gui_location_settings_t *right)
{
    if ((left == NULL) || (right == NULL)) {
        return false;
    }

    return (strcmp(left->latitude, right->latitude) == 0) &&
           (strcmp(left->longitude, right->longitude) == 0);
}

void gui_state_init(gui_state_t *state)
{
    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->active_panel = GUI_PANEL_BME280;
    state->appearance.theme = gui_theme_default();
    state->appearance.show_background_image = true;
    state->appearance.night_variant_enabled = false;
    state->wifi.selected_network_index = -1;
    state->wifi.selected_known_network_index = -1;
    state->wifi.connect_requested = false;
    state->wifi.can_disconnect = false;
    state->wifi.state = GUI_WIFI_STATE_IDLE;
    state->wifi_state = GUI_WIFI_STATE_IDLE;
    state->bluetooth_state = GUI_BLUETOOTH_STATE_IDLE;
    state->sd_card_state = GUI_SD_CARD_STATE_IDLE;
    gui_state_init_forecast(&state->forecast);
    gui_state_reset_wifi_scan(state);
    gui_state_copy_status(&state->wifi, "Press Scan to search for Wi-Fi networks.");
}

bool gui_state_set_active_panel(gui_state_t *state, gui_panel_id_t panel)
{
    if ((state == NULL) || (state->active_panel == panel)) {
        return false;
    }

    state->active_panel = panel;
    return true;
}

bool gui_state_set_sensor(gui_state_t *state, const gui_sensor_state_t *sensor)
{
    if ((state == NULL) || (sensor == NULL) ||
        gui_state_sensor_equals(&state->sensor, sensor)) {
        return false;
    }

    state->sensor = *sensor;
    return true;
}

bool gui_state_set_energy_plan(gui_state_t *state, const gui_energy_plan_t *energy_plan)
{
    if ((state == NULL) || (energy_plan == NULL) ||
        gui_state_energy_plan_equals(&state->energy_plan, energy_plan)) {
        return false;
    }

    state->energy_plan = *energy_plan;
    return true;
}

bool gui_state_set_forecast(gui_state_t *state, const gui_forecast_state_t *forecast)
{
    if ((state == NULL) || (forecast == NULL) ||
        gui_state_forecast_equals(&state->forecast, forecast)) {
        return false;
    }

    state->forecast = *forecast;
    return true;
}

bool gui_state_set_wifi_settings(gui_state_t *state, const gui_wifi_settings_t *wifi)
{
    if ((state == NULL) || (wifi == NULL) ||
        gui_state_wifi_settings_equals(&state->wifi, wifi)) {
        return false;
    }

    state->wifi = *wifi;
    return true;
}

bool gui_state_set_wifi_state(gui_state_t *state, gui_wifi_state_t wifi_state)
{
    if ((state == NULL) || (state->wifi_state == wifi_state)) {
        return false;
    }

    state->wifi_state = wifi_state;
    return true;
}

bool gui_state_set_bluetooth_state(gui_state_t *state,
                                   gui_bluetooth_state_t bluetooth_state)
{
    if ((state == NULL) || (state->bluetooth_state == bluetooth_state)) {
        return false;
    }

    state->bluetooth_state = bluetooth_state;
    return true;
}

bool gui_state_set_sd_card_state(gui_state_t *state, gui_sd_card_state_t sd_card_state)
{
    if ((state == NULL) || (state->sd_card_state == sd_card_state)) {
        return false;
    }

    state->sd_card_state = sd_card_state;
    return true;
}

bool gui_state_set_theme(gui_state_t *state, gui_view_theme_t theme)
{
    if ((state == NULL) || (state->appearance.theme == theme)) {
        return false;
    }

    state->appearance.theme = theme;
    return true;
}

bool gui_state_set_background_image_enabled(gui_state_t *state, bool enabled)
{
    if ((state == NULL) || (state->appearance.show_background_image == enabled)) {
        return false;
    }

    state->appearance.show_background_image = enabled;
    return true;
}

bool gui_state_set_night_variant_enabled(gui_state_t *state, bool enabled)
{
    if ((state == NULL) || (state->appearance.night_variant_enabled == enabled)) {
        return false;
    }

    state->appearance.night_variant_enabled = enabled;
    return true;
}

bool gui_state_set_location_settings(gui_state_t *state, const gui_location_settings_t *location)
{
    if ((state == NULL) || (location == NULL) ||
        gui_state_location_settings_equals(&state->location, location)) {
        return false;
    }

    state->location = *location;
    return true;
}

void gui_state_scan_wifi(gui_state_t *state)
{
    if (state == NULL) {
        return;
    }

    gui_state_reset_wifi_scan(state);
    state->wifi.selected_network_index = -1;
    state->wifi.selected_known_network_index = -1;
    state->wifi.selected_ssid[0] = '\0';
    state->wifi.password[0] = '\0';
    state->wifi.connect_requested = false;
    state->wifi.can_disconnect = false;
    state->wifi.state = GUI_WIFI_STATE_IDLE;
    gui_state_copy_status(&state->wifi, "Scan requested. No Wi-Fi results are available yet.");
}

bool gui_state_select_wifi_network(gui_state_t *state, uint8_t network_index)
{
    if ((state == NULL) || (network_index >= state->wifi.network_count)) {
        return false;
    }

    state->wifi.selected_network_index = (int8_t)network_index;
    state->wifi.selected_known_network_index = -1;
    snprintf(state->wifi.selected_ssid, sizeof(state->wifi.selected_ssid), "%s",
             state->wifi.networks[network_index].ssid);
    state->wifi.connect_requested = false;
    state->wifi.can_disconnect = false;
    state->wifi.state = GUI_WIFI_STATE_SCANNED;
    snprintf(state->wifi.status_text, sizeof(state->wifi.status_text),
             "Selected %s. Enter a password and press Connect.",
             state->wifi.networks[network_index].ssid);
    return true;
}

bool gui_state_connect_known_wifi(gui_state_t *state, uint8_t network_index)
{
    if ((state == NULL) || (network_index >= state->wifi.known_network_count)) {
        return false;
    }

    state->wifi.selected_network_index = -1;
    state->wifi.selected_known_network_index = (int8_t)network_index;
    snprintf(state->wifi.selected_ssid, sizeof(state->wifi.selected_ssid), "%s",
             state->wifi.known_networks[network_index].ssid);
    state->wifi.password[0] = '\0';
    state->wifi.connect_requested = true;
    state->wifi.can_disconnect = false;
    state->wifi.state = GUI_WIFI_STATE_CONNECTING;
    snprintf(state->wifi.status_text, sizeof(state->wifi.status_text),
             "Connecting to %s...",
             state->wifi.known_networks[network_index].ssid);
    return true;
}

bool gui_state_set_wifi_password(gui_state_t *state, const char *password)
{
    if ((state == NULL) || (password == NULL)) {
        return false;
    }

    if (strcmp(state->wifi.password, password) == 0) {
        return false;
    }

    snprintf(state->wifi.password, sizeof(state->wifi.password), "%s", password);
    return true;
}

bool gui_state_connect_wifi(gui_state_t *state)
{
    gui_wifi_network_t *selected_network;

    if (state == NULL) {
        return false;
    }

    if ((state->wifi.selected_network_index < 0) ||
        (state->wifi.selected_network_index >= (int8_t)state->wifi.network_count)) {
        state->wifi.state = GUI_WIFI_STATE_FAILED;
        gui_state_copy_status(&state->wifi,
                              "Select a network before attempting to connect.");
        return true;
    }

    selected_network = &state->wifi.networks[(uint8_t)state->wifi.selected_network_index];
    state->wifi.selected_known_network_index = -1;
    snprintf(state->wifi.selected_ssid, sizeof(state->wifi.selected_ssid), "%s",
             selected_network->ssid);

    if (selected_network->secured && (strlen(state->wifi.password) < 8U)) {
        state->wifi.state = GUI_WIFI_STATE_FAILED;
        gui_state_copy_status(
            &state->wifi,
            "Password must be at least 8 characters for secured networks.");
        return true;
    }

    state->wifi.connect_requested = true;
    state->wifi.can_disconnect = false;
    state->wifi.state = GUI_WIFI_STATE_CONNECTING;
    snprintf(state->wifi.status_text, sizeof(state->wifi.status_text),
             "Connecting to %s.", selected_network->ssid);
    return true;
}

bool gui_state_disconnect_wifi(gui_state_t *state)
{
    if (state == NULL) {
        return false;
    }

    state->wifi.selected_network_index = -1;
    state->wifi.selected_known_network_index = -1;
    state->wifi.connect_requested = false;
    state->wifi.can_disconnect = false;
    state->wifi.state = GUI_WIFI_STATE_IDLE;
    gui_state_copy_status(&state->wifi, "Disconnected.");
    return true;
}

int8_t gui_state_find_known_wifi_network(const gui_state_t *state, const char *ssid)
{
    uint8_t index;

    if ((state == NULL) || (ssid == NULL)) {
        return -1;
    }

    for (index = 0; index < state->wifi.known_network_count; index++) {
        if (strcmp(state->wifi.known_networks[index].ssid, ssid) == 0) {
            return (int8_t)index;
        }
    }

    return -1;
}

void gui_state_build_screen_model(const gui_state_t *state, gui_view_model_t *model)
{
    if ((state == NULL) || (model == NULL)) {
        return;
    }

    model->active_panel = state->active_panel;
    model->sensor = state->sensor;
    model->energy_plan = state->energy_plan;
    model->forecast = state->forecast;
    model->wifi = state->wifi;
    model->wifi_state = state->wifi_state;
    model->bluetooth_state = state->bluetooth_state;
    model->sd_card_state = state->sd_card_state;
    model->appearance = state->appearance;
    model->location = state->location;
}
