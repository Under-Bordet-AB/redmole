#ifndef GUI_VIEW_FORECAST_PANEL_H
#define GUI_VIEW_FORECAST_PANEL_H

/**
 * @file gui_view_forecast_panel.h
 * @brief Internal view helpers for the forecast panel.
 */

#include "../gui_view.h"

/**
 * @brief Create the forecast panel widgets under the shared content area.
 *
 * @param view View object that stores created widget pointers, must not be NULL.
 * @param content Parent content container, must not be NULL.
 */
void gui_view_init_forecast_panel(gui_view_t *view, lv_obj_t *content);

/**
 * @brief Apply the current forecast model state to the forecast panel widgets.
 *
 * @param view Initialized view object, must not be NULL.
 * @param model Model snapshot to render, must not be NULL.
 */
void gui_view_apply_forecast_panel(gui_view_t *view, const gui_view_model_t *model);

#endif
