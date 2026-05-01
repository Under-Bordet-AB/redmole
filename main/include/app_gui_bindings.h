/**
 * @file app_gui_bindings.h
 * @brief Bind the application services and persisted state to the GUI module.
 */

#ifndef APP_GUI_BINDINGS_H
#define APP_GUI_BINDINGS_H

#include "esp_err.h"
#include "task_scheduler.h"
#include "gui_module.h"

/**
 * @brief Initialize GUI bindings and restore any saved GUI state.
 *
 * Registers the callback table used by the GUI, restores persisted appearance
 * and remembered Wi-Fi metadata, performs an initial sync, and queues a saved
 * Wi-Fi auto-connect request when possible.
 *
 * @param gui Initialized GUI context to bind to the application.
 * @return ESP_OK on success, or ESP_ERR_INVALID_ARG when @p gui is NULL.
 */
esp_err_t app_gui_bindings_init(gui_ctx_t *gui);

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