/**
 * @file gui_view_bme280_panel.h
 * @brief Internal view helpers for the BME280 sensor panel.
 */

#ifndef GUI_VIEW_BME280_PANEL_H
#define GUI_VIEW_BME280_PANEL_H

#include "../gui_view.h"

/**
 * @brief Create the BME280 panel widgets under the shared content area.
 *
 * @param view View object that stores created widget pointers.
 * @param content Parent content container.
 */
void gui_view_init_bme280_panel(gui_view_t *view, lv_obj_t *content);

/**
 * @brief Apply the current sensor model to the BME280 panel widgets.
 *
 * @param view Initialized view object.
 * @param model Model snapshot to render.
 */
void gui_view_apply_bme280_panel(gui_view_t *view, const gui_view_model_t *model);

#endif