#include "gui_view_bme280_panel.h"

#include <inttypes.h>
#include <stdio.h>

#include "../gui_view_common.h"

void gui_view_init_bme280_panel(gui_view_t *view, lv_obj_t *content)
{
    if ((view == NULL) || (content == NULL)) {
        return;
    }

    view->bme280_panel = lv_obj_create(content);
    lv_obj_set_size(view->bme280_panel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(view->bme280_panel, LV_ALIGN_TOP_LEFT, 34, 118);
    lv_obj_set_layout(view->bme280_panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(view->bme280_panel, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(view->bme280_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(view->bme280_panel, 24, 0);
    lv_obj_set_style_pad_column(view->bme280_panel, 14, 0);
    lv_obj_set_style_pad_row(view->bme280_panel, 14, 0);
    lv_obj_set_style_radius(view->bme280_panel, 26, 0);
    lv_obj_set_style_bg_color(view->bme280_panel, lv_color_hex(0xEFF5FB), 0);
    lv_obj_set_style_bg_opa(view->bme280_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(view->bme280_panel, 0, 0);
    lv_obj_clear_flag(view->bme280_panel, LV_OBJ_FLAG_SCROLLABLE);

    gui_view_create_metric_card(view->bme280_panel, 0, 0, "Temperature", &view->temperature_value);
    gui_view_create_metric_card(view->bme280_panel, 0, 0, "Humidity", &view->humidity_value);
    gui_view_create_metric_card(view->bme280_panel, 0, 0, "Pressure", &view->pressure_value);
}

void gui_view_apply_bme280_panel(gui_view_t *view, const gui_view_model_t *model)
{
    char temperature_text[32];
    char humidity_text[32];
    char pressure_text[32];

    if ((view == NULL) || (model == NULL)) {
        return;
    }

    snprintf(temperature_text, sizeof(temperature_text), "%" PRId32 ".%" PRId32 " C",
             model->sensor.temperature_deci_c / 10,
             gui_view_abs_i32(model->sensor.temperature_deci_c % 10));
    snprintf(humidity_text, sizeof(humidity_text), "%" PRId32 ".%" PRId32 " %%",
             model->sensor.humidity_deci_pct / 10,
             gui_view_abs_i32(model->sensor.humidity_deci_pct % 10));
    snprintf(pressure_text, sizeof(pressure_text), "%" PRId32 ".%" PRId32 " hPa",
             model->sensor.pressure_deci_hpa / 10,
             gui_view_abs_i32(model->sensor.pressure_deci_hpa % 10));

    gui_view_set_label_text_if_changed(view->temperature_value, temperature_text);
    gui_view_set_label_text_if_changed(view->humidity_value, humidity_text);
    gui_view_set_label_text_if_changed(view->pressure_value, pressure_text);
}