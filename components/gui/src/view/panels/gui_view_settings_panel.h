#ifndef GUI_VIEW_SETTINGS_PANEL_H
#define GUI_VIEW_SETTINGS_PANEL_H

#include "../gui_view.h"

void gui_view_init_settings_panel(gui_view_t *view, lv_event_cb_t settings_event_cb,
                                  void *event_user_data);
void gui_view_apply_settings_panel(gui_view_t *view, const gui_view_model_t *model);
void gui_view_hide_wifi_dialogs_impl(gui_view_t *view);
void gui_view_show_network_dialog_impl(gui_view_t *view);
void gui_view_show_password_dialog_impl(gui_view_t *view);

#endif