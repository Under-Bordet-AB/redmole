/**
 * @file gui_module.h
 * @brief Public lifecycle and state API for the GUI module.
 */

#ifndef GUI_MODULE_H
#define GUI_MODULE_H

#include <stdbool.h>
#include "freertos/FreeRTOS.h"

#include "gui_types.h"

/**
 * @brief Opaque-like context owned by the caller and initialized by the GUI module.
 */
typedef struct gui_ctx {
    bool is_ready;
    void *frame_buffer;
    void *module_state;
} gui_ctx_t;

/**
 * @brief Optional startup overrides applied before the first GUI render.
 */
typedef struct {
    bool has_theme;
    gui_view_theme_t theme;
    bool has_background_image;
    bool show_background_image;
    bool has_night_variant;
    bool night_variant_enabled;
    bool has_brightness;
    int32_t brightness_percent;
} gui_init_config_t;

/**
 * @brief Callback bindings used to connect GUI interactions to application logic.
 */
typedef struct {
    void *user_data;
    void (*on_panel_changed)(gui_ctx_t *self, gui_panel_id_t panel, void *user_data);
    bool (*on_wifi_scan_requested)(gui_ctx_t *self, void *user_data);
    void (*on_wifi_network_selected)(gui_ctx_t *self, const gui_wifi_network_t *network, void *user_data);
    bool (*on_wifi_known_network_requested)(gui_ctx_t *self, const gui_wifi_network_t *network, void *user_data);
    bool (*on_wifi_connect_requested)(gui_ctx_t *self, const char *ssid, const char *password, void *user_data);
    bool (*on_wifi_disconnect_requested)(gui_ctx_t *self, void *user_data);
} gui_module_bindings_t;

/**
 * @brief Initialize the GUI module and its backing runtime state.
 *
 * @param self GUI context to initialize.
 * @param config Optional startup overrides applied before the first render.
 */
void gui_init(gui_ctx_t *self, const gui_init_config_t *config);

/**
 * @brief Run one GUI processing iteration.
 *
 * Processes LVGL timers and performs any pending rendering work.
 *
 * @param self Initialized GUI context.
 */
void gui_run(gui_ctx_t *self);

/**
 * @brief Tear down the GUI module and release owned resources.
 *
 * @param self GUI context to deinitialize.
 */
void gui_deinit(gui_ctx_t *self);

/**
 * @brief Register or replace the application callbacks used by the GUI.
 *
 * @param self Initialized GUI context.
 * @param bindings Callback table copied by the GUI module.
 */
void gui_set_bindings(gui_ctx_t *self, const gui_module_bindings_t *bindings);

/**
 * @brief Force the GUI to rebuild and apply its current view model.
 *
 * @param self Initialized GUI context.
 */
void gui_refresh(gui_ctx_t *self);

/**
 * @brief Set the currently visible top-level panel.
 *
 * @param self Initialized GUI context.
 * @param panel Panel identifier to show.
 */
void gui_set_active_panel(gui_ctx_t *self, gui_panel_id_t panel);

/**
 * @brief Read back the currently visible top-level panel.
 *
 * @param self Initialized GUI context.
 * @param panel Output pointer that receives the active panel on success.
 * @return True when the context is ready and the active panel was written.
 */
bool gui_get_active_panel(gui_ctx_t *self, gui_panel_id_t *panel);

/**
 * @brief Update the latest sensor values used by the GUI.
 *
 * @param self Initialized GUI context.
 * @param sensor Sensor values to copy into the GUI state.
 */
void gui_set_sensor_state(gui_ctx_t *self, const gui_sensor_state_t *sensor);

/**
 * @brief Read back the current sensor values stored by the GUI.
 *
 * @param self Initialized GUI context.
 * @param sensor Output pointer that receives the current sensor state on success.
 * @return True when the context is ready and the sensor state was written.
 */
bool gui_get_sensor_state(gui_ctx_t *self, gui_sensor_state_t *sensor);

/**
 * @brief Replace the Wi-Fi settings model consumed by the GUI.
 *
 * @param self Initialized GUI context.
 * @param wifi Wi-Fi settings snapshot to copy.
 */
void gui_set_wifi_settings(gui_ctx_t *self, const gui_wifi_settings_t *wifi);

/**
 * @brief Read back the Wi-Fi settings currently stored by the GUI.
 *
 * @param self Initialized GUI context.
 * @param wifi Output pointer that receives the current Wi-Fi settings on success.
 * @return True when the context is ready and the Wi-Fi settings were written.
 */
bool gui_get_wifi_settings(gui_ctx_t *self, gui_wifi_settings_t *wifi);

/**
 * @brief Update the sidebar Wi-Fi status indicator state.
 *
 * @param self Initialized GUI context.
 * @param state New Wi-Fi status value.
 */
void gui_set_wifi_state(gui_ctx_t *self, gui_wifi_state_t state);

/**
 * @brief Read back the current sidebar Wi-Fi status indicator state.
 *
 * @param self Initialized GUI context.
 * @param state Output pointer that receives the current Wi-Fi state on success.
 * @return True when the context is ready and the Wi-Fi state was written.
 */
bool gui_get_wifi_state(gui_ctx_t *self, gui_wifi_state_t *state);

/**
 * @brief Update the sidebar Bluetooth status indicator state.
 *
 * @param self Initialized GUI context.
 * @param state New Bluetooth status value.
 */
void gui_set_bluetooth_state(gui_ctx_t *self, gui_bluetooth_state_t state);

/**
 * @brief Read back the current sidebar Bluetooth status indicator state.
 *
 * @param self Initialized GUI context.
 * @param state Output pointer that receives the current Bluetooth state on success.
 * @return True when the context is ready and the Bluetooth state was written.
 */
bool gui_get_bluetooth_state(gui_ctx_t *self, gui_bluetooth_state_t *state);

/**
 * @brief Update the sidebar SD card status indicator state.
 *
 * @param self Initialized GUI context.
 * @param state New SD card status value.
 */
void gui_set_sd_card_state(gui_ctx_t *self, gui_sd_card_state_t state);

/**
 * @brief Read back the current sidebar SD card status indicator state.
 *
 * @param self Initialized GUI context.
 * @param state Output pointer that receives the current SD card state on success.
 * @return True when the context is ready and the SD card state was written.
 */
bool gui_get_sd_card_state(gui_ctx_t *self, gui_sd_card_state_t *state);

/**
 * @brief Update the user-facing appearance settings.
 *
 * @param self Initialized GUI context.
 * @param appearance Appearance settings snapshot to copy.
 */
void gui_set_appearance_settings(gui_ctx_t *self, const gui_appearance_settings_t *appearance);

/**
 * @brief Read back the current appearance settings.
 *
 * @param self Initialized GUI context.
 * @param appearance Output pointer that receives the current appearance settings on success.
 * @return True when the context is ready and the appearance settings were written.
 */
bool gui_get_appearance_settings(gui_ctx_t *self, gui_appearance_settings_t *appearance);

/**
 * @brief Update the user-editable location settings.
 *
 * @param self Initialized GUI context.
 * @param location Location settings snapshot to copy.
 */
void gui_set_location_settings(gui_ctx_t *self, const gui_location_settings_t *location);

/**
 * @brief Read back the current location settings.
 *
 * @param self Initialized GUI context.
 * @param location Output pointer that receives the current location settings on success.
 * @return True when the context is ready and the location settings were written.
 */
bool gui_get_location_settings(gui_ctx_t *self, gui_location_settings_t *location);

/**
 * @brief Set the display brightness used by the GUI platform layer.
 *
 * @param self Initialized GUI context.
 * @param brightness_percent Brightness percentage in the range expected by the platform.
 */
void gui_set_brightness(gui_ctx_t *self, int32_t brightness_percent);

/**
 * @brief Read back the current display brightness setting.
 *
 * @param self Initialized GUI context.
 * @param brightness_percent Output pointer that receives the current brightness on success.
 * @return True when the context is ready and the brightness value was written.
 */
bool gui_get_brightness(gui_ctx_t *self, int32_t *brightness_percent);

/**
 * @brief Show the Wi-Fi network selection dialog.
 *
 * @param self Initialized GUI context.
 */
void gui_show_wifi_network_dialog(gui_ctx_t *self);

/**
 * @brief Show the Wi-Fi password entry dialog.
 *
 * @param self Initialized GUI context.
 */
void gui_show_wifi_password_dialog(gui_ctx_t *self);

/**
 * @brief Hide any active Wi-Fi-related modal dialogs.
 *
 * @param self Initialized GUI context.
 */
void gui_hide_wifi_dialogs(gui_ctx_t *self);

#endif // GUI_MODULE_H
