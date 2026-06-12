#ifndef APP_GUI_BINDINGS_INTERNAL_H
#define APP_GUI_BINDINGS_INTERNAL_H

/**
 * @file app_gui_bindings_internal.h
 * @brief Private helpers shared by the application GUI binding modules.
 *
 * The binding context is owned by app_gui_bindings.c and passed to focused
 * helper files for settings persistence, Wi-Fi callbacks, synchronization, and
 * scheduled network fetches.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_gui_bindings.h"

/** Log tag shared by the application GUI binding modules. */
#define APP_GUI_BINDINGS_TAG "APP_GUI_BINDINGS"

/**
 * @brief Private runtime state for GUI/application binding helpers.
 */
typedef struct {
    gui_ctx_t *gui;                  /*!< Non-owning pointer to the GUI context being synchronized. */
    EventGroupHandle_t *event_group; /*!< Non-owning pointer to the shared application event group handle. */

    bool wifi_connect_requested;     /*!< True while a GUI-requested Wi-Fi connect is pending. */
    bool wifi_scan_requested;        /*!< True while scan results are expected from NAC. */
    bool wifi_disconnect_requested;  /*!< True while a GUI-requested Wi-Fi disconnect is pending. */

    bool has_last_wifi_state;        /*!< True when last_wifi_state contains a valid cached value. */
    gui_wifi_state_t last_wifi_state; /*!< Last Wi-Fi sidebar state published to the GUI. */

    bool has_last_sd_card_state;     /*!< True when last_sd_card_state contains a valid cached value. */
    gui_sd_card_state_t last_sd_card_state; /*!< Last SD card state published to the GUI. */

    bool has_last_appearance;        /*!< True when last_appearance contains a valid cached value. */
    gui_appearance_settings_t last_appearance; /*!< Last appearance settings persisted or observed. */

    bool has_last_location;          /*!< True when last_location contains a valid cached value. */
    gui_location_settings_t last_location; /*!< Last location settings persisted or observed. */
    bool location_changed;           /*!< True when forecast data should be refreshed for a new location. */

    bool has_last_brightness;        /*!< True when last_brightness contains a valid cached value. */
    int32_t last_brightness;         /*!< Last persisted brightness percentage. */

    bool boot_autoconnect_queued;    /*!< True after saved-network auto-connect has been requested. */
    char requested_ssid[GUI_WIFI_SSID_MAX_LEN]; /*!< Null-terminated SSID targeted by a pending request. */

    task_node_t forecast_task;       /*!< Scheduler node for periodic forecast refresh. */
    task_node_t leop_task;           /*!< Scheduler node for periodic LEOP refresh. */
    task_node_t sensor_task;         /*!< Scheduler node for periodic sensor refresh. */
} app_gui_bindings_ctx_t;

/**
 * @brief Load saved appearance and brightness overrides from NVS.
 *
 * @param config Output startup configuration, must not be NULL.
 * @return True when at least one value was loaded.
 */
bool app_gui_settings_load_saved_appearance(gui_init_config_t *config);

/**
 * @brief Cache the GUI's current appearance and brightness state.
 *
 * @param ctx Binding context to update; NULL is ignored.
 * @param gui GUI context to read from; NULL is ignored.
 */
void app_gui_settings_cache_current_appearance(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui);

/**
 * @brief Cache the GUI's current location settings.
 *
 * @param ctx Binding context to update; NULL is ignored.
 * @param gui GUI context to read from; NULL is ignored.
 */
void app_gui_settings_cache_current_location(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui);

/**
 * @brief Load saved location settings from NVS and apply them to the GUI.
 *
 * Defaults are applied when no saved coordinates exist.
 *
 * @param ctx Binding context to update, must not be NULL.
 * @param gui GUI context to update, must not be NULL.
 * @return True when a location model was applied.
 */
bool app_gui_settings_load_saved_location(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui);

/**
 * @brief Persist appearance or brightness changes to NVS.
 *
 * @param ctx Binding context with cached previous values, must not be NULL.
 * @param gui GUI context to read from, must not be NULL.
 * @return True when one or more values were written successfully.
 */
bool app_gui_settings_save_appearance_if_changed(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui);

/**
 * @brief Persist valid location changes to NVS.
 *
 * Invalid coordinates are replaced by the cached fallback before persistence.
 *
 * @param ctx Binding context with cached previous values, must not be NULL.
 * @param gui GUI context to read from and possibly correct, must not be NULL.
 * @return True when the location was written successfully.
 */
bool app_gui_settings_save_location_if_changed(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui);

/**
 * @brief Read and validate the location used for forecast requests.
 *
 * @param ctx Binding context, must not be NULL.
 * @param latitude Output latitude in decimal degrees, must not be NULL.
 * @param longitude Output longitude in decimal degrees, must not be NULL.
 * @return True when both coordinates were valid and written.
 */
bool app_gui_settings_load_location_for_forecast(app_gui_bindings_ctx_t *ctx,
                                                double *latitude,
                                                double *longitude);

/**
 * @brief Reset persisted GUI settings and restore factory-default GUI state.
 *
 * Erases related NVS keys, requests Wi-Fi disconnect, hides Wi-Fi dialogs, and
 * updates cached settings state.
 *
 * @param gui GUI context to update; NULL is ignored.
 * @param user_data app_gui_bindings_ctx_t pointer supplied as callback user data.
 */
void app_gui_on_reset_requested(gui_ctx_t *gui, void *user_data);

/**
 * @brief Fill a GUI callback table with Wi-Fi and panel handlers.
 *
 * @param bindings Output callback table; NULL is ignored.
 * @param ctx Binding context forwarded as callback user_data.
 */
void app_gui_wifi_fill_bindings(gui_module_bindings_t *bindings,
                                app_gui_bindings_ctx_t *ctx);

/**
 * @brief Load saved Wi-Fi metadata into the GUI model.
 *
 * @param ctx Binding context to update, must not be NULL.
 * @param gui GUI context to update, must not be NULL.
 * @return True when saved metadata was loaded and applied.
 */
bool app_gui_wifi_load_saved_metadata(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui);

/**
 * @brief Queue a saved-network Wi-Fi auto-connect request.
 *
 * Requests NAC to connect using saved credentials and marks boot auto-connect
 * as queued on success.
 *
 * @param ctx Binding context to update, must not be NULL.
 * @param gui GUI context to update, must not be NULL.
 * @return True when the auto-connect request was queued.
 */
bool app_gui_wifi_queue_saved_autoconnect(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui);

/**
 * @brief Synchronize the sidebar Wi-Fi state from NAC.
 *
 * @param ctx Binding context with cached Wi-Fi state, must not be NULL.
 * @param gui GUI context to update, must not be NULL.
 * @return True when the published Wi-Fi state changed.
 */
bool app_gui_wifi_sync_state(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui);

/**
 * @brief Synchronize detailed Wi-Fi dialog state from NAC and pending requests.
 *
 * @param ctx Binding context with pending Wi-Fi request flags, must not be NULL.
 * @param gui GUI context to update, must not be NULL.
 */
void app_gui_wifi_sync(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui);

/**
 * @brief Push current application runtime state into the GUI.
 *
 * Updates the GUI online bit, Wi-Fi state, sensor state, SD card state, and
 * schedules forecast/LEOP refreshes when connectivity or location changes.
 *
 * @param ctx Binding context to synchronize, may be NULL.
 * @param gui GUI context to update, may be NULL.
 */
void app_gui_sync_runtime(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui);

/**
 * @brief Register the periodic sensor refresh scheduler node.
 *
 * @param ctx Binding context containing the scheduler node; NULL is ignored.
 */
void app_gui_sync_register_sensor_task(app_gui_bindings_ctx_t *ctx);

/**
 * @brief Register the periodic forecast refresh scheduler node.
 *
 * @param ctx Binding context containing the scheduler node; NULL is ignored.
 */
void app_gui_forecast_register_task(app_gui_bindings_ctx_t *ctx);

/**
 * @brief Schedule the forecast refresh task to run immediately.
 *
 * @param ctx Binding context containing the scheduler node; NULL or inactive task is ignored.
 */
void app_gui_forecast_schedule_now(app_gui_bindings_ctx_t *ctx);

/**
 * @brief Register the periodic LEOP refresh scheduler node.
 *
 * @param ctx Binding context containing the scheduler node; NULL is ignored.
 */
void app_gui_leop_register_task(app_gui_bindings_ctx_t *ctx);

/**
 * @brief Schedule the LEOP refresh task to run immediately.
 *
 * @param ctx Binding context containing the scheduler node; NULL or inactive task is ignored.
 */
void app_gui_leop_schedule_now(app_gui_bindings_ctx_t *ctx);

/**
 * @brief Write the placeholder last-updated label.
 *
 * @param text Output text buffer; NULL is ignored.
 * @param text_len Size of text in bytes; zero is ignored.
 */
void app_gui_time_format_unknown_last_updated(char *text, size_t text_len);

/**
 * @brief Write a last-updated label using the current local time.
 *
 * Falls back to the unknown placeholder when the system time is not set or the
 * formatted text does not fit.
 *
 * @param text Output text buffer; NULL is ignored.
 * @param text_len Size of text in bytes; zero is ignored.
 */
void app_gui_time_format_last_updated_now(char *text, size_t text_len);

#endif
