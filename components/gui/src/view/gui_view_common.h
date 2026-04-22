#ifndef GUI_VIEW_COMMON_H
#define GUI_VIEW_COMMON_H

#include "lvgl.h"

#include "../gui_defs.h"

void gui_view_set_label_text_if_changed(lv_obj_t *label, const char *text);
void gui_view_set_textarea_text_if_changed(lv_obj_t *textarea, const char *text);
void gui_view_style_scanned_wifi_button(lv_obj_t *button, bool is_selected, bool is_known,
                                        bool is_connected);
bool gui_view_wifi_is_connected(const gui_view_model_t *model, const char *ssid);
int8_t gui_view_find_known_network_index(const gui_view_model_t *model, const char *ssid);
int32_t gui_view_abs_i32(int32_t value);
lv_obj_t *gui_view_create_action_button(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                                        lv_coord_t width, const char *text,
                                        lv_event_code_t event_code, lv_event_cb_t event_cb,
                                        void *event_user_data);
lv_obj_t *gui_view_create_legend_item(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                                      lv_color_t color, const char *text);
lv_obj_t *gui_view_create_metric_card(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                                      const char *label_text, lv_obj_t **value_label);
void gui_view_apply_energy_series(lv_obj_t *chart, lv_chart_series_t *series,
                                  const uint16_t *values);

#endif