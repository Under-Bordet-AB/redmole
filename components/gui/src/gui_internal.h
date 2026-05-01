/**
 * @file gui_internal.h
 * @brief Internal runtime structures and render entry points for the GUI module.
 */

#ifndef GUI_INTERNAL_H
#define GUI_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "gui_module.h"
#include "gui_screen.h"
#include "gui_state.h"
#include "lvgl.h"

/** Log tag used by the GUI module implementation. */
#define GUI_MODULE_TAG "gui"
/** Default display brightness applied during GUI initialization. */
#define GUI_MODULE_DEFAULT_BRIGHTNESS 82

/**
 * @brief Internal runtime object backing a public gui_ctx_t instance.
 */
typedef struct {
    gui_state_t state;                 /*!< Mutable GUI state owned by the runtime. */
    gui_screen_t screen;               /*!< Screen/view object used to render the current model. */
    lv_timer_t *refresh_timer;         /*!< Periodic LVGL timer used to trigger refresh work. */
    gui_ctx_t *owner;                  /*!< Back-reference to the owning public GUI context. */
    gui_module_bindings_t bindings;    /*!< Application callbacks currently registered with the GUI. */
} gui_runtime_t;

/**
 * @brief Resolve the internal runtime object associated with a public GUI context.
 *
 * @param self GUI context initialized by gui_init().
 * @return Pointer to the internal runtime, or NULL when the context is not initialized.
 */
gui_runtime_t *gui_get_runtime(gui_ctx_t *self);

/**
 * @brief Rebuild and apply the current screen model for an initialized runtime.
 *
 * @param runtime Internal runtime to render.
 */
void gui_render_runtime(gui_runtime_t *runtime);

#endif
