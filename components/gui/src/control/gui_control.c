#include "gui_control.h"

#include <string.h>

#include "gui_control_internal.h"

void gui_control_init(gui_control_t *control)
{
    if (control == NULL) {
        return;
    }

    memset(control, 0, sizeof(*control));
    control->active_panel = GUI_PANEL_BME280;
    control->appearance.theme = GUI_VIEW_THEME_HELLO_KITTY;
    control->appearance.show_background_image = true;
    control->appearance.night_variant_enabled = false;
    control->wifi.selected_network_index = -1;
    control->wifi.selected_known_network_index = -1;
    control->wifi.state = GUI_WIFI_STATE_IDLE;
    gui_control_reset_wifi_scan(control);
    gui_control_copy_status(&control->wifi, "Press Scan to search for Wi-Fi networks.");
}

void gui_control_select_panel(gui_control_t *control, gui_panel_id_t panel)
{
    if (control == NULL) {
        return;
    }

    control->active_panel = panel;
}