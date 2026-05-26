/**
 * @file app_gui_bindings.h
 * @brief Bind the application services and persisted state to the GUI module.
 */

#ifndef APP_GUI_BINDINGS_H
#define APP_GUI_BINDINGS_H

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
 * @param config Output startup configuration to populate.
 * @return True when at least one saved value was loaded, otherwise false.
 */
bool app_gui_bindings_load_saved_appearance(gui_init_config_t *config);

/**
 * @brief Initialize GUI bindings and restore saved runtime GUI state.
 *
 * Registers the callback table used by the GUI, restores remembered Wi-Fi
 * metadata, performs an initial sync, and queues a saved Wi-Fi auto-connect
 * request when possible.
 *
 * @param gui Initialized GUI context to bind to the application.
 * @param event_group Shared application event group used for UART status bits.
 * @return ESP_OK on success, or ESP_ERR_INVALID_ARG when an argument is invalid.
 */
esp_err_t app_gui_bindings_init(gui_ctx_t *gui, EventGroupHandle_t *event_group);

/**
 * @brief Push the latest application state into the GUI view model.
 *
 * Synchronizes sensor values, Wi-Fi state, SD card state, and any pending GUI
 * appearance changes into the GUI-owned state.
 *
 * @param gui Initialized GUI context to update.
 */
void app_gui_bindings_sync(gui_ctx_t *gui);

#endif
