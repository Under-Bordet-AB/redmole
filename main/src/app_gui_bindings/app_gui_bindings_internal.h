/**
 * @file app_gui_bindings_internal.h
 * @brief Private helpers shared by the application GUI binding modules.
 */

#ifndef APP_GUI_BINDINGS_INTERNAL_H
#define APP_GUI_BINDINGS_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_gui_bindings.h"

#define APP_GUI_BINDINGS_TAG "APP_GUI_BINDINGS"

typedef struct {
    gui_ctx_t *gui;
    EventGroupHandle_t *event_group;

    bool wifi_connect_requested;
    bool wifi_scan_requested;
    bool wifi_disconnect_requested;

    bool has_last_wifi_state;
    gui_wifi_state_t last_wifi_state;

    bool has_last_sd_card_state;
    gui_sd_card_state_t last_sd_card_state;

    bool has_last_appearance;
    gui_appearance_settings_t last_appearance;
    bool has_last_location;
    gui_location_settings_t last_location;

    bool has_last_brightness;
    int32_t last_brightness;

    bool boot_autoconnect_queued;
    char requested_ssid[GUI_WIFI_SSID_MAX_LEN];

    task_node_t forecast_task;
    task_node_t leop_task;
    task_node_t sensor_task;
} app_gui_bindings_ctx_t;

bool app_gui_settings_load_saved_appearance(gui_init_config_t *config);
void app_gui_settings_cache_current_appearance(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui);
void app_gui_settings_cache_current_location(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui);
bool app_gui_settings_load_saved_location(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui);
bool app_gui_settings_save_appearance_if_changed(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui);
bool app_gui_settings_save_location_if_changed(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui);
bool app_gui_settings_load_location_for_forecast(app_gui_bindings_ctx_t *ctx,
                                                double *latitude,
                                                double *longitude);

void app_gui_wifi_fill_bindings(gui_module_bindings_t *bindings,
                                app_gui_bindings_ctx_t *ctx);
bool app_gui_wifi_load_saved_metadata(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui);
bool app_gui_wifi_queue_saved_autoconnect(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui);
bool app_gui_wifi_sync_state(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui);
void app_gui_wifi_sync(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui);

void app_gui_sync_runtime(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui);
void app_gui_sync_register_sensor_task(app_gui_bindings_ctx_t *ctx);

void app_gui_forecast_register_task(app_gui_bindings_ctx_t *ctx);
void app_gui_leop_register_task(app_gui_bindings_ctx_t *ctx);

void app_gui_time_format_unknown_last_updated(char *text, size_t text_len);
void app_gui_time_format_last_updated_now(char *text, size_t text_len);

#endif
