#ifndef APP_GUI_BINDINGS_H
#define APP_GUI_BINDINGS_H

/**
 * @file app_gui_bindings.h
 * @brief Bind the application services and persisted state to the GUI module.
 *
 * The binding layer translates application services, NVS-backed settings, and
 * scheduled network fetches into the copy-by-value GUI model.
 */

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "task_scheduler.h"
#include "gui_module.h"

/**
 * @brief Load saved GUI appearance settings for startup.
 *
 * Reads any persisted appearance and brightness overrides that should be
 * applied before the GUI performs its first render.
 *
 * @param config Output startup configuration to populate, must not be NULL.
 * @return True when at least one saved value was loaded, otherwise false.
 */
bool app_gui_bindings_load_saved_appearance(gui_init_config_t *config);

/**
 * @brief Initialize GUI bindings and restore saved runtime GUI state.
 *
 * Registers the callback table used by the GUI, restores saved location and
 * Wi-Fi metadata, performs an initial sync, and registers scheduled sensor,
 * forecast, and LEOP refresh tasks.
 *
 * @param gui Initialized GUI context to bind to the application, must not be NULL.
 * @param event_group Shared application event group handle pointer used for UART status bits, must not be NULL.
 * @return ESP_OK on success, or ESP_ERR_INVALID_ARG when an argument is invalid.
 */
esp_err_t app_gui_bindings_init(gui_ctx_t *gui, EventGroupHandle_t *event_group);

/**
 * @brief Push the latest application state into the GUI view model.
 *
 * Synchronizes sensor values, Wi-Fi state, SD card state, and any pending GUI
 * appearance and location changes into the GUI-owned state. Appearance and
 * location changes may be persisted to NVS.
 *
 * @param gui Initialized GUI context to update; NULL skips GUI writes but may clear the GUI online bit.
 */
void app_gui_bindings_sync(gui_ctx_t *gui);

#endif
