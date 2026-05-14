/**
 * @file gui_state.h
 * @brief Internal GUI state container and state transition helpers.
 */

#ifndef GUI_STATE_H
#define GUI_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "gui_defs.h"

/**
 * @brief Internal mutable state mirrored into a gui_view_model_t before rendering.
 */
typedef struct {
    gui_panel_id_t active_panel;                /*!< Panel currently selected by the user. */
    gui_sensor_state_t sensor;                  /*!< Latest sensor values known to the GUI. */
    gui_forecast_state_t forecast;              /*!< Latest forecast values known to the GUI. */
    gui_wifi_settings_t wifi;                   /*!< Wi-Fi dialog state and connection workflow data. */
    gui_wifi_state_t wifi_state;                /*!< Sidebar Wi-Fi indicator state. */
    gui_bluetooth_state_t bluetooth_state;      /*!< Sidebar Bluetooth indicator state. */
    gui_sd_card_state_t sd_card_state;          /*!< Sidebar SD card indicator state. */
    gui_appearance_settings_t appearance;       /*!< Appearance settings used to theme the screen. */
    gui_location_settings_t location;           /*!< Editable location settings used by the System page. */
} gui_state_t;

/**
 * @brief Initialize a GUI state object with default values.
 *
 * @param state State object to initialize.
 */
void gui_state_init(gui_state_t *state);

/**
 * @brief Set the active panel.
 *
 * @param state State object to update.
 * @param panel Panel to make active.
 * @return True when the stored value changed.
 */
bool gui_state_set_active_panel(gui_state_t *state, gui_panel_id_t panel);

/**
 * @brief Update the stored sensor state.
 *
 * @param state State object to update.
 * @param sensor Sensor state snapshot to copy.
 * @return True when the stored value changed.
 */
bool gui_state_set_sensor(gui_state_t *state, const gui_sensor_state_t *sensor);

/**
 * @brief Update the stored forecast state.
 *
 * @param state State object to update.
 * @param forecast Forecast state snapshot to copy.
 * @return True when the stored value changed.
 */
bool gui_state_set_forecast(gui_state_t *state, const gui_forecast_state_t *forecast);

/**
 * @brief Update the stored Wi-Fi settings model.
 *
 * @param state State object to update.
 * @param wifi Wi-Fi settings snapshot to copy.
 * @return True when the stored value changed.
 */
bool gui_state_set_wifi_settings(gui_state_t *state, const gui_wifi_settings_t *wifi);

/**
 * @brief Set the sidebar Wi-Fi indicator state.
 *
 * @param state State object to update.
 * @param wifi_state New Wi-Fi indicator state.
 * @return True when the stored value changed.
 */
bool gui_state_set_wifi_state(gui_state_t *state, gui_wifi_state_t wifi_state);

/**
 * @brief Set the sidebar Bluetooth indicator state.
 *
 * @param state State object to update.
 * @param bluetooth_state New Bluetooth indicator state.
 * @return True when the stored value changed.
 */
bool gui_state_set_bluetooth_state(gui_state_t *state, gui_bluetooth_state_t bluetooth_state);

/**
 * @brief Set the sidebar SD card indicator state.
 *
 * @param state State object to update.
 * @param sd_card_state New SD card indicator state.
 * @return True when the stored value changed.
 */
bool gui_state_set_sd_card_state(gui_state_t *state, gui_sd_card_state_t sd_card_state);

/**
 * @brief Change the active GUI theme.
 *
 * @param state State object to update.
 * @param theme Theme to apply.
 * @return True when the stored value changed.
 */
bool gui_state_set_theme(gui_state_t *state, gui_view_theme_t theme);

/**
 * @brief Enable or disable the theme background image.
 *
 * @param state State object to update.
 * @param enabled True to show the background image.
 * @return True when the stored value changed.
 */
bool gui_state_set_background_image_enabled(gui_state_t *state, bool enabled);

/**
 * @brief Enable or disable the theme night variant.
 *
 * @param state State object to update.
 * @param enabled True to use the night variant when the theme supports it.
 * @return True when the stored value changed.
 */
bool gui_state_set_night_variant_enabled(gui_state_t *state, bool enabled);

/**
 * @brief Update the stored location settings.
 *
 * @param state State object to update.
 * @param location Location settings snapshot to copy.
 * @return True when the stored value changed.
 */
bool gui_state_set_location_settings(gui_state_t *state, const gui_location_settings_t *location);

/**
 * @brief Mark the Wi-Fi state as actively scanning.
 *
 * @param state State object to update.
 */
void gui_state_scan_wifi(gui_state_t *state);

/**
 * @brief Select a scanned Wi-Fi network by index.
 *
 * @param state State object to update.
 * @param network_index Index into state->wifi.networks.
 * @return True when the selection changed and the request was accepted.
 */
bool gui_state_select_wifi_network(gui_state_t *state, uint8_t network_index);

/**
 * @brief Select a known Wi-Fi network and prepare a quick-connect flow.
 *
 * @param state State object to update.
 * @param network_index Index into state->wifi.known_networks.
 * @return True when the selection changed and the request was accepted.
 */
bool gui_state_connect_known_wifi(gui_state_t *state, uint8_t network_index);

/**
 * @brief Update the password buffer used by the Wi-Fi password dialog.
 *
 * @param state State object to update.
 * @param password Null-terminated password string to copy.
 * @return True when the stored value changed.
 */
bool gui_state_set_wifi_password(gui_state_t *state, const char *password);

/**
 * @brief Mark the current Wi-Fi target as ready for a connection attempt.
 *
 * @param state State object to update.
 * @return True when the connection request state changed.
 */
bool gui_state_connect_wifi(gui_state_t *state);

/**
 * @brief Request disconnection from the current Wi-Fi network.
 *
 * @param state State object to update.
 * @return True when the disconnect request state changed.
 */
bool gui_state_disconnect_wifi(gui_state_t *state);

/**
 * @brief Find a saved Wi-Fi network by SSID.
 *
 * @param state State object to inspect.
 * @param ssid Null-terminated SSID string to search for.
 * @return Matching known network index, or -1 when no match exists.
 */
int8_t gui_state_find_known_wifi_network(const gui_state_t *state, const char *ssid);

/**
 * @brief Build a renderable screen model from the internal state.
 *
 * @param state Source state snapshot.
 * @param model Output model populated for the view layer.
 */
void gui_state_build_screen_model(const gui_state_t *state, gui_view_model_t *model);

#endif
