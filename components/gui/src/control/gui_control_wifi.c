#include "gui_control_internal.h"

#include <stdio.h>
#include <string.h>

void gui_control_scan_wifi(gui_control_t *control)
{
    if (control == NULL) {
        return;
    }

    gui_control_reset_wifi_scan(control);
    control->wifi.selected_network_index = -1;
    control->wifi.selected_known_network_index = -1;
    control->wifi.selected_ssid[0] = '\0';
    control->wifi.password[0] = '\0';
    control->wifi.state = GUI_WIFI_STATE_IDLE;
    gui_control_copy_status(&control->wifi, "Scan requested. No Wi-Fi results are available yet.");
}

void gui_control_select_wifi_network(gui_control_t *control, uint8_t network_index)
{
    if ((control == NULL) || (network_index >= control->wifi.network_count)) {
        return;
    }

    control->wifi.selected_network_index = (int8_t)network_index;
    control->wifi.selected_known_network_index = -1;
    snprintf(control->wifi.selected_ssid, sizeof(control->wifi.selected_ssid), "%s",
             control->wifi.networks[network_index].ssid);
    control->wifi.state = GUI_WIFI_STATE_SCANNED;
    snprintf(control->wifi.status_text, sizeof(control->wifi.status_text),
             "Selected %s. Enter a password and press Connect.",
             control->wifi.networks[network_index].ssid);
}

void gui_control_connect_known_wifi(gui_control_t *control, uint8_t network_index)
{
    if ((control == NULL) || (network_index >= control->wifi.known_network_count)) {
        return;
    }

    control->wifi.selected_network_index = -1;
    control->wifi.selected_known_network_index = (int8_t)network_index;
    snprintf(control->wifi.selected_ssid, sizeof(control->wifi.selected_ssid), "%s",
             control->wifi.known_networks[network_index].ssid);
    snprintf(control->wifi.password, sizeof(control->wifi.password), "%s", "saved-password");
    control->wifi.state = GUI_WIFI_STATE_SCANNED;
    snprintf(control->wifi.status_text, sizeof(control->wifi.status_text),
             "Saved network %s selected. Connection is pending.",
             control->wifi.known_networks[network_index].ssid);
}

void gui_control_set_wifi_password(gui_control_t *control, const char *password)
{
    if ((control == NULL) || (password == NULL)) {
        return;
    }

    snprintf(control->wifi.password, sizeof(control->wifi.password), "%s", password);
}

void gui_control_connect_wifi(gui_control_t *control)
{
    gui_wifi_network_t *selected_network;

    if (control == NULL) {
        return;
    }

    if ((control->wifi.selected_network_index < 0) ||
        (control->wifi.selected_network_index >= (int8_t)control->wifi.network_count)) {
        control->wifi.state = GUI_WIFI_STATE_FAILED;
        gui_control_copy_status(&control->wifi, "Select a network before attempting to connect.");
        return;
    }

    selected_network = &control->wifi.networks[(uint8_t)control->wifi.selected_network_index];
    control->wifi.selected_known_network_index = -1;
    snprintf(control->wifi.selected_ssid, sizeof(control->wifi.selected_ssid), "%s",
             selected_network->ssid);

    if (selected_network->secured && strlen(control->wifi.password) < 8U) {
        control->wifi.state = GUI_WIFI_STATE_FAILED;
        gui_control_copy_status(&control->wifi,
                                "Password must be at least 8 characters for secured networks.");
        return;
    }

    control->wifi.state = GUI_WIFI_STATE_IDLE;
    snprintf(control->wifi.status_text, sizeof(control->wifi.status_text),
             "Connection requested for %s.", selected_network->ssid);
}