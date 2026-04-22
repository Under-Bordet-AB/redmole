#ifndef GUI_VIEW_BME280_PANEL_H
#define GUI_VIEW_BME280_PANEL_H

#include "../gui_view.h"

void gui_view_init_bme280_panel(gui_view_t *view, lv_obj_t *content);
void gui_view_apply_bme280_panel(gui_view_t *view, const gui_view_model_t *model);

#endif