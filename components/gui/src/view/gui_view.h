/**
 * @file gui_view.h
 * @brief Internal LVGL view tree and rendering helpers for the GUI screen.
 */

#ifndef GUI_VIEW_H
#define GUI_VIEW_H

#include "lvgl.h"

#include "../gui_defs.h"

typedef enum {
    GUI_SETTINGS_SUBPAGE_HOME = 0,
    GUI_SETTINGS_SUBPAGE_CONNECTIVITY,
    GUI_SETTINGS_SUBPAGE_DISPLAY,
    GUI_SETTINGS_SUBPAGE_SYSTEM,
} gui_settings_subpage_t;

/**
 * @brief Concrete LVGL object tree for the GUI screen.
 *
 * The struct stores both persistent widget pointers and the last-applied model
 * fragments used to avoid unnecessary redraw work.
 */
typedef struct {
    lv_obj_t *screen;             /*!< Root LVGL screen object. */
    lv_obj_t *background_image;   /*!< Optional full-screen background image. */
    lv_obj_t *sidebar;            /*!< Sidebar container holding navigation and status items. */
    lv_obj_t *content;            /*!< Main content container hosting the active panel. */

    lv_obj_t *bme280_button;      /*!< Sidebar button for the BME280 panel. */
    lv_obj_t *energy_plan_button; /*!< Sidebar button for the energy plan panel. */
    lv_obj_t *forecast_button;    /*!< Sidebar button for the forecast panel. */
    lv_obj_t *settings_button;    /*!< Sidebar button for the settings panel. */

    lv_obj_t *header_title;           /*!< Primary title shown above the active panel. */
    lv_obj_t *header_subtitle;        /*!< Secondary subtitle shown above the active panel. */
    lv_obj_t *update_label;           /*!< Timestamp or freshness label for the current data. */
    lv_obj_t *sidebar_clock_label;    /*!< Sidebar clock label. */
    lv_obj_t *sidebar_date_label;     /*!< Sidebar date label. */
    lv_obj_t *sidebar_wifi_label;     /*!< Sidebar Wi-Fi status label. */
    lv_obj_t *sidebar_bluetooth_label; /*!< Sidebar Bluetooth status label. */
    lv_obj_t *sidebar_sd_card_label;  /*!< Sidebar SD card status label. */

    lv_obj_t *bme280_panel;       /*!< Root container for the BME280 panel. */
    lv_obj_t *energy_plan_panel;  /*!< Root container for the energy plan panel. */
    lv_obj_t *forecast_panel;     /*!< Root container for the forecast panel. */
    lv_obj_t *settings_panel;     /*!< Root container for the settings panel. */

    lv_obj_t *settings_home_panel;    /*!< Top-level settings content container. */
    lv_obj_t *settings_connectivity_panel; /*!< Settings subpage for connectivity controls. */
    lv_obj_t *settings_display_panel; /*!< Settings subpage for display controls. */
    lv_obj_t *settings_system_panel;  /*!< Settings subpage reserved for future system controls. */
    lv_obj_t *settings_connectivity_button; /*!< Category button that opens the connectivity subpage. */
    lv_obj_t *settings_display_button; /*!< Category button that opens the display subpage. */
    lv_obj_t *settings_system_button;  /*!< Category button that opens the system subpage. */
    lv_obj_t *settings_connectivity_back_button; /*!< Back button on the connectivity subpage. */
    lv_obj_t *settings_display_back_button; /*!< Back button on the display subpage. */
    lv_obj_t *settings_system_back_button; /*!< Back button on the system subpage. */
    lv_obj_t *other_settings_card;    /*!< Settings card containing non-connectivity options. */
    lv_obj_t *wifi_card;              /*!< Settings subsection for Wi-Fi controls. */
    lv_obj_t *wifi_header_row;        /*!< Row containing the Wi-Fi section title and status. */
    lv_obj_t *wifi_title_label;       /*!< Wi-Fi section title label. */
    lv_obj_t *wifi_subtitle_label;    /*!< Wi-Fi section subtitle label. */
    lv_obj_t *wifi_status_label;      /*!< Wi-Fi section status label. */
    lv_obj_t *bluetooth_card;         /*!< Settings subsection for Bluetooth status. */
    lv_obj_t *bluetooth_status_label; /*!< Bluetooth status label. */
    lv_obj_t *brightness_card;        /*!< Settings subsection containing brightness controls. */
    lv_obj_t *scan_button;            /*!< Button that starts a Wi-Fi scan. */
    lv_obj_t *disconnect_button;      /*!< Button that disconnects the active Wi-Fi network. */
    lv_obj_t *theme_card;             /*!< Settings subsection containing theme controls. */
    lv_obj_t *theme_dropdown;         /*!< Theme selection dropdown. */
    lv_obj_t *theme_background_label; /*!< Label for the background image switch. */
    lv_obj_t *theme_background_switch; /*!< Switch controlling background image visibility. */
    lv_obj_t *theme_night_label;      /*!< Label for the night variant switch. */
    lv_obj_t *theme_night_switch;     /*!< Switch controlling the night variant setting. */
    lv_obj_t *brightness_slider;      /*!< Slider used to change display brightness. */
    lv_obj_t *brightness_value_label; /*!< Label showing the current brightness percentage. */
    lv_obj_t *location_card;          /*!< Settings subsection containing location controls. */
    lv_obj_t *location_latitude_label; /*!< Label for the latitude textarea. */
    lv_obj_t *location_latitude_textarea; /*!< Text area used to edit latitude. */
    lv_obj_t *location_longitude_label; /*!< Label for the longitude textarea. */
    lv_obj_t *location_longitude_textarea; /*!< Text area used to edit longitude. */
    lv_obj_t *location_keyboard;      /*!< On-screen keyboard dedicated to location input. */

    lv_obj_t *dialog_scrim;                    /*!< Shared modal scrim behind dialog content. */
    lv_obj_t *network_dialog;                  /*!< Wi-Fi network selection dialog container. */
    lv_obj_t *network_dialog_title;            /*!< Network dialog title label. */
    lv_obj_t *network_dialog_subtitle;         /*!< Network dialog subtitle label. */
    lv_obj_t *network_empty_label;             /*!< Empty-state label shown when no networks are available. */
    lv_obj_t *network_dialog_buttons[GUI_WIFI_NETWORK_COUNT]; /*!< Buttons for scanned Wi-Fi network entries. */
    lv_obj_t *network_dialog_button_labels[GUI_WIFI_NETWORK_COUNT]; /*!< Labels paired with network_dialog_buttons. */
    lv_obj_t *network_dialog_cancel_button;    /*!< Cancel button for the network dialog. */
    lv_obj_t *password_dialog;                 /*!< Wi-Fi password entry dialog container. */
    lv_obj_t *password_dialog_title;           /*!< Password dialog title label. */
    lv_obj_t *password_dialog_network_label;   /*!< Label showing the selected SSID. */
    lv_obj_t *wifi_password_textarea;          /*!< Text area used to enter the Wi-Fi password. */
    lv_obj_t *wifi_keyboard;                   /*!< On-screen keyboard bound to the password text area. */
    lv_obj_t *password_dialog_cancel_button;   /*!< Cancel button for the password dialog. */
    lv_obj_t *password_dialog_connect_button;  /*!< Connect button for the password dialog. */
    lv_obj_t *password_dialog_disconnect_button; /*!< Disconnect button shown when a network can be disconnected. */

    lv_obj_t *temperature_value;   /*!< BME280 temperature value label. */
    lv_obj_t *humidity_value;      /*!< BME280 humidity value label. */
    lv_obj_t *pressure_value;      /*!< BME280 pressure value label. */
    lv_obj_t *status_line;         /*!< BME280 status line under the metric cards. */

    lv_obj_t *energy_plan_chart;   /*!< Chart widget used by the energy panel. */
    lv_obj_t *energy_legend_dots[4]; /*!< Legend markers for the four energy series. */
    lv_obj_t *energy_legend_labels[4]; /*!< Legend labels for the four energy series. */
    lv_obj_t *energy_time_labels[GUI_ENERGY_PLAN_TIME_LABEL_COUNT]; /*!< Time labels under the energy chart. */
    lv_chart_series_t *buy_series; /*!< Grid purchase chart series. */
    lv_chart_series_t *solar_series; /*!< Solar usage chart series. */
    lv_chart_series_t *charge_series; /*!< Battery charging chart series. */
    lv_chart_series_t *sell_series; /*!< Excess energy sell chart series. */

    gui_panel_id_t last_active_panel;        /*!< Last panel applied to the navigation state. */
    bool has_last_active_panel;              /*!< True when last_active_panel contains a valid cached value. */
    gui_energy_plan_t last_energy_plan;      /*!< Last energy plan applied to the chart. */
    bool has_last_energy_plan;               /*!< True when last_energy_plan contains a valid cached value. */
    gui_wifi_settings_t last_wifi_settings;  /*!< Last Wi-Fi settings model applied to the dialogs and cards. */
    bool has_last_wifi_settings;             /*!< True when last_wifi_settings contains a valid cached value. */
    gui_location_settings_t last_location_settings; /*!< Last location settings model applied to the System page. */
    bool has_last_location_settings;         /*!< True when last_location_settings contains a valid cached value. */
    gui_view_theme_t current_theme;          /*!< Theme currently applied to the view. */
    bool current_show_background_image;      /*!< Cached background image visibility setting. */
    bool current_night_variant_enabled;      /*!< Cached night variant setting. */
    bool has_current_appearance;             /*!< True when the cached appearance settings are initialized. */
    gui_settings_subpage_t active_settings_subpage; /*!< Settings subpage currently visible inside the settings panel. */
} gui_view_t;

/**
 * @brief Create the LVGL object tree for the GUI view.
 *
 * @param view View object to initialize.
 * @param model Initial model applied during creation.
 * @param nav_event_cb Callback for navigation interactions.
 * @param settings_event_cb Callback for settings interactions.
 * @param event_user_data User data forwarded to both callbacks.
 */
void gui_view_init(gui_view_t *view, const gui_view_model_t *model, lv_event_cb_t nav_event_cb,
                   lv_event_cb_t settings_event_cb, void *event_user_data);

/**
 * @brief Apply a new model snapshot to an existing GUI view.
 *
 * @param view Initialized view object.
 * @param model Model snapshot to render.
 */
void gui_view_apply(gui_view_t *view, const gui_view_model_t *model);

/**
 * @brief Apply the effective theme configuration to the current view tree.
 *
 * @param view Initialized view object.
 * @param theme Base theme to apply.
 * @param show_background_image True to show the theme background image.
 * @param night_variant_enabled True to use the theme night variant when available.
 */
void gui_view_apply_theme(gui_view_t *view, gui_view_theme_t theme, bool show_background_image, bool night_variant_enabled);

/**
 * @brief Resolve the theme color for the Wi-Fi status indicator.
 *
 * @param theme Theme whose palette should be used.
 * @param state Wi-Fi state to map to a color.
 * @return Theme-specific LVGL color for the given state.
 */
lv_color_t gui_view_wifi_status_color(gui_view_theme_t theme, gui_wifi_state_t state);

/**
 * @brief Resolve the theme color for the Bluetooth status indicator.
 *
 * @param theme Theme whose palette should be used.
 * @param state Bluetooth state to map to a color.
 * @return Theme-specific LVGL color for the given state.
 */
lv_color_t gui_view_bluetooth_status_color(gui_view_theme_t theme, gui_bluetooth_state_t state);

/**
 * @brief Resolve the theme color for the SD card status indicator.
 *
 * @param theme Theme whose palette should be used.
 * @param state SD card state to map to a color.
 * @return Theme-specific LVGL color for the given state.
 */
lv_color_t gui_view_sd_card_status_color(gui_view_theme_t theme, gui_sd_card_state_t state);

/**
 * @brief Refresh the sidebar clock and date labels from the current time.
 *
 * @param view Initialized view object.
 */
void gui_view_update_sidebar_clock_labels(gui_view_t *view);

/**
 * @brief Hide any active Wi-Fi modal dialogs in the view.
 *
 * @param view Initialized view object.
 */
void gui_view_hide_wifi_dialogs(gui_view_t *view);

/**
 * @brief Show the Wi-Fi network selection dialog.
 *
 * @param view Initialized view object.
 */
void gui_view_show_network_dialog(gui_view_t *view);

/**
 * @brief Show the Wi-Fi password entry dialog.
 *
 * @param view Initialized view object.
 */
void gui_view_show_password_dialog(gui_view_t *view);

/**
 * @brief Switch the settings panel to a specific internal subpage.
 *
 * @param view Initialized view object.
 * @param subpage Settings subpage to show.
 */
void gui_view_show_settings_subpage(gui_view_t *view, gui_settings_subpage_t subpage);

/**
 * @brief Reset the settings panel to the category chooser home page.
 *
 * @param view Initialized view object.
 */
void gui_view_reset_settings_navigation(gui_view_t *view);

#endif
