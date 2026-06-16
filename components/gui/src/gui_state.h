#ifndef GUI_STATE_H
#define GUI_STATE_H

/**
 * @file gui_state.h
 * @brief Internal GUI state container and state transition helpers.
 *
 * The state layer stores the model that later gets copied into a
 * gui_view_model_t. Mutators return whether a meaningful change occurred so
 * callers can avoid unnecessary renders.
 */

#include <stdbool.h>
#include <stdint.h>

#include "gui_defs.h"

/**
 * @brief Internal mutable state mirrored into a gui_view_model_t before rendering.
 */
typedef struct {
    gui_panel_id_t active_panel;                /*!< Panel currently selected by the user. */
    gui_energy_panel_mode_t energy_panel_mode;  /*!< Subview currently selected in the energy panel. */
    gui_sensor_state_t sensor;                  /*!< Latest sensor values known to the GUI. */
    gui_energy_plan_t energy_plan;              /*!< Latest energy plan values known to the GUI. */
    gui_spot_price_state_t spot_price;          /*!< Latest spot-price values known to the GUI. */
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
 * @param state State object to initialize; NULL is ignored.
 */
void gui_state_init(gui_state_t *state);

/**
 * @brief Set the active panel.
 *
 * @param state State object to update; NULL returns false.
 * @param panel Panel to make active.
 * @return True when the stored value changed.
 */
bool gui_state_set_active_panel(gui_state_t *state, gui_panel_id_t panel);

/**
 * @brief Update the stored sensor state.
 *
 * @param state State object to update; NULL returns false.
 * @param sensor Sensor state snapshot to copy, must not be NULL.
 * @return True when the stored value changed.
 */
bool gui_state_set_sensor(gui_state_t *state, const gui_sensor_state_t *sensor);

/**
 * @brief Update the stored energy plan state.
 *
 * @param state State object to update; NULL returns false.
 * @param energy_plan Energy plan snapshot to copy, must not be NULL.
 * @return True when the stored value changed.
 */
bool gui_state_set_energy_plan(gui_state_t *state, const gui_energy_plan_t *energy_plan);

/**
 * @brief Set the active subview inside the energy panel.
 *
 * @param state State object to update; NULL returns false.
 * @param mode Energy panel mode to make active.
 * @return True when the stored value changed.
 */
bool gui_state_set_energy_panel_mode(gui_state_t *state, gui_energy_panel_mode_t mode);

/**
 * @brief Update the stored spot-price state.
 *
 * @param state State object to update; NULL returns false.
 * @param spot_price Spot-price snapshot to copy, must not be NULL.
 * @return True when the stored value changed.
 */
bool gui_state_set_spot_price(gui_state_t *state,
                              const gui_spot_price_state_t *spot_price);

/**
 * @brief Update the selected spot-price area.
 *
 * @param state State object to update; NULL returns false.
 * @param area Swedish price area to select.
 * @return True when the stored value changed.
 */
bool gui_state_set_spot_price_area(gui_state_t *state, gui_spot_price_area_t area);

/**
 * @brief Update the stored forecast state.
 *
 * @param state State object to update; NULL returns false.
 * @param forecast Forecast state snapshot to copy, must not be NULL.
 * @return True when the stored value changed.
 */
bool gui_state_set_forecast(gui_state_t *state, const gui_forecast_state_t *forecast);

/**
 * @brief Update the stored Wi-Fi settings model.
 *
 * @param state State object to update; NULL returns false.
 * @param wifi Wi-Fi settings snapshot to copy, must not be NULL.
 * @return True when the stored value changed.
 */
bool gui_state_set_wifi_settings(gui_state_t *state, const gui_wifi_settings_t *wifi);

/**
 * @brief Set the sidebar Wi-Fi indicator state.
 *
 * @param state State object to update; NULL returns false.
 * @param wifi_state New Wi-Fi indicator state.
 * @return True when the stored value changed.
 */
bool gui_state_set_wifi_state(gui_state_t *state, gui_wifi_state_t wifi_state);

/**
 * @brief Set the sidebar Bluetooth indicator state.
 *
 * @param state State object to update; NULL returns false.
 * @param bluetooth_state New Bluetooth indicator state.
 * @return True when the stored value changed.
 */
bool gui_state_set_bluetooth_state(gui_state_t *state, gui_bluetooth_state_t bluetooth_state);

/**
 * @brief Set the sidebar SD card indicator state.
 *
 * @param state State object to update; NULL returns false.
 * @param sd_card_state New SD card indicator state.
 * @return True when the stored value changed.
 */
bool gui_state_set_sd_card_state(gui_state_t *state, gui_sd_card_state_t sd_card_state);

/**
 * @brief Change the active GUI theme.
 *
 * @param state State object to update; NULL returns false.
 * @param theme Theme to apply.
 * @return True when the stored value changed.
 */
bool gui_state_set_theme(gui_state_t *state, gui_view_theme_t theme);

/**
 * @brief Enable or disable the theme background image.
 *
 * @param state State object to update; NULL returns false.
 * @param enabled True to show the background image.
 * @return True when the stored value changed.
 */
bool gui_state_set_background_image_enabled(gui_state_t *state, bool enabled);

/**
 * @brief Enable or disable the theme night variant.
 *
 * @param state State object to update; NULL returns false.
 * @param enabled True to use the night variant when the theme supports it.
 * @return True when the stored value changed.
 */
bool gui_state_set_night_variant_enabled(gui_state_t *state, bool enabled);

/**
 * @brief Update the stored location settings.
 *
 * @param state State object to update; NULL returns false.
 * @param location Location settings snapshot to copy, must not be NULL.
 * @return True when the stored value changed.
 */
bool gui_state_set_location_settings(gui_state_t *state, const gui_location_settings_t *location);

/**
 * @brief Mark the Wi-Fi state as actively scanning.
 *
 * Clears visible scan results and connection selection.
 *
 * @param state State object to update; NULL is ignored.
 */
void gui_state_scan_wifi(gui_state_t *state);

/**
 * @brief Select a scanned Wi-Fi network by index.
 *
 * @param state State object to update; NULL returns false.
 * @param network_index Index into state->wifi.networks, must be less than network_count.
 * @return True when the selection changed and the request was accepted.
 */
bool gui_state_select_wifi_network(gui_state_t *state, uint8_t network_index);

/**
 * @brief Select a known Wi-Fi network and prepare a quick-connect flow.
 *
 * @param state State object to update; NULL returns false.
 * @param network_index Index into state->wifi.known_networks, must be less than known_network_count.
 * @return True when the selection changed and the request was accepted.
 */
bool gui_state_connect_known_wifi(gui_state_t *state, uint8_t network_index);

/**
 * @brief Update the password buffer used by the Wi-Fi password dialog.
 *
 * @param state State object to update; NULL returns false.
 * @param password Null-terminated password string to copy, must not be NULL.
 * @return True when the stored value changed.
 */
bool gui_state_set_wifi_password(gui_state_t *state, const char *password);

/**
 * @brief Mark the current Wi-Fi target as ready for a connection attempt.
 *
 * Validates that a scanned network is selected and that secured networks have
 * a password of at least eight characters.
 *
 * @param state State object to update; NULL returns false.
 * @return True when the connection request state changed.
 */
bool gui_state_connect_wifi(gui_state_t *state);

/**
 * @brief Request disconnection from the current Wi-Fi network.
 *
 * @param state State object to update; NULL returns false.
 * @return True when the disconnect request state changed.
 */
bool gui_state_disconnect_wifi(gui_state_t *state);

/**
 * @brief Find a saved Wi-Fi network by SSID.
 *
 * @param state State object to inspect; NULL returns -1.
 * @param ssid Null-terminated SSID string to search for, must not be NULL.
 * @return Matching known network index, or -1 when no match exists.
 */
int8_t gui_state_find_known_wifi_network(const gui_state_t *state, const char *ssid);

/**
 * @brief Build a renderable screen model from the internal state.
 *
 * @param state Source state snapshot, must not be NULL.
 * @param model Output model populated for the view layer, must not be NULL.
 */
void gui_state_build_screen_model(const gui_state_t *state, gui_view_model_t *model);

#endif
