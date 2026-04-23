#include "gui_view_energy_panel.h"

#include <string.h>

#include "../gui_view_common.h"

#define GUI_VIEW_ENERGY_CHART_WIDTH 630
#define GUI_VIEW_ENERGY_CHART_Y_AXIS_DRAW_SIZE 52

static bool gui_view_energy_plan_changed(gui_view_t *view, const gui_energy_plan_t *energy_plan)
{
    if ((view == NULL) || (energy_plan == NULL)) {
        return false;
    }

    return !view->has_last_energy_plan ||
           (memcmp(&view->last_energy_plan, energy_plan, sizeof(*energy_plan)) != 0);
}

void gui_view_init_energy_panel(gui_view_t *view, lv_obj_t *content)
{
    // lv_obj_t *energy_caption;
    lv_obj_t *time_row;
    lv_obj_t *time_label;

    if ((view == NULL) || (content == NULL)) {
        return;
    }

    view->energy_plan_panel = lv_obj_create(content);
    lv_obj_set_size(view->energy_plan_panel, 746, 378);
    lv_obj_align(view->energy_plan_panel, LV_ALIGN_BOTTOM_MID, 0, -24);
    lv_obj_set_style_radius(view->energy_plan_panel, 26, 0);
    lv_obj_set_style_bg_color(view->energy_plan_panel, lv_color_hex(0xF8FBFF), 0);
    lv_obj_set_style_bg_opa(view->energy_plan_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(view->energy_plan_panel, 1, 0);
    lv_obj_set_style_border_color(view->energy_plan_panel, lv_color_hex(0xD9E3F1), 0);
    lv_obj_clear_flag(view->energy_plan_panel, LV_OBJ_FLAG_SCROLLABLE);

    // energy_caption = lv_label_create(view->energy_plan_panel);
    // lv_label_set_text(energy_caption, "LEOP recommendation split for the next 24 hours.");
    // lv_obj_set_style_text_color(energy_caption, lv_color_hex(0x4D5F7C), 0);
    // lv_obj_align(energy_caption, LV_ALIGN_TOP_LEFT, 24, 18);

    gui_view_create_legend_item(view->energy_plan_panel, 24, 24, lv_color_hex(0x1D4ED8),
                                "Buy electricity");
    gui_view_create_legend_item(view->energy_plan_panel, 188, 24, lv_color_hex(0xF59E0B),
                                "Use solar directly");
    gui_view_create_legend_item(view->energy_plan_panel, 382, 24, lv_color_hex(0x10B981),
                                "Charge battery");
    gui_view_create_legend_item(view->energy_plan_panel, 544, 24, lv_color_hex(0xEF4444),
                                "Sell excess");

    view->energy_plan_chart = lv_chart_create(view->energy_plan_panel);
    lv_obj_set_size(view->energy_plan_chart, GUI_VIEW_ENERGY_CHART_WIDTH, 232);
    lv_obj_align(view->energy_plan_chart, LV_ALIGN_TOP_MID, 0, 48);
    lv_obj_set_style_bg_color(view->energy_plan_chart, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(view->energy_plan_chart, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(view->energy_plan_chart, 1, 0);
    lv_obj_set_style_border_color(view->energy_plan_chart, lv_color_hex(0xD8E4F0), 0);
    lv_obj_set_style_pad_all(view->energy_plan_chart, 12, 0);
    lv_obj_set_style_line_width(view->energy_plan_chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(view->energy_plan_chart, 0, LV_PART_INDICATOR);
    lv_chart_set_type(view->energy_plan_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(view->energy_plan_chart, GUI_ENERGY_PLAN_POINT_COUNT);
    lv_chart_set_range(view->energy_plan_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_axis_tick(view->energy_plan_chart, LV_CHART_AXIS_PRIMARY_Y, 6, 3, 6, 1, true,
                           GUI_VIEW_ENERGY_CHART_Y_AXIS_DRAW_SIZE);
    lv_chart_set_axis_tick(view->energy_plan_chart, LV_CHART_AXIS_PRIMARY_X, 6, 3, 5, 5, false,
                           20);

    view->buy_series = lv_chart_add_series(view->energy_plan_chart, lv_color_hex(0x1D4ED8),
                                           LV_CHART_AXIS_PRIMARY_Y);
    view->solar_series = lv_chart_add_series(view->energy_plan_chart, lv_color_hex(0xF59E0B),
                                             LV_CHART_AXIS_PRIMARY_Y);
    view->charge_series = lv_chart_add_series(view->energy_plan_chart, lv_color_hex(0x10B981),
                                              LV_CHART_AXIS_PRIMARY_Y);
    view->sell_series = lv_chart_add_series(view->energy_plan_chart, lv_color_hex(0xEF4444),
                                            LV_CHART_AXIS_PRIMARY_Y);

    time_row = lv_obj_create(view->energy_plan_panel);
    lv_obj_set_size(time_row, GUI_VIEW_ENERGY_CHART_WIDTH, 26);
    lv_obj_align(time_row, LV_ALIGN_BOTTOM_MID, 0, -14);
    lv_obj_set_style_bg_opa(time_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(time_row, 0, 0);
    lv_obj_set_style_pad_all(time_row, 0, 0);
    lv_obj_clear_flag(time_row, LV_OBJ_FLAG_SCROLLABLE);

    time_label = lv_label_create(time_row);
    lv_label_set_text(time_label, "00 h");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x607089), 0);
    lv_obj_align(time_label, LV_ALIGN_LEFT_MID, 0, 0);

    time_label = lv_label_create(time_row);
    lv_label_set_text(time_label, "06 h");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x607089), 0);
    lv_obj_align(time_label, LV_ALIGN_LEFT_MID, 164, 0);

    time_label = lv_label_create(time_row);
    lv_label_set_text(time_label, "12 h");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x607089), 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, 0);

    time_label = lv_label_create(time_row);
    lv_label_set_text(time_label, "18 h");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x607089), 0);
    lv_obj_align(time_label, LV_ALIGN_RIGHT_MID, -154, 0);

    time_label = lv_label_create(time_row);
    lv_label_set_text(time_label, "24 h");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x607089), 0);
    lv_obj_align(time_label, LV_ALIGN_RIGHT_MID, 0, 0);
}

void gui_view_apply_energy_panel(gui_view_t *view, const gui_view_model_t *model)
{
    if ((view == NULL) || (model == NULL)) {
        return;
    }

    if (!gui_view_energy_plan_changed(view, &model->energy_plan)) {
        return;
    }

    gui_view_apply_energy_series(view->energy_plan_chart, view->buy_series,
                                 model->energy_plan.buy_electricity);
    gui_view_apply_energy_series(view->energy_plan_chart, view->solar_series,
                                 model->energy_plan.use_solar_directly);
    gui_view_apply_energy_series(view->energy_plan_chart, view->charge_series,
                                 model->energy_plan.charge_battery);
    gui_view_apply_energy_series(view->energy_plan_chart, view->sell_series,
                                 model->energy_plan.sell_excess);
    lv_chart_refresh(view->energy_plan_chart);
    view->last_energy_plan = model->energy_plan;
    view->has_last_energy_plan = true;
}