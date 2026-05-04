/**
 * @file gui_view_energy_panel.h
 * @brief Internal view helpers for the energy plan panel.
 */

#ifndef GUI_VIEW_ENERGY_PANEL_H
#define GUI_VIEW_ENERGY_PANEL_H

#include "../gui_view.h"

/**
 * @brief Create the energy plan panel widgets under the shared content area.
 *
 * @param view View object that stores created widget pointers.
 * @param content Parent content container.
 */
void gui_view_init_energy_panel(gui_view_t *view, lv_obj_t *content);

/**
 * @brief Apply the current energy plan model to the energy panel widgets.
 *
 * @param view Initialized view object.
 * @param model Model snapshot to render.
 */
void gui_view_apply_energy_panel(gui_view_t *view, const gui_view_model_t *model);

#endif