#ifndef GUI_VIEW_ENERGY_PANEL_H
#define GUI_VIEW_ENERGY_PANEL_H

/**
 * @file gui_view_energy_panel.h
 * @brief Internal view helpers for the energy plan panel.
 */

#include "../gui_view.h"

/**
 * @brief Create the energy plan panel widgets under the shared content area.
 *
 * @param view View object that stores created widget pointers, must not be NULL.
 * @param content Parent content container, must not be NULL.
 * @param settings_event_cb Callback for mode switch interactions, may be NULL.
 * @param event_user_data Caller-owned user data forwarded to the callback.
 */
void gui_view_init_energy_panel(gui_view_t *view, lv_obj_t *content,
                                lv_event_cb_t settings_event_cb, void *event_user_data);

/**
 * @brief Apply the current energy plan model to the energy panel widgets.
 *
 * @param view Initialized view object, must not be NULL.
 * @param model Model snapshot to render, must not be NULL.
 */
void gui_view_apply_energy_panel(gui_view_t *view, const gui_view_model_t *model);

#endif
