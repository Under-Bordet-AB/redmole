/**
 * @file gui_view_settings_panel.h
 * @brief Internal view helpers for the settings panel and Wi-Fi dialogs.
 */

#ifndef GUI_VIEW_SETTINGS_PANEL_H
#define GUI_VIEW_SETTINGS_PANEL_H

#include "../gui_view.h"

/**
 * @brief Create the settings panel widgets and bind settings callbacks.
 *
 * @param view View object that stores created widget pointers.
 * @param settings_event_cb Callback for settings interactions.
 * @param event_user_data User data forwarded to the callback.
 */
void gui_view_init_settings_panel(gui_view_t *view, lv_event_cb_t settings_event_cb,void *event_user_data);

/**
 * @brief Apply the current settings-related model state to the settings panel.
 *
 * @param view Initialized view object.
 * @param model Model snapshot to render.
 */
void gui_view_apply_settings_panel(gui_view_t *view, const gui_view_model_t *model);

/**
 * @brief Hide any active Wi-Fi dialogs owned by the settings panel implementation.
 *
 * @param view Initialized view object.
 */
void gui_view_hide_wifi_dialogs_impl(gui_view_t *view);

/**
 * @brief Show the Wi-Fi network selection dialog owned by the settings panel.
 *
 * @param view Initialized view object.
 */
void gui_view_show_network_dialog_impl(gui_view_t *view);

/**
 * @brief Show the Wi-Fi password dialog owned by the settings panel.
 *
 * @param view Initialized view object.
 */
void gui_view_show_password_dialog_impl(gui_view_t *view);

/**
 * @brief Switch the settings panel to a specific internal subpage.
 *
 * @param view Initialized view object.
 * @param subpage Settings subpage to show.
 */
void gui_view_show_settings_subpage_impl(gui_view_t *view, gui_settings_subpage_t subpage);

/**
 * @brief Reset settings navigation to the category chooser home page.
 *
 * @param view Initialized view object.
 */
void gui_view_reset_settings_navigation_impl(gui_view_t *view);

#endif
