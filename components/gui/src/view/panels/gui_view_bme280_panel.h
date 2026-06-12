#ifndef GUI_VIEW_BME280_PANEL_H
#define GUI_VIEW_BME280_PANEL_H

/**
 * @file gui_view_bme280_panel.h
 * @brief Internal view helpers for the BME280 sensor panel.
 */

#include "../gui_view.h"

/**
 * @brief Create the BME280 panel widgets under the shared content area.
 *
 * @param view View object that stores created widget pointers, must not be NULL.
 * @param content Parent content container, must not be NULL.
 */
void gui_view_init_bme280_panel(gui_view_t *view, lv_obj_t *content);

/**
 * @brief Apply the current sensor model to the BME280 panel widgets.
 *
 * @param view Initialized view object, must not be NULL.
 * @param model Model snapshot to render, must not be NULL.
 */
void gui_view_apply_bme280_panel(gui_view_t *view, const gui_view_model_t *model);

#endif
