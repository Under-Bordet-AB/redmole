/**
 * @file gui_platform.h
 * @brief Internal platform hooks for display control and periodic GUI refresh.
 */

#ifndef GUI_PLATFORM_H
#define GUI_PLATFORM_H

#include <stdint.h>

#include "esp_err.h"

#include "gui_internal.h"

/**
 * @brief Set the display brightness through the platform abstraction.
 *
 * @param brightness_percent Brightness percentage to apply.
 */
void gui_platform_set_brightness(int32_t brightness_percent);

/**
 * @brief Read the current display brightness from the platform abstraction.
 *
 * @return Current brightness percentage.
 */
int32_t gui_platform_get_brightness(void);

/**
 * @brief Initialize the display hardware and GUI platform bindings.
 *
 * @return ESP_OK on success, or an ESP-IDF error code on failure.
 */
esp_err_t gui_platform_init_display(void);

/**
 * @brief Start the periodic refresh timer used by the GUI runtime.
 *
 * @param self Public GUI context that owns the runtime.
 * @param runtime Internal runtime whose render loop should be scheduled.
 */
void gui_platform_start_refresh_timer(gui_ctx_t *self, gui_runtime_t *runtime);

/**
 * @brief Stop and release the periodic refresh timer for the runtime.
 *
 * @param runtime Internal runtime whose refresh timer should be stopped.
 */
void gui_platform_stop_refresh_timer(gui_runtime_t *runtime);

#endif
