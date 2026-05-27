#include "gui_view_energy_panel.h"

#include <string.h>

#include "../gui_view_common.h"

#define GUI_VIEW_ENERGY_PANEL_MARGIN 24
#define GUI_VIEW_ENERGY_LEGEND_ITEM_WIDTH 300
#define GUI_VIEW_ENERGY_LEGEND_COLUMN_GAP 20
#define GUI_VIEW_ENERGY_LEGEND_LABEL_X_OFFSET 18
#define GUI_VIEW_ENERGY_LEGEND_ROW_GAP 8
#define GUI_VIEW_ENERGY_CHART_HORIZONTAL_INSET 36
#define GUI_VIEW_ENERGY_CHART_Y_AXIS_DRAW_SIZE 52
#define GUI_VIEW_ENERGY_CHART_Y_AXIS_MIN 0
#define GUI_VIEW_ENERGY_CHART_Y_AXIS_MAX 1000
#define GUI_VIEW_ENERGY_CHART_VALUE_SCALE 1000
#define GUI_VIEW_ENERGY_TIME_LABEL_STEP_HOURS 6
#define GUI_VIEW_ENERGY_TIME_LABEL_SLOT_WIDTH 72
#define GUI_VIEW_ENERGY_TIME_LABEL_X_OFFSET 6

static void gui_view_energy_format_time_label(char *label,
                                              size_t label_len,
                                              uint8_t start_hour,
                                              uint8_t offset_hours)
{
    uint8_t normalized_start_hour;
    uint8_t display_hour;

    if ((label == NULL) || (label_len == 0U)) {
        return;
    }

    normalized_start_hour = (uint8_t)(start_hour % GUI_ENERGY_PLAN_POINT_COUNT);
    if ((normalized_start_hour == 0U) && (offset_hours == GUI_ENERGY_PLAN_POINT_COUNT)) {
        lv_snprintf(label, label_len, "24 h");
        return;
    }

    display_hour =
        (uint8_t)((normalized_start_hour + offset_hours) % GUI_ENERGY_PLAN_POINT_COUNT);
    lv_snprintf(label, label_len, "%02u h", (unsigned int)display_hour);
}

static void gui_view_update_energy_time_labels(gui_view_t *view, uint8_t start_hour)
{
    uint8_t label_index;

    if (view == NULL) {
        return;
    }

    for (label_index = 0; label_index < GUI_ENERGY_PLAN_TIME_LABEL_COUNT; label_index++) {
        char label_text[8];
        uint8_t offset_hours =
            (uint8_t)(label_index * GUI_VIEW_ENERGY_TIME_LABEL_STEP_HOURS);

        if (view->energy_time_labels[label_index] == NULL) {
            continue;
        }

        gui_view_energy_format_time_label(label_text, sizeof(label_text), start_hour,
                                          offset_hours);
        lv_label_set_text(view->energy_time_labels[label_index], label_text);
    }
}

static void gui_view_energy_chart_draw_event_cb(lv_event_t *event)
{
    lv_obj_draw_part_dsc_t *draw_part;

    if (event == NULL) {
        return;
    }

    draw_part = lv_event_get_draw_part_dsc(event);
    if (!lv_obj_draw_part_check_type(draw_part, &lv_chart_class,
                                     LV_CHART_DRAW_PART_TICK_LABEL) ||
        (draw_part->id != LV_CHART_AXIS_PRIMARY_Y) || (draw_part->text == NULL)) {
        return;
    }

    lv_snprintf(draw_part->text, draw_part->text_length, "%d.%d",
                (int)(draw_part->value / GUI_VIEW_ENERGY_CHART_VALUE_SCALE),
                (int)((draw_part->value % GUI_VIEW_ENERGY_CHART_VALUE_SCALE) /
                      (GUI_VIEW_ENERGY_CHART_VALUE_SCALE / 10)));
}

static lv_coord_t gui_view_energy_chart_x_tick_offset(lv_obj_t *chart,
                                                      lv_coord_t chart_width,
                                                      uint8_t label_index)
{
    lv_coord_t border_width;
    lv_coord_t pad_left;
    lv_coord_t pad_right;
    lv_coord_t plot_width;
    uint8_t last_label_index;

    if ((chart == NULL) || (chart_width <= 0)) {
        return 0;
    }

    last_label_index = (uint8_t)(GUI_ENERGY_PLAN_TIME_LABEL_COUNT - 1U);
    if (last_label_index == 0U) {
        return 0;
    }

    border_width = lv_obj_get_style_border_width(chart, LV_PART_MAIN);
    pad_left = lv_obj_get_style_pad_left(chart, LV_PART_MAIN);
    pad_right = lv_obj_get_style_pad_right(chart, LV_PART_MAIN);
    plot_width = chart_width - pad_left - pad_right - (border_width * 2);
    if (plot_width < 0) {
        plot_width = 0;
    }

    return pad_left + border_width + GUI_VIEW_ENERGY_TIME_LABEL_X_OFFSET +
           (plot_width * label_index) / last_label_index;
}

static void gui_view_layout_energy_legend_items(gui_view_t *view, lv_coord_t chart_width)
{
    lv_coord_t item_width;
    uint8_t item_index;

    if ((view == NULL) || (chart_width <= GUI_VIEW_ENERGY_LEGEND_COLUMN_GAP)) {
        return;
    }

    item_width = (chart_width - GUI_VIEW_ENERGY_LEGEND_COLUMN_GAP) / 2;
    if (item_width > GUI_VIEW_ENERGY_LEGEND_ITEM_WIDTH) {
        item_width = GUI_VIEW_ENERGY_LEGEND_ITEM_WIDTH;
    }
    if (item_width <= GUI_VIEW_ENERGY_LEGEND_LABEL_X_OFFSET) {
        return;
    }

    for (item_index = 0;
         item_index < (sizeof(view->energy_legend_dots) / sizeof(view->energy_legend_dots[0]));
         item_index++) {
        lv_obj_t *item;

        if (view->energy_legend_dots[item_index] == NULL) {
            continue;
        }

        item = lv_obj_get_parent(view->energy_legend_dots[item_index]);
        if (item != NULL) {
            lv_obj_set_width(item, item_width);
        }
        if (view->energy_legend_labels[item_index] != NULL) {
            lv_obj_set_width(view->energy_legend_labels[item_index],
                             item_width - GUI_VIEW_ENERGY_LEGEND_LABEL_X_OFFSET);
        }
    }
}

static void gui_view_layout_energy_time_labels(gui_view_t *view,
                                               lv_obj_t *time_row,
                                               lv_coord_t panel_content_width,
                                               lv_coord_t chart_width)
{
    lv_coord_t chart_left;
    uint8_t label_index;

    if ((view == NULL) || (time_row == NULL) || (panel_content_width <= 0) ||
        (chart_width <= 0)) {
        return;
    }

    chart_left = (panel_content_width - chart_width) / 2;

    for (label_index = 0; label_index < GUI_ENERGY_PLAN_TIME_LABEL_COUNT; label_index++) {
        lv_obj_t *time_label = view->energy_time_labels[label_index];
        lv_coord_t tick_x;
        lv_coord_t label_x;

        if (time_label == NULL) {
            continue;
        }

        tick_x = chart_left + gui_view_energy_chart_x_tick_offset(
                                  view->energy_plan_chart, chart_width, label_index);
        label_x = tick_x - (GUI_VIEW_ENERGY_TIME_LABEL_SLOT_WIDTH / 2);

        lv_obj_set_width(time_label, GUI_VIEW_ENERGY_TIME_LABEL_SLOT_WIDTH);
        lv_obj_align(time_label, LV_ALIGN_LEFT_MID, label_x, 0);
    }
}

static void gui_view_layout_energy_panel(gui_view_t *view)
{
    lv_obj_t *legend_container;
    lv_obj_t *time_row;
    lv_coord_t panel_content_width;
    lv_coord_t panel_width;
    lv_coord_t panel_height;
    lv_coord_t chart_width;

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

    panel_content_width = lv_obj_get_content_width(view->energy_plan_panel);
    chart_width = panel_content_width - (GUI_VIEW_ENERGY_CHART_HORIZONTAL_INSET * 2);
    if (chart_width <= 0) {
        return;
    }

    if (legend_container != NULL) {
        lv_obj_set_width(legend_container, chart_width);
        gui_view_layout_energy_legend_items(view, chart_width);
    }
    if (view->energy_plan_chart != NULL) {
        lv_obj_set_width(view->energy_plan_chart, chart_width);
        lv_obj_align(view->energy_plan_chart, LV_ALIGN_CENTER, 0, 0);
    }

    if (time_row != NULL) {
        lv_obj_set_width(time_row, panel_content_width);
        gui_view_layout_energy_time_labels(view, time_row, panel_content_width,
                                           chart_width);
    }
}

static bool gui_view_energy_plan_changed(gui_view_t *view, const gui_energy_plan_t *energy_plan)
{
    if ((view == NULL) || (energy_plan == NULL)) {
        return false;
    }

    return !view->has_last_energy_plan ||
           (memcmp(view->last_energy_plan.buy_electricity, energy_plan->buy_electricity,
                   sizeof(energy_plan->buy_electricity)) != 0) ||
           (memcmp(view->last_energy_plan.use_solar_directly,
                   energy_plan->use_solar_directly,
                   sizeof(energy_plan->use_solar_directly)) != 0) ||
           (memcmp(view->last_energy_plan.charge_battery, energy_plan->charge_battery,
                   sizeof(energy_plan->charge_battery)) != 0) ||
           (memcmp(view->last_energy_plan.sell_excess, energy_plan->sell_excess,
                   sizeof(energy_plan->sell_excess)) != 0) ||
           (view->last_energy_plan.start_hour != energy_plan->start_hour);
}

void gui_view_init_energy_panel(gui_view_t *view, lv_obj_t *content)
{
    lv_obj_t *legend_container;
    lv_obj_t *time_row;
    lv_obj_t *time_label;
    uint8_t label_index;

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
    lv_obj_set_style_pad_column(legend_container, GUI_VIEW_ENERGY_LEGEND_COLUMN_GAP, 0);
    lv_obj_set_style_pad_row(legend_container, GUI_VIEW_ENERGY_LEGEND_ROW_GAP, 0);
    lv_obj_clear_flag(legend_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(legend_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(legend_container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(legend_container, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    view->energy_legend_dots[0] = gui_view_create_legend_item(
        legend_container, 0, 0, GUI_VIEW_ENERGY_LEGEND_ITEM_WIDTH, lv_color_hex(0x1D4ED8),
        "Buy electricity", &view->energy_legend_labels[0]);
    view->energy_legend_dots[1] = gui_view_create_legend_item(
        legend_container, 0, 0, GUI_VIEW_ENERGY_LEGEND_ITEM_WIDTH, lv_color_hex(0xF59E0B),
        "Use solar directly", &view->energy_legend_labels[1]);
    view->energy_legend_dots[2] = gui_view_create_legend_item(
        legend_container, 0, 0, GUI_VIEW_ENERGY_LEGEND_ITEM_WIDTH, lv_color_hex(0x10B981),
        "Charge battery", &view->energy_legend_labels[2]);
    view->energy_legend_dots[3] = gui_view_create_legend_item(
        legend_container, 0, 0, GUI_VIEW_ENERGY_LEGEND_ITEM_WIDTH, lv_color_hex(0xEF4444),
        "Sell excess", &view->energy_legend_labels[3]);

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
    lv_chart_set_range(view->energy_plan_chart, LV_CHART_AXIS_PRIMARY_Y,
                       GUI_VIEW_ENERGY_CHART_Y_AXIS_MIN,
                       GUI_VIEW_ENERGY_CHART_Y_AXIS_MAX);
    lv_chart_set_axis_tick(view->energy_plan_chart, LV_CHART_AXIS_PRIMARY_Y, 6, 3, 6, 1, true,
                           GUI_VIEW_ENERGY_CHART_Y_AXIS_DRAW_SIZE);
    lv_chart_set_axis_tick(view->energy_plan_chart, LV_CHART_AXIS_PRIMARY_X, 6, 3, 5, 5, false,
                           20);
    lv_obj_add_event_cb(view->energy_plan_chart, gui_view_energy_chart_draw_event_cb,
                        LV_EVENT_DRAW_PART_BEGIN, NULL);
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

    for (label_index = 0; label_index < GUI_ENERGY_PLAN_TIME_LABEL_COUNT; label_index++) {
        time_label = lv_label_create(time_row);
        view->energy_time_labels[label_index] = time_label;
        lv_label_set_long_mode(time_label, LV_LABEL_LONG_CLIP);
        lv_obj_set_width(time_label, GUI_VIEW_ENERGY_TIME_LABEL_SLOT_WIDTH);
        lv_obj_set_style_text_color(time_label, lv_color_hex(0x607089), 0);
        lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_CENTER, 0);
    }
    gui_view_update_energy_time_labels(view, 0U);

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

    gui_view_update_energy_time_labels(view, model->energy_plan.start_hour);

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
