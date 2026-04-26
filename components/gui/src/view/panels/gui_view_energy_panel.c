#include "gui_view_energy_panel.h"

#include <string.h>

#include "../gui_view_common.h"

#define GUI_VIEW_ENERGY_PANEL_MARGIN 24
#define GUI_VIEW_ENERGY_LEGEND_ITEM_WIDTH 300
#define GUI_VIEW_ENERGY_LEGEND_ROW_GAP 8
#define GUI_VIEW_ENERGY_CHART_Y_AXIS_DRAW_SIZE 52

static void gui_view_layout_energy_panel(gui_view_t *view)
{
    lv_obj_t *legend_container;
    lv_obj_t *time_row;
    lv_coord_t panel_width;
    lv_coord_t panel_height;

    if ((view == NULL) || (view->content == NULL) || (view->energy_plan_panel == NULL)) {
        return;
    }

    panel_width = lv_obj_get_content_width(view->content) - (GUI_VIEW_ENERGY_PANEL_MARGIN * 2);
    panel_height =
        lv_obj_get_content_height(view->content) - (GUI_VIEW_ENERGY_PANEL_MARGIN * 2);

    if ((panel_width <= 0) || (panel_height <= 0)) {
        return;
    }

    lv_obj_set_size(view->energy_plan_panel, panel_width, panel_height);
    lv_obj_align(view->energy_plan_panel, LV_ALIGN_CENTER, 0, 0);

    legend_container = lv_obj_get_child(view->energy_plan_panel, 0);
    time_row = lv_obj_get_child(view->energy_plan_panel, 2);

    if (legend_container != NULL) {
        lv_obj_set_width(legend_container, LV_PCT(100));
    }

    if (view->energy_plan_chart != NULL) {
        lv_obj_set_width(view->energy_plan_chart, LV_PCT(100));
    }

    if (time_row != NULL) {
        lv_obj_set_width(time_row, LV_PCT(100));
    }
}

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
    lv_obj_t *legend_container;
    lv_obj_t *time_row;
    lv_obj_t *time_label;

    if ((view == NULL) || (content == NULL)) {
        return;
    }

    view->energy_plan_panel = lv_obj_create(content);
    lv_obj_set_size(view->energy_plan_panel, 746, 378);
    lv_obj_align(view->energy_plan_panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(view->energy_plan_panel, 26, 0);
    lv_obj_set_style_bg_color(view->energy_plan_panel, lv_color_hex(0xF8FBFF), 0);
    lv_obj_set_style_bg_opa(view->energy_plan_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(view->energy_plan_panel, 1, 0);
    lv_obj_set_style_border_color(view->energy_plan_panel, lv_color_hex(0xD9E3F1), 0);
    lv_obj_set_style_pad_all(view->energy_plan_panel, 24, 0);
    lv_obj_set_style_pad_row(view->energy_plan_panel, 18, 0);
    lv_obj_clear_flag(view->energy_plan_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(view->energy_plan_panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(view->energy_plan_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(view->energy_plan_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    legend_container = lv_obj_create(view->energy_plan_panel);
    lv_obj_set_size(legend_container, LV_PCT(100), 48);
    lv_obj_set_style_bg_opa(legend_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(legend_container, 0, 0);
    lv_obj_set_style_shadow_width(legend_container, 0, 0);
    lv_obj_set_style_pad_all(legend_container, 0, 0);
    lv_obj_set_style_pad_column(legend_container, 20, 0);
    lv_obj_set_style_pad_row(legend_container, GUI_VIEW_ENERGY_LEGEND_ROW_GAP, 0);
    lv_obj_clear_flag(legend_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(legend_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(legend_container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(legend_container, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    view->energy_legend_dots[0] = gui_view_create_legend_item(
        legend_container, 0, 0, GUI_VIEW_ENERGY_LEGEND_ITEM_WIDTH, lv_color_hex(0x1D4ED8),
        "Buy electricity");
    view->energy_legend_dots[1] = gui_view_create_legend_item(
        legend_container, 0, 0, GUI_VIEW_ENERGY_LEGEND_ITEM_WIDTH, lv_color_hex(0xF59E0B),
        "Use solar directly");
    view->energy_legend_dots[2] = gui_view_create_legend_item(
        legend_container, 0, 0,
        GUI_VIEW_ENERGY_LEGEND_ITEM_WIDTH, lv_color_hex(0x10B981), "Charge battery");
    view->energy_legend_dots[3] = gui_view_create_legend_item(
        legend_container, 0, 0, GUI_VIEW_ENERGY_LEGEND_ITEM_WIDTH, lv_color_hex(0xEF4444),
        "Sell excess");

    view->energy_plan_chart = lv_chart_create(view->energy_plan_panel);
    lv_obj_set_size(view->energy_plan_chart, LV_PCT(100), 0);
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
    lv_obj_set_flex_grow(view->energy_plan_chart, 1);

    view->buy_series = lv_chart_add_series(view->energy_plan_chart, lv_color_hex(0x1D4ED8),
                                           LV_CHART_AXIS_PRIMARY_Y);
    view->solar_series = lv_chart_add_series(view->energy_plan_chart, lv_color_hex(0xF59E0B),
                                             LV_CHART_AXIS_PRIMARY_Y);
    view->charge_series = lv_chart_add_series(view->energy_plan_chart, lv_color_hex(0x10B981),
                                              LV_CHART_AXIS_PRIMARY_Y);
    view->sell_series = lv_chart_add_series(view->energy_plan_chart, lv_color_hex(0xEF4444),
                                            LV_CHART_AXIS_PRIMARY_Y);

    time_row = lv_obj_create(view->energy_plan_panel);
    lv_obj_set_size(time_row, LV_PCT(100), 26);
    lv_obj_set_style_bg_opa(time_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(time_row, 0, 0);
    lv_obj_set_style_shadow_width(time_row, 0, 0);
    lv_obj_set_style_pad_all(time_row, 0, 0);
    lv_obj_clear_flag(time_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(time_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(time_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(time_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    time_label = lv_label_create(time_row);
    lv_label_set_text(time_label, "00 h");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x607089), 0);

    time_label = lv_label_create(time_row);
    lv_label_set_text(time_label, "06 h");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x607089), 0);

    time_label = lv_label_create(time_row);
    lv_label_set_text(time_label, "12 h");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x607089), 0);

    time_label = lv_label_create(time_row);
    lv_label_set_text(time_label, "18 h");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x607089), 0);

    time_label = lv_label_create(time_row);
    lv_label_set_text(time_label, "24 h");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x607089), 0);

    gui_view_layout_energy_panel(view);
}

void gui_view_apply_energy_panel(gui_view_t *view, const gui_view_model_t *model)
{
    if ((view == NULL) || (model == NULL)) {
        return;
    }

    gui_view_layout_energy_panel(view);

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
