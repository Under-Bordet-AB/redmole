/**
 * @file gui_view_energy_panel.c
 * @brief LVGL widgets and chart updates for the energy plan panel.
 */

#include "gui_view_energy_panel.h"

#include <string.h>

#include "../gui_theme_defs.h"
#include "../gui_view_common.h"

#define GUI_VIEW_ENERGY_PANEL_MARGIN 24
#define GUI_VIEW_ENERGY_MODE_ROW_HEIGHT 40
#define GUI_VIEW_ENERGY_MODE_BUTTON_WIDTH 104
#define GUI_VIEW_ENERGY_MODE_BUTTON_HEIGHT 34
#define GUI_VIEW_ENERGY_OVERVIEW_HEIGHT 128
#define GUI_VIEW_ENERGY_ACTION_STRIP_HEIGHT 26
#define GUI_VIEW_ENERGY_ACTION_SEGMENT_HEIGHT 14
#define GUI_VIEW_ENERGY_ACTION_SEGMENT_GAP 4
#define GUI_VIEW_ENERGY_LEGEND_ITEM_WIDTH 258
#define GUI_VIEW_ENERGY_LEGEND_COLUMN_GAP 0
#define GUI_VIEW_ENERGY_LEGEND_LABEL_X_OFFSET 18
#define GUI_VIEW_ENERGY_LEGEND_ROW_GAP 8
#define GUI_VIEW_ENERGY_CHART_HORIZONTAL_INSET 36
#define GUI_VIEW_ENERGY_CHART_Y_AXIS_DRAW_SIZE 52
#define GUI_VIEW_ENERGY_CHART_Y_AXIS_MIN 0
#define GUI_VIEW_ENERGY_CHART_Y_AXIS_MAX 1000
#define GUI_VIEW_ENERGY_CHART_VALUE_SCALE 1000
#define GUI_VIEW_ENERGY_SPOT_PRICE_RANGE_STEP 500
#define GUI_VIEW_ENERGY_TIME_LABEL_STEP_HOURS 6
#define GUI_VIEW_ENERGY_TIME_LABEL_SLOT_WIDTH 72
#define GUI_VIEW_ENERGY_TIME_LABEL_X_OFFSET 1
#define GUI_VIEW_ENERGY_CHART_LINE_WIDTH 3

typedef enum {
    GUI_VIEW_ENERGY_ACTION_IDLE = 0,
    GUI_VIEW_ENERGY_ACTION_BUY,
    GUI_VIEW_ENERGY_ACTION_SOLAR,
    GUI_VIEW_ENERGY_ACTION_CHARGE,
    GUI_VIEW_ENERGY_ACTION_SELL,
} gui_view_energy_action_t;

static const char *gui_view_energy_spot_area_label(gui_spot_price_area_t area)
{
    switch (area) {
        case GUI_SPOT_PRICE_AREA_SE1:
            return "SE1";
        case GUI_SPOT_PRICE_AREA_SE2:
            return "SE2";
        case GUI_SPOT_PRICE_AREA_SE3:
            return "SE3";
        case GUI_SPOT_PRICE_AREA_SE4:
            return "SE4";
        default:
            return "SE3";
    }
}

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
        lv_snprintf(label, label_len, "24");
        return;
    }

    display_hour =
        (uint8_t)((normalized_start_hour + offset_hours) % GUI_ENERGY_PLAN_POINT_COUNT);
    lv_snprintf(label, label_len, "%02u", (unsigned int)display_hour);
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

static gui_view_energy_action_t gui_view_energy_dominant_action(
    const gui_energy_plan_t *energy_plan,
    uint8_t point_index,
    uint16_t *value_out)
{
    gui_view_energy_action_t action = GUI_VIEW_ENERGY_ACTION_IDLE;
    uint16_t best_value = 0U;
    uint16_t value;

    if ((energy_plan == NULL) || (point_index >= GUI_ENERGY_PLAN_POINT_COUNT)) {
        if (value_out != NULL) {
            *value_out = 0U;
        }
        return GUI_VIEW_ENERGY_ACTION_IDLE;
    }

    value = energy_plan->use_solar_directly[point_index];
    if (value > best_value) {
        best_value = value;
        action = GUI_VIEW_ENERGY_ACTION_SOLAR;
    }

    value = energy_plan->charge_battery[point_index];
    if (value > best_value) {
        best_value = value;
        action = GUI_VIEW_ENERGY_ACTION_CHARGE;
    }

    value = energy_plan->sell_excess[point_index];
    if (value > best_value) {
        best_value = value;
        action = GUI_VIEW_ENERGY_ACTION_SELL;
    }

    value = energy_plan->buy_electricity[point_index];
    if (value > best_value) {
        best_value = value;
        action = GUI_VIEW_ENERGY_ACTION_BUY;
    }

    if (value_out != NULL) {
        *value_out = best_value;
    }

    return action;
}

static const char *gui_view_energy_action_symbol(gui_view_energy_action_t action)
{
    switch (action) {
        case GUI_VIEW_ENERGY_ACTION_BUY:
            return LV_SYMBOL_DOWNLOAD;
        case GUI_VIEW_ENERGY_ACTION_SOLAR:
            return LV_SYMBOL_CHARGE;
        case GUI_VIEW_ENERGY_ACTION_CHARGE:
            return LV_SYMBOL_BATTERY_3;
        case GUI_VIEW_ENERGY_ACTION_SELL:
            return LV_SYMBOL_UPLOAD;
        case GUI_VIEW_ENERGY_ACTION_IDLE:
        default:
            return LV_SYMBOL_POWER;
    }
}

static const char *gui_view_energy_action_title(gui_view_energy_action_t action)
{
    switch (action) {
        case GUI_VIEW_ENERGY_ACTION_BUY:
            return "Buy electricity";
        case GUI_VIEW_ENERGY_ACTION_SOLAR:
            return "Use solar";
        case GUI_VIEW_ENERGY_ACTION_CHARGE:
            return "Charge battery";
        case GUI_VIEW_ENERGY_ACTION_SELL:
            return "Sell excess";
        case GUI_VIEW_ENERGY_ACTION_IDLE:
        default:
            return "Plan standby";
    }
}

static lv_color_t gui_view_energy_action_color(const gui_view_t *view,
                                               gui_view_energy_action_t action)
{
    const gui_theme_def_t *theme = (view != NULL) ? gui_theme_get(view->current_theme) : NULL;

    switch (action) {
        case GUI_VIEW_ENERGY_ACTION_BUY:
            return lv_color_hex((theme != NULL) ? theme->energy_buy_color : 0x1D4ED8);
        case GUI_VIEW_ENERGY_ACTION_SOLAR:
            return lv_color_hex((theme != NULL) ? theme->energy_solar_color : 0xF59E0B);
        case GUI_VIEW_ENERGY_ACTION_CHARGE:
            return lv_color_hex((theme != NULL) ? theme->energy_charge_color : 0x10B981);
        case GUI_VIEW_ENERGY_ACTION_SELL:
            return lv_color_hex((theme != NULL) ? theme->energy_sell_color : 0xEF4444);
        case GUI_VIEW_ENERGY_ACTION_IDLE:
        default:
            return lv_color_hex((theme != NULL) ? theme->panel_border : 0xD9E3F1);
    }
}

static lv_color_t gui_view_energy_action_border_color(const gui_view_t *view)
{
    const gui_theme_def_t *theme = (view != NULL) ? gui_theme_get(view->current_theme) : NULL;

    return lv_color_hex((theme != NULL) ? theme->title_text : 0x10213D);
}

static void gui_view_energy_format_level(char *text, size_t text_len, uint16_t value)
{
    uint32_t tenths;

    if ((text == NULL) || (text_len == 0U)) {
        return;
    }

    tenths = (((uint32_t)value * 10U) + (GUI_VIEW_ENERGY_CHART_VALUE_SCALE / 2U)) /
             GUI_VIEW_ENERGY_CHART_VALUE_SCALE;
    lv_snprintf(text, text_len, "Level %u.%u", (unsigned int)(tenths / 10U),
                (unsigned int)(tenths % 10U));
}

static void gui_view_update_energy_action_card(gui_view_t *view,
                                               const gui_energy_plan_t *energy_plan)
{
    gui_view_energy_action_t action;
    lv_color_t action_color;
    uint16_t action_value;
    char eyebrow_text[20];
    char value_text[20];

    if ((view == NULL) || (energy_plan == NULL)) {
        return;
    }

    action = gui_view_energy_dominant_action(energy_plan, 0U, &action_value);
    action_color = gui_view_energy_action_color(view, action);

    // lv_snprintf(eyebrow_text, sizeof(eyebrow_text), "Now / %02u h",
    //             (unsigned int)(energy_plan->start_hour % GUI_ENERGY_PLAN_POINT_COUNT));
    lv_snprintf(eyebrow_text, sizeof(eyebrow_text), "Now");
    gui_view_energy_format_level(value_text, sizeof(value_text), action_value);

    gui_view_set_label_text_if_changed(view->energy_action_icon,
                                       gui_view_energy_action_symbol(action));
    gui_view_set_label_text_if_changed(view->energy_action_eyebrow, eyebrow_text);
    gui_view_set_label_text_if_changed(view->energy_action_title,
                                       gui_view_energy_action_title(action));
    gui_view_set_label_text_if_changed(view->energy_action_value, value_text);

    if (view->energy_action_icon != NULL) {
        lv_obj_set_style_text_color(view->energy_action_icon, action_color, 0);
    }
    if (view->energy_action_value != NULL) {
        lv_obj_set_style_text_color(view->energy_action_value, action_color, 0);
    }
}

static void gui_view_update_energy_action_strip(gui_view_t *view,
                                                const gui_energy_plan_t *energy_plan)
{
    uint8_t point_index;

    if ((view == NULL) || (energy_plan == NULL)) {
        return;
    }

    for (point_index = 0; point_index < GUI_ENERGY_PLAN_POINT_COUNT; point_index++) {
        lv_obj_t *segment = view->energy_action_segments[point_index];
        gui_view_energy_action_t action;
        lv_color_t action_color;

        if (segment == NULL) {
            continue;
        }

        action = gui_view_energy_dominant_action(energy_plan, point_index, NULL);
        action_color = gui_view_energy_action_color(view, action);

        lv_obj_set_style_bg_color(segment, action_color, 0);
        lv_obj_set_style_bg_opa(segment, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(segment, gui_view_energy_action_border_color(view), 0);
        lv_obj_set_style_border_opa(segment, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(segment, 2, 0);
    }
}

static void gui_view_update_energy_action_overview(gui_view_t *view,
                                                   const gui_energy_plan_t *energy_plan)
{
    gui_view_update_energy_action_card(view, energy_plan);
    gui_view_update_energy_action_strip(view, energy_plan);
}

static void gui_view_update_spot_price_overview(gui_view_t *view,
                                                const gui_spot_price_state_t *spot_price)
{
    char eyebrow_text[20];
    lv_color_t accent_color;
    const gui_theme_def_t *theme;

    if ((view == NULL) || (spot_price == NULL)) {
        return;
    }

    theme = gui_theme_get(view->current_theme);
    accent_color = lv_color_hex((theme != NULL) ? theme->accent_color : 0x1D4ED8);

    lv_snprintf(eyebrow_text, sizeof(eyebrow_text), "Now / %s",
                gui_view_energy_spot_area_label(spot_price->area));

    gui_view_set_label_text_if_changed(view->energy_action_icon, LV_SYMBOL_POWER);
    gui_view_set_label_text_if_changed(view->energy_action_eyebrow, eyebrow_text);
    gui_view_set_label_text_if_changed(view->energy_action_title, "Spot price");
    gui_view_set_label_text_if_changed(view->energy_action_value,
                                       spot_price->current_price_text);
    gui_view_set_label_text_if_changed(view->spot_price_summary_label,
                                       spot_price->summary);

    if (view->energy_action_value != NULL) {
        lv_obj_set_style_text_color(view->energy_action_value, accent_color, 0);
    }
}

static void gui_view_set_legend_item_hidden(lv_obj_t *dot, bool hidden)
{
    lv_obj_t *item = (dot != NULL) ? lv_obj_get_parent(dot) : NULL;

    if (item == NULL) {
        return;
    }

    if (hidden) {
        lv_obj_add_flag(item, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(item, LV_OBJ_FLAG_HIDDEN);
    }
}

static void gui_view_update_energy_legend_mode(gui_view_t *view,
                                               gui_energy_panel_mode_t mode)
{
    if (view == NULL) {
        return;
    }

    if (mode == GUI_ENERGY_PANEL_MODE_SPOT_PRICE) {
        gui_view_set_legend_item_hidden(view->energy_legend_dots[0], false);
        gui_view_set_legend_item_hidden(view->energy_legend_dots[1], true);
        gui_view_set_legend_item_hidden(view->energy_legend_dots[2], true);
        gui_view_set_legend_item_hidden(view->energy_legend_dots[3], true);
        gui_view_set_label_text_if_changed(view->energy_legend_labels[0], "Spot price");
        if (view->spot_price_summary_label != NULL) {
            lv_obj_clear_flag(view->spot_price_summary_label, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }

    gui_view_set_legend_item_hidden(view->energy_legend_dots[0], false);
    gui_view_set_legend_item_hidden(view->energy_legend_dots[1], false);
    gui_view_set_legend_item_hidden(view->energy_legend_dots[2], false);
    gui_view_set_legend_item_hidden(view->energy_legend_dots[3], false);
    gui_view_set_label_text_if_changed(view->energy_legend_labels[0], "Buy electricity");
    gui_view_set_label_text_if_changed(view->energy_legend_labels[1], "Use solar");
    gui_view_set_label_text_if_changed(view->energy_legend_labels[2], "Charge battery");
    gui_view_set_label_text_if_changed(view->energy_legend_labels[3], "Sell excess");
    if (view->spot_price_summary_label != NULL) {
        lv_obj_add_flag(view->spot_price_summary_label, LV_OBJ_FLAG_HIDDEN);
    }
}

static void gui_view_energy_chart_draw_event_cb(lv_event_t *event)
{
    lv_obj_draw_part_dsc_t *draw_part;
    int32_t tick_value;
    int32_t abs_value;

    if (event == NULL) {
        return;
    }

    draw_part = lv_event_get_draw_part_dsc(event);
    if (!lv_obj_draw_part_check_type(draw_part, &lv_chart_class,
                                     LV_CHART_DRAW_PART_TICK_LABEL) ||
        (draw_part->id != LV_CHART_AXIS_PRIMARY_Y) || (draw_part->text == NULL)) {
        return;
    }

    tick_value = draw_part->value;
    abs_value = (tick_value < 0) ? -tick_value : tick_value;
    lv_snprintf(draw_part->text, draw_part->text_length, "%s%d.%d",
                (tick_value < 0) ? "-" : "",
                (int)(abs_value / GUI_VIEW_ENERGY_CHART_VALUE_SCALE),
                (int)((abs_value % GUI_VIEW_ENERGY_CHART_VALUE_SCALE) /
                      (GUI_VIEW_ENERGY_CHART_VALUE_SCALE / 10)));
}

static void gui_view_style_energy_mode_button(gui_view_t *view, lv_obj_t *button,
                                              bool active)
{
    const gui_theme_def_t *theme;
    lv_color_t bg_color;
    lv_color_t text_color;
    lv_color_t border_color;

    if ((view == NULL) || (button == NULL)) {
        return;
    }

    theme = gui_theme_get(view->current_theme);
    if (theme == NULL) {
        return;
    }

    bg_color = lv_color_hex(active ? theme->action_primary_bg
                                   : theme->action_secondary_bg);
    text_color = lv_color_hex(active ? theme->action_primary_text
                                     : theme->action_secondary_text);
    border_color = lv_color_hex(active ? theme->action_primary_border
                                       : theme->action_secondary_border);

    lv_obj_set_style_bg_color(button, bg_color, 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(button, border_color, 0);
    lv_obj_set_style_border_width(button, active ? 0 : 1, 0);
    lv_obj_set_style_text_color(button, text_color, 0);
    lv_obj_set_style_text_font(button, theme->body_font, 0);
}

static void gui_view_update_energy_mode_buttons(gui_view_t *view,
                                                gui_energy_panel_mode_t mode)
{
    if (view == NULL) {
        return;
    }

    gui_view_style_energy_mode_button(view, view->energy_leop_mode_button,
                                      mode == GUI_ENERGY_PANEL_MODE_LEOP);
    gui_view_style_energy_mode_button(view, view->energy_spot_mode_button,
                                      mode == GUI_ENERGY_PANEL_MODE_SPOT_PRICE);
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

static void gui_view_layout_energy_legend_items(gui_view_t *view, lv_coord_t legend_width)
{
    lv_coord_t item_width;
    uint8_t item_index;

    if ((view == NULL) || (legend_width <= GUI_VIEW_ENERGY_LEGEND_LABEL_X_OFFSET)) {
        return;
    }

    item_width = legend_width;
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
    lv_obj_t *overview_row;
    lv_obj_t *legend_container;
    lv_obj_t *action_strip_row;
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

    overview_row = lv_obj_get_child(view->energy_plan_panel, 1);
    action_strip_row = lv_obj_get_child(view->energy_plan_panel, 2);
    time_row = lv_obj_get_child(view->energy_plan_panel, 4);
    legend_container = (overview_row != NULL) ? lv_obj_get_child(overview_row, 1) : NULL;

    panel_content_width = lv_obj_get_content_width(view->energy_plan_panel);
    chart_width = panel_content_width - (GUI_VIEW_ENERGY_CHART_HORIZONTAL_INSET * 2);
    if (chart_width <= 0) {
        return;
    }

    if (overview_row != NULL) {
        lv_obj_set_width(overview_row, chart_width);
    }
    if (view->energy_mode_row != NULL) {
        lv_obj_set_width(view->energy_mode_row, chart_width);
    }
    if (legend_container != NULL) {
        lv_obj_set_width(legend_container, GUI_VIEW_ENERGY_LEGEND_ITEM_WIDTH);
        gui_view_layout_energy_legend_items(view, GUI_VIEW_ENERGY_LEGEND_ITEM_WIDTH);
    }
    if (action_strip_row != NULL) {
        lv_obj_set_width(action_strip_row, chart_width);
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

static bool gui_view_spot_price_changed(gui_view_t *view,
                                        const gui_spot_price_state_t *spot_price)
{
    if ((view == NULL) || (spot_price == NULL)) {
        return false;
    }

    return !view->has_last_spot_price ||
           (memcmp(&view->last_spot_price, spot_price, sizeof(*spot_price)) != 0);
}

static void gui_view_spot_price_chart_range(const gui_spot_price_state_t *spot_price,
                                            int16_t *min_out, int16_t *max_out)
{
    int16_t min_value = 0;
    int16_t max_value = GUI_VIEW_ENERGY_CHART_Y_AXIS_MAX;
    uint8_t point_index;

    if ((min_out == NULL) || (max_out == NULL)) {
        return;
    }

    if (spot_price != NULL) {
        for (point_index = 0; point_index < GUI_SPOT_PRICE_POINT_COUNT; point_index++) {
            if (!spot_price->valid_points[point_index]) {
                continue;
            }

            if (spot_price->price_milli_kr[point_index] > max_value) {
                max_value = spot_price->price_milli_kr[point_index];
            }
            if (spot_price->price_milli_kr[point_index] < min_value) {
                min_value = spot_price->price_milli_kr[point_index];
            }
        }
    }

    max_value = (int16_t)((((int32_t)max_value + GUI_VIEW_ENERGY_SPOT_PRICE_RANGE_STEP - 1) /
                           GUI_VIEW_ENERGY_SPOT_PRICE_RANGE_STEP) *
                          GUI_VIEW_ENERGY_SPOT_PRICE_RANGE_STEP);
    if (min_value < 0) {
        min_value = (int16_t)((((int32_t)min_value - GUI_VIEW_ENERGY_SPOT_PRICE_RANGE_STEP + 1) /
                               GUI_VIEW_ENERGY_SPOT_PRICE_RANGE_STEP) *
                              GUI_VIEW_ENERGY_SPOT_PRICE_RANGE_STEP);
    }

    *min_out = min_value;
    *max_out = max_value;
}

static void gui_view_apply_empty_series(lv_obj_t *chart, lv_chart_series_t *series)
{
    uint16_t point_index;

    if ((chart == NULL) || (series == NULL)) {
        return;
    }

    for (point_index = 0; point_index < GUI_ENERGY_PLAN_POINT_COUNT; point_index++) {
        lv_chart_set_value_by_id(chart, series, point_index, LV_CHART_POINT_NONE);
    }
}

static void gui_view_apply_spot_price_series(lv_obj_t *chart, lv_chart_series_t *series,
                                             const gui_spot_price_state_t *spot_price)
{
    uint16_t point_index;

    if ((chart == NULL) || (series == NULL) || (spot_price == NULL)) {
        return;
    }

    for (point_index = 0; point_index < GUI_SPOT_PRICE_POINT_COUNT; point_index++) {
        lv_coord_t value = spot_price->valid_points[point_index]
                               ? (lv_coord_t)spot_price->price_milli_kr[point_index]
                               : LV_CHART_POINT_NONE;

        lv_chart_set_value_by_id(chart, series, point_index, value);
    }
}

static lv_obj_t *gui_view_create_energy_mode_button(lv_obj_t *parent,
                                                    const char *text,
                                                    lv_event_cb_t event_cb,
                                                    void *event_user_data)
{
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_t *label;

    lv_obj_set_size(button, GUI_VIEW_ENERGY_MODE_BUTTON_WIDTH,
                    GUI_VIEW_ENERGY_MODE_BUTTON_HEIGHT);
    lv_obj_set_style_radius(button, 14, 0);
    lv_obj_set_style_shadow_width(button, 0, 0);
    if (event_cb != NULL) {
        lv_obj_add_event_cb(button, event_cb, LV_EVENT_CLICKED, event_user_data);
    }

    label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return button;
}

void gui_view_init_energy_panel(gui_view_t *view, lv_obj_t *content,
                                lv_event_cb_t settings_event_cb, void *event_user_data)
{
    lv_obj_t *overview_row;
    lv_obj_t *legend_container;
    lv_obj_t *action_strip_row;
    lv_obj_t *action_text_column;
    lv_obj_t *time_row;
    lv_obj_t *time_label;
    uint8_t label_index;

    if ((view == NULL) || (content == NULL)) {
        return;
    }

    view->energy_mode_row = NULL;
    view->energy_leop_mode_button = NULL;
    view->energy_spot_mode_button = NULL;
    view->spot_price_summary_label = NULL;

    view->energy_plan_panel = lv_obj_create(content);
    lv_obj_set_size(view->energy_plan_panel, 746, 378);
    lv_obj_align(view->energy_plan_panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(view->energy_plan_panel, 26, 0);
    lv_obj_set_style_bg_color(view->energy_plan_panel, lv_color_hex(0xF8FBFF), 0);
    lv_obj_set_style_bg_opa(view->energy_plan_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(view->energy_plan_panel, 1, 0);
    lv_obj_set_style_border_color(view->energy_plan_panel, lv_color_hex(0xD9E3F1), 0);
    lv_obj_set_style_pad_all(view->energy_plan_panel, 24, 0);
    lv_obj_set_style_pad_row(view->energy_plan_panel, 14, 0);
    lv_obj_clear_flag(view->energy_plan_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(view->energy_plan_panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(view->energy_plan_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(view->energy_plan_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    view->energy_mode_row = lv_obj_create(view->energy_plan_panel);
    lv_obj_set_size(view->energy_mode_row, LV_PCT(100), GUI_VIEW_ENERGY_MODE_ROW_HEIGHT);
    lv_obj_set_style_bg_opa(view->energy_mode_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(view->energy_mode_row, 0, 0);
    lv_obj_set_style_shadow_width(view->energy_mode_row, 0, 0);
    lv_obj_set_style_pad_all(view->energy_mode_row, 0, 0);
    lv_obj_set_style_pad_column(view->energy_mode_row, 10, 0);
    lv_obj_clear_flag(view->energy_mode_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(view->energy_mode_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(view->energy_mode_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(view->energy_mode_row, LV_FLEX_ALIGN_END,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    view->energy_leop_mode_button = gui_view_create_energy_mode_button(
        view->energy_mode_row, "LEOP", settings_event_cb, event_user_data);
    view->energy_spot_mode_button = gui_view_create_energy_mode_button(
        view->energy_mode_row, "Spot price", settings_event_cb, event_user_data);

    overview_row = lv_obj_create(view->energy_plan_panel);
    lv_obj_set_size(overview_row, LV_PCT(100), GUI_VIEW_ENERGY_OVERVIEW_HEIGHT);
    lv_obj_set_style_bg_opa(overview_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(overview_row, 0, 0);
    lv_obj_set_style_shadow_width(overview_row, 0, 0);
    lv_obj_set_style_pad_all(overview_row, 0, 0);
    lv_obj_set_style_pad_column(overview_row, 20, 0);
    lv_obj_clear_flag(overview_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(overview_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(overview_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(overview_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);

    view->energy_action_card = lv_obj_create(overview_row);
    lv_obj_set_size(view->energy_action_card, 0, LV_PCT(100));
    lv_obj_set_style_radius(view->energy_action_card, 22, 0);
    lv_obj_set_style_bg_color(view->energy_action_card, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(view->energy_action_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(view->energy_action_card, 1, 0);
    lv_obj_set_style_border_color(view->energy_action_card, lv_color_hex(0xD9E3F1), 0);
    lv_obj_set_style_shadow_width(view->energy_action_card, 0, 0);
    lv_obj_set_style_pad_all(view->energy_action_card, 16, 0);
    lv_obj_set_style_pad_column(view->energy_action_card, 14, 0);
    lv_obj_clear_flag(view->energy_action_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(view->energy_action_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(view->energy_action_card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(view->energy_action_card, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_flex_grow(view->energy_action_card, 3);

    view->energy_action_icon = lv_label_create(view->energy_action_card);
    lv_label_set_text(view->energy_action_icon, LV_SYMBOL_POWER);
    lv_label_set_long_mode(view->energy_action_icon, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(view->energy_action_icon, 44);
    lv_obj_set_style_text_align(view->energy_action_icon, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(view->energy_action_icon, lv_color_hex(0x1D4ED8), 0);
    lv_obj_set_style_text_font(view->energy_action_icon, &lv_font_montserrat_24, 0);
    lv_obj_add_flag(view->energy_action_icon, LV_OBJ_FLAG_HIDDEN);

    action_text_column = lv_obj_create(view->energy_action_card);
    lv_obj_set_size(action_text_column, 0, LV_PCT(100));
    lv_obj_set_style_bg_opa(action_text_column, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(action_text_column, 0, 0);
    lv_obj_set_style_shadow_width(action_text_column, 0, 0);
    lv_obj_set_style_pad_all(action_text_column, 0, 0);
    lv_obj_set_style_pad_row(action_text_column, 2, 0);
    lv_obj_clear_flag(action_text_column, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(action_text_column, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(action_text_column, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(action_text_column, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_flex_grow(action_text_column, 1);

    view->energy_action_eyebrow = lv_label_create(action_text_column);
    lv_label_set_text(view->energy_action_eyebrow, "Now / 00 h");
    lv_label_set_long_mode(view->energy_action_eyebrow, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(view->energy_action_eyebrow, LV_PCT(100));
    lv_obj_set_style_text_color(view->energy_action_eyebrow, lv_color_hex(0x607089), 0);

    view->energy_action_title = lv_label_create(action_text_column);
    lv_label_set_text(view->energy_action_title, "Plan standby");
    lv_label_set_long_mode(view->energy_action_title, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(view->energy_action_title, LV_PCT(100));
    lv_obj_set_style_text_color(view->energy_action_title, lv_color_hex(0x10213D), 0);
    lv_obj_set_style_text_font(view->energy_action_title, &lv_font_montserrat_24, 0);

    view->energy_action_value = lv_label_create(action_text_column);
    lv_label_set_text(view->energy_action_value, "Level 0.0");
    lv_label_set_long_mode(view->energy_action_value, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(view->energy_action_value, LV_PCT(100));
    lv_obj_set_style_text_color(view->energy_action_value, lv_color_hex(0x1D4ED8), 0);

    legend_container = lv_obj_create(overview_row);
    lv_obj_set_size(legend_container, GUI_VIEW_ENERGY_LEGEND_ITEM_WIDTH, LV_PCT(100));
    lv_obj_set_style_bg_opa(legend_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(legend_container, 0, 0);
    lv_obj_set_style_shadow_width(legend_container, 0, 0);
    lv_obj_set_style_pad_all(legend_container, 0, 0);
    lv_obj_set_style_pad_column(legend_container, GUI_VIEW_ENERGY_LEGEND_COLUMN_GAP, 0);
    lv_obj_set_style_pad_row(legend_container, GUI_VIEW_ENERGY_LEGEND_ROW_GAP, 0);
    lv_obj_clear_flag(legend_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(legend_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(legend_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(legend_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_flex_grow(legend_container, 2);

    view->energy_legend_dots[0] = gui_view_create_legend_item(
        legend_container, 0, 0, GUI_VIEW_ENERGY_LEGEND_ITEM_WIDTH, lv_color_hex(0x1D4ED8),
        "Buy electricity", &view->energy_legend_labels[0]);
    view->energy_legend_dots[1] = gui_view_create_legend_item(
        legend_container, 0, 0, GUI_VIEW_ENERGY_LEGEND_ITEM_WIDTH, lv_color_hex(0xF59E0B),
        "Use solar", &view->energy_legend_labels[1]);
    view->energy_legend_dots[2] = gui_view_create_legend_item(
        legend_container, 0, 0, GUI_VIEW_ENERGY_LEGEND_ITEM_WIDTH, lv_color_hex(0x10B981),
        "Charge battery", &view->energy_legend_labels[2]);
    view->energy_legend_dots[3] = gui_view_create_legend_item(
        legend_container, 0, 0, GUI_VIEW_ENERGY_LEGEND_ITEM_WIDTH, lv_color_hex(0xEF4444),
        "Sell excess", &view->energy_legend_labels[3]);

    view->spot_price_summary_label = lv_label_create(legend_container);
    lv_label_set_text(view->spot_price_summary_label,
                      "Spot price excluding VAT and taxes.");
    lv_obj_set_width(view->spot_price_summary_label, GUI_VIEW_ENERGY_LEGEND_ITEM_WIDTH);
    lv_label_set_long_mode(view->spot_price_summary_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(view->spot_price_summary_label, lv_color_hex(0x607089), 0);
    lv_obj_add_flag(view->spot_price_summary_label, LV_OBJ_FLAG_HIDDEN);

    action_strip_row = lv_obj_create(view->energy_plan_panel);
    lv_obj_set_size(action_strip_row, LV_PCT(100), GUI_VIEW_ENERGY_ACTION_STRIP_HEIGHT);
    lv_obj_set_style_bg_opa(action_strip_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(action_strip_row, 0, 0);
    lv_obj_set_style_shadow_width(action_strip_row, 0, 0);
    lv_obj_set_style_pad_all(action_strip_row, 0, 0);
    lv_obj_set_style_pad_column(action_strip_row, GUI_VIEW_ENERGY_ACTION_SEGMENT_GAP, 0);
    lv_obj_clear_flag(action_strip_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(action_strip_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(action_strip_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(action_strip_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_bottom(action_strip_row, -8, 0);
    lv_obj_add_flag(action_strip_row, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_add_flag(action_strip_row, LV_OBJ_FLAG_HIDDEN);

    for (label_index = 0; label_index < GUI_ENERGY_PLAN_POINT_COUNT; label_index++) {
        lv_obj_t *segment = lv_obj_create(action_strip_row);

        view->energy_action_segments[label_index] = segment;
        lv_obj_set_size(segment, 0, GUI_VIEW_ENERGY_ACTION_SEGMENT_HEIGHT);
        lv_obj_set_flex_grow(segment, 1);
        lv_obj_set_style_radius(segment, 7, 0);
        lv_obj_set_style_bg_color(segment, lv_color_hex(0xD9E3F1), 0);
        lv_obj_set_style_bg_opa(segment, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(segment, 2, 0);
        lv_obj_set_style_border_color(segment, lv_color_hex(0x10213D), 0);
        lv_obj_set_style_border_opa(segment, LV_OPA_TRANSP, 0);
        lv_obj_set_style_pad_all(segment, 0, 0);
        lv_obj_clear_flag(segment, LV_OBJ_FLAG_SCROLLABLE);
    }

    view->energy_plan_chart = lv_chart_create(view->energy_plan_panel);
    lv_obj_set_size(view->energy_plan_chart, LV_PCT(100), 0);
    lv_obj_set_style_bg_color(view->energy_plan_chart, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(view->energy_plan_chart, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(view->energy_plan_chart, 1, 0);
    lv_obj_set_style_border_color(view->energy_plan_chart, lv_color_hex(0xD8E4F0), 0);
    lv_obj_set_style_pad_all(view->energy_plan_chart, 12, 0);
    lv_obj_set_style_line_width(view->energy_plan_chart, GUI_VIEW_ENERGY_CHART_LINE_WIDTH, LV_PART_ITEMS);
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
    gui_view_update_energy_mode_buttons(view, GUI_ENERGY_PANEL_MODE_LEOP);

    gui_view_layout_energy_panel(view);
}

void gui_view_apply_energy_panel(gui_view_t *view, const gui_view_model_t *model)
{
    bool mode_changed;

    if ((view == NULL) || (model == NULL)) {
        return;
    }

    mode_changed = !view->has_last_energy_panel_mode ||
                   (view->last_energy_panel_mode != model->energy_panel_mode);

    gui_view_layout_energy_panel(view);
    gui_view_update_energy_mode_buttons(view, model->energy_panel_mode);
    gui_view_update_energy_legend_mode(view, model->energy_panel_mode);

    if (model->energy_panel_mode == GUI_ENERGY_PANEL_MODE_SPOT_PRICE) {
        const gui_theme_def_t *theme = gui_theme_get(view->current_theme);
        lv_color_t spot_color = lv_color_hex((theme != NULL) ? theme->accent_color
                                                             : 0x1D4ED8);
        int16_t chart_min;
        int16_t chart_max;

        gui_view_update_energy_time_labels(view, model->spot_price.start_hour);
        gui_view_update_spot_price_overview(view, &model->spot_price);
        gui_view_spot_price_chart_range(&model->spot_price, &chart_min, &chart_max);
        lv_chart_set_range(view->energy_plan_chart, LV_CHART_AXIS_PRIMARY_Y,
                           chart_min, chart_max);
        lv_chart_set_series_color(view->energy_plan_chart, view->buy_series, spot_color);
        if (view->energy_legend_dots[0] != NULL) {
            lv_obj_set_style_bg_color(view->energy_legend_dots[0], spot_color, 0);
        }

        if (mode_changed || gui_view_spot_price_changed(view, &model->spot_price)) {
            gui_view_apply_spot_price_series(view->energy_plan_chart, view->buy_series,
                                             &model->spot_price);
            gui_view_apply_empty_series(view->energy_plan_chart, view->solar_series);
            gui_view_apply_empty_series(view->energy_plan_chart, view->charge_series);
            gui_view_apply_empty_series(view->energy_plan_chart, view->sell_series);
            lv_chart_refresh(view->energy_plan_chart);
            view->last_spot_price = model->spot_price;
            view->has_last_spot_price = true;
        }

        view->last_energy_panel_mode = model->energy_panel_mode;
        view->has_last_energy_panel_mode = true;
        return;
    }

    gui_view_update_energy_time_labels(view, model->energy_plan.start_hour);
    gui_view_update_energy_action_overview(view, &model->energy_plan);
    lv_chart_set_range(view->energy_plan_chart, LV_CHART_AXIS_PRIMARY_Y,
                       GUI_VIEW_ENERGY_CHART_Y_AXIS_MIN,
                       GUI_VIEW_ENERGY_CHART_Y_AXIS_MAX);
    {
        const gui_theme_def_t *theme = gui_theme_get(view->current_theme);

        if (theme != NULL) {
            lv_chart_set_series_color(view->energy_plan_chart, view->buy_series,
                                      lv_color_hex(theme->energy_buy_color));
            lv_chart_set_series_color(view->energy_plan_chart, view->solar_series,
                                      lv_color_hex(theme->energy_solar_color));
            lv_chart_set_series_color(view->energy_plan_chart, view->charge_series,
                                      lv_color_hex(theme->energy_charge_color));
            lv_chart_set_series_color(view->energy_plan_chart, view->sell_series,
                                      lv_color_hex(theme->energy_sell_color));
            if (view->energy_legend_dots[0] != NULL) {
                lv_obj_set_style_bg_color(view->energy_legend_dots[0],
                                          lv_color_hex(theme->energy_buy_color), 0);
            }
        }
    }

    if (!mode_changed && !gui_view_energy_plan_changed(view, &model->energy_plan)) {
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
    view->last_energy_panel_mode = model->energy_panel_mode;
    view->has_last_energy_panel_mode = true;
}
