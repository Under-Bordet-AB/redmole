#include "gui_screen.h"

#include <stdio.h>

void gui_screen_init(gui_screen_t *screen, const gui_screen_model_t *model,
                     lv_event_cb_t nav_event_cb, lv_event_cb_t settings_event_cb,
                     void *event_user_data)
{
    gui_view_init(screen, model, nav_event_cb, settings_event_cb, event_user_data);
}

void gui_screen_apply(gui_screen_t *screen, const gui_screen_model_t *model)
{
    gui_view_apply(screen, model);
}

void gui_screen_update_sidebar_clock(gui_screen_t *screen)
{
    gui_view_update_sidebar_clock_labels(screen);
}

void gui_screen_show_network_dialog(gui_screen_t *screen)
{
    gui_view_show_network_dialog(screen);
}

void gui_screen_show_password_dialog(gui_screen_t *screen)
{
    gui_view_show_password_dialog(screen);
}

void gui_screen_hide_wifi_dialogs(gui_screen_t *screen)
{
    gui_view_hide_wifi_dialogs(screen);
}

void gui_screen_sync_brightness(gui_screen_t *screen, int32_t brightness_percent)
{
    char brightness_text[8];

    if ((screen == NULL) || (screen->brightness_slider == NULL) ||
        (screen->brightness_value_label == NULL)) {
        return;
    }

    lv_slider_set_value(screen->brightness_slider, brightness_percent, LV_ANIM_OFF);
    snprintf(brightness_text, sizeof(brightness_text), "%ld%%", (long)brightness_percent);
    lv_label_set_text(screen->brightness_value_label, brightness_text);
}
