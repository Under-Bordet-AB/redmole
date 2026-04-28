#ifndef GUI_SCREEN_H
#define GUI_SCREEN_H

#include "view/gui_view.h"

typedef gui_view_t gui_screen_t;
typedef gui_view_model_t gui_screen_model_t;

void gui_screen_init(gui_screen_t *screen, const gui_screen_model_t *model,
                     lv_event_cb_t nav_event_cb, lv_event_cb_t settings_event_cb,
                     void *event_user_data);
void gui_screen_apply(gui_screen_t *screen, const gui_screen_model_t *model);
void gui_screen_update_sidebar_clock(gui_screen_t *screen);
void gui_screen_show_network_dialog(gui_screen_t *screen);
void gui_screen_show_password_dialog(gui_screen_t *screen);
void gui_screen_hide_wifi_dialogs(gui_screen_t *screen);
void gui_screen_sync_brightness(gui_screen_t *screen, int32_t brightness_percent);

#endif
