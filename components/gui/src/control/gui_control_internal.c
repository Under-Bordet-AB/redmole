#include "gui_control_internal.h"

#include <stdio.h>
#include <string.h>

void gui_control_copy_status(gui_wifi_settings_t *wifi, const char *text)
{
    if ((wifi == NULL) || (text == NULL)) {
        return;
    }

    snprintf(wifi->status_text, sizeof(wifi->status_text), "%s", text);
}

void gui_control_reset_wifi_scan(gui_control_t *control)
{
    if (control == NULL) {
        return;
    }

    memset(control->wifi.networks, 0, sizeof(control->wifi.networks));
    memset(control->wifi.known_networks, 0, sizeof(control->wifi.known_networks));
    control->wifi.network_count = 0;
    control->wifi.known_network_count = 0;
}

void gui_control_clear_energy_plan(gui_energy_plan_t *energy_plan)
{
    if (energy_plan == NULL) {
        return;
    }

    memset(energy_plan, 0, sizeof(*energy_plan));
}