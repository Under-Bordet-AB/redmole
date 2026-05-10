/**
 * @file gui_screen.h
 * @brief Internal screen-level wrapper around the GUI view layer.
 */

#ifndef GUI_SCREEN_H
#define GUI_SCREEN_H

#include "view/gui_view.h"

/** @brief Concrete screen object used by the GUI runtime. */
typedef gui_view_t gui_screen_t;
/** @brief Model type consumed when applying screen updates. */
typedef gui_view_model_t gui_screen_model_t;

/**
 * @brief Initialize the GUI screen and create its LVGL object tree.
 *
 * @param screen Screen object to initialize.
 * @param model Initial model applied while creating the screen.
 * @param nav_event_cb Navigation callback for panel switching actions.
 * @param settings_event_cb Settings callback for controls owned by the settings panel.
 * @param event_user_data User data forwarded to the event callbacks.
 */
void gui_screen_init(gui_screen_t *screen, const gui_screen_model_t *model, lv_event_cb_t nav_event_cb, lv_event_cb_t settings_event_cb, void *event_user_data);

/**
 * @brief Apply a new screen model to the existing LVGL object tree.
 *
 * @param screen Initialized screen object.
 * @param model Model snapshot to render.
 */
void gui_screen_apply(gui_screen_t *screen, const gui_screen_model_t *model);

/**
 * @brief Refresh the sidebar clock and date labels.
 *
 * @param screen Initialized screen object.
 */
void gui_screen_update_sidebar_clock(gui_screen_t *screen);

/**
 * @brief Show the Wi-Fi network selection dialog.
 *
 * @param screen Initialized screen object.
 */
void gui_screen_show_network_dialog(gui_screen_t *screen);

/**
 * @brief Show the Wi-Fi password entry dialog.
 *
 * @param screen Initialized screen object.
 */
void gui_screen_show_password_dialog(gui_screen_t *screen);

/**
 * @brief Hide any active Wi-Fi modal dialogs.
 *
 * @param screen Initialized screen object.
 */
void gui_screen_hide_wifi_dialogs(gui_screen_t *screen);

/**
 * @brief Synchronize the brightness controls with the effective platform brightness.
 *
 * @param screen Initialized screen object.
 * @param brightness_percent Brightness percentage to reflect in the settings UI.
 */
void gui_screen_sync_brightness(gui_screen_t *screen, int32_t brightness_percent);

/**
 * @brief Switch the settings panel to a specific internal subpage.
 *
 * @param screen Initialized screen object.
 * @param subpage Settings subpage to show.
 */
void gui_screen_show_settings_subpage(gui_screen_t *screen, gui_settings_subpage_t subpage);

/**
 * @brief Reset settings navigation to the category chooser home page.
 *
 * @param screen Initialized screen object.
 */
void gui_screen_reset_settings_navigation(gui_screen_t *screen);

#endif
