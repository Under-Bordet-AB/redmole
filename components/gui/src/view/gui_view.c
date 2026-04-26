#include "gui_view.h"

#include <stdio.h>
#include <time.h>

#include "gui_theme_defs.h"
#include "gui_view_common.h"
#include "panels/gui_view_bme280_panel.h"
#include "panels/gui_view_energy_panel.h"
#include "panels/gui_view_forecast_panel.h"
#include "panels/gui_view_settings_panel.h"

static gui_view_theme_t gui_view_effective_theme(gui_view_theme_t base_theme,
                                                 bool night_variant_enabled)
{
    const gui_theme_def_t *def = gui_theme_get(base_theme);

    if (night_variant_enabled && (def != NULL) && def->has_night_variant) {
        return def->night_variant;
    }

    return base_theme;
}

lv_color_t gui_view_wifi_status_color(gui_view_theme_t theme, gui_wifi_state_t state)
{
    const gui_theme_def_t *def = gui_theme_get(theme);

    switch (state) {
        case GUI_WIFI_STATE_CONNECTED:
            return (def != NULL) ? lv_color_hex(def->wifi_connected_color)
                                 : lv_color_hex(0x1D4ED8);
        case GUI_WIFI_STATE_CONNECTING:
            return lv_color_hex(0xF59E0B);
        case GUI_WIFI_STATE_SCANNED:
            return lv_color_hex(0xF59E0B);
        case GUI_WIFI_STATE_FAILED:
            return lv_color_hex(0xEF4444);
        case GUI_WIFI_STATE_IDLE:
        default:
            return (def != NULL) ? lv_color_hex(def->wifi_idle_color)
                                 : lv_color_hex(0x4A5C78);
    }
}

lv_color_t gui_view_bluetooth_status_color(gui_view_theme_t theme,
                                           gui_bluetooth_state_t state)
{
    const gui_theme_def_t *def = gui_theme_get(theme);

    switch (state) {
        case GUI_BLUETOOTH_STATE_CONNECTED:
            return (def != NULL) ? lv_color_hex(def->wifi_connected_color)
                                 : lv_color_hex(0x1D4ED8);
        case GUI_BLUETOOTH_STATE_CONNECTING:
            return lv_color_hex(0xF59E0B);
        case GUI_BLUETOOTH_STATE_UNAVAILABLE:
            return lv_color_hex(0x4A5C78);
        case GUI_BLUETOOTH_STATE_IDLE:
        default:
            return (def != NULL) ? lv_color_hex(def->wifi_idle_color)
                                 : lv_color_hex(0x4A5C78);
    }
}

lv_color_t gui_view_sd_card_status_color(gui_view_theme_t theme,
                                         gui_sd_card_state_t state)
{
    const gui_theme_def_t *def = gui_theme_get(theme);

    switch (state) {
        case GUI_SD_CARD_STATE_CONNECTED:
            return (def != NULL) ? lv_color_hex(def->wifi_connected_color)
                                 : lv_color_hex(0x1D4ED8);
        case GUI_SD_CARD_STATE_UNAVAILABLE:
        case GUI_SD_CARD_STATE_IDLE:
        default:
            return (def != NULL) ? lv_color_hex(def->wifi_idle_color)
                                 : lv_color_hex(0x4A5C78);
    }
}

static void gui_view_update_sidebar_clock_impl(gui_view_t *view)
{
    time_t now;
    struct tm time_info;
    char clock_text[16];
    char date_text[32];

    if (view == NULL) {
        return;
    }

    now = time(NULL);
    if ((now < 1700000000) || (localtime_r(&now, &time_info) == NULL)) {
        gui_view_set_label_text_if_changed(view->sidebar_clock_label, "--:--");
        gui_view_set_label_text_if_changed(view->sidebar_date_label, "--- -- ---");
        return;
    }

    strftime(clock_text, sizeof(clock_text), "%H:%M", &time_info);
    strftime(date_text, sizeof(date_text), "%a %d %b", &time_info);
    gui_view_set_label_text_if_changed(view->sidebar_clock_label, clock_text);
    gui_view_set_label_text_if_changed(view->sidebar_date_label, date_text);
}

static void gui_view_apply_header(gui_view_t *view, const gui_view_model_t *model)
{
    if (model->active_panel == GUI_PANEL_BME280) {
        gui_view_set_label_text_if_changed(view->header_title, "BME280 Overview");
        gui_view_set_label_text_if_changed(
            view->header_subtitle,
            "Temperature, humidity, and pressure from the sensor data pipeline.");
        lv_obj_clear_flag(view->bme280_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(view->energy_plan_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(view->forecast_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(view->settings_panel, LV_OBJ_FLAG_HIDDEN);
    } else if (model->active_panel == GUI_PANEL_ENERGY_PLAN) {
        gui_view_set_label_text_if_changed(view->header_title, "LEOP Energy Plan");
        gui_view_set_label_text_if_changed(
            view->header_subtitle,
            "Recommendations for grid, solar, battery, and export decisions.");
        lv_obj_add_flag(view->bme280_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(view->energy_plan_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(view->forecast_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(view->settings_panel, LV_OBJ_FLAG_HIDDEN);
    } else if (model->active_panel == GUI_PANEL_FORECAST) {
        gui_view_set_label_text_if_changed(view->header_title, "Weather Forecast");
        gui_view_set_label_text_if_changed(
            view->header_subtitle,
            "Today, conditions details, and the next few days at a glance.");
        lv_obj_add_flag(view->bme280_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(view->energy_plan_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(view->forecast_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(view->settings_panel, LV_OBJ_FLAG_HIDDEN);
    } else {
        gui_view_set_label_text_if_changed(view->header_title, "");
        gui_view_set_label_text_if_changed(view->header_subtitle, "");
        lv_obj_add_flag(view->bme280_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(view->energy_plan_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(view->forecast_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(view->settings_panel, LV_OBJ_FLAG_HIDDEN);
    }
}

static void gui_view_apply_connectivity_icon_colors(gui_view_t *view,
                                                    const gui_view_model_t *model)
{
    if ((view == NULL) || (model == NULL)) {
        return;
    }

    if (view->sidebar_wifi_label != NULL) {
        lv_obj_set_style_text_color(view->sidebar_wifi_label,
                                    gui_view_wifi_status_color(view->current_theme,
                                                               model->wifi_state),
                                    0);
    }

    if (view->sidebar_bluetooth_label != NULL) {
        lv_obj_set_style_text_color(view->sidebar_bluetooth_label,
                                    gui_view_bluetooth_status_color(view->current_theme,
                                                                    model->bluetooth_state),
                                    0);
    }

    if (view->sidebar_sd_card_label != NULL) {
        lv_obj_set_style_text_color(view->sidebar_sd_card_label,
                                    gui_view_sd_card_status_color(view->current_theme,
                                                                  model->sd_card_state),
                                    0);
    }
}

static void gui_view_style_settings_card(lv_obj_t *card, lv_color_t bg_color,
                                         lv_color_t border_color, lv_color_t title_color,
                                         lv_color_t subtitle_color, gui_view_theme_t theme)
{
    lv_obj_t *title;
    lv_obj_t *subtitle;
    const lv_font_t *body_font;

    if (card == NULL) {
        return;
    }

    body_font = gui_theme_get(theme)->body_font;

    lv_obj_set_style_bg_color(card, bg_color, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, border_color, 0);
    lv_obj_set_style_text_font(card, body_font, 0);

    title = lv_obj_get_child(card, 0);
    subtitle = lv_obj_get_child(card, 1);
    if (title != NULL) {
        lv_obj_set_style_text_color(title, title_color, 0);
        lv_obj_set_style_text_font(title, body_font, 0);
    }
    if (subtitle != NULL) {
        lv_obj_set_style_text_color(subtitle, subtitle_color, 0);
        lv_obj_set_style_text_font(subtitle, body_font, 0);
    }
}

static void gui_view_style_wifi_card(gui_view_t *view, lv_color_t bg_color,
                                     lv_color_t border_color, lv_color_t title_color,
                                     lv_color_t subtitle_color, gui_view_theme_t theme)
{
    const lv_font_t *body_font;

    if ((view == NULL) || (view->wifi_card == NULL)) {
        return;
    }

    body_font = gui_theme_get(theme)->body_font;

    lv_obj_set_style_bg_color(view->wifi_card, bg_color, 0);
    lv_obj_set_style_bg_opa(view->wifi_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(view->wifi_card, 1, 0);
    lv_obj_set_style_border_color(view->wifi_card, border_color, 0);
    lv_obj_set_style_text_font(view->wifi_card, body_font, 0);

    if (view->wifi_header_row != NULL) {
        lv_obj_set_style_text_font(view->wifi_header_row, body_font, 0);
    }
    if (view->wifi_title_label != NULL) {
        lv_obj_set_style_text_color(view->wifi_title_label, title_color, 0);
        lv_obj_set_style_text_font(view->wifi_title_label, body_font, 0);
    }
    if (view->wifi_subtitle_label != NULL) {
        lv_obj_set_style_text_color(view->wifi_subtitle_label, subtitle_color, 0);
        lv_obj_set_style_text_font(view->wifi_subtitle_label, body_font, 0);
    }
}

static void gui_view_style_bme280_cards(gui_view_t *view, lv_color_t card_bg,
                                        lv_color_t card_border, lv_color_t label_color,
                                        lv_color_t value_color, gui_view_theme_t theme)
{
    uint32_t child_count;
    const lv_font_t *body_font;

    if ((view == NULL) || (view->bme280_panel == NULL)) {
        return;
    }

    body_font = gui_theme_get(theme)->body_font;

    child_count = lv_obj_get_child_cnt(view->bme280_panel);
    for (uint32_t index = 0; index < child_count; index++) {
        lv_obj_t *card = lv_obj_get_child(view->bme280_panel, (int32_t)index);
        lv_obj_t *label;
        lv_obj_t *value_label;

        if (card == NULL) {
            continue;
        }

        lv_obj_set_style_bg_color(card, card_bg, 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(card, card_border, 0);
        lv_obj_set_style_text_font(card, body_font, 0);

        label = lv_obj_get_child(card, 0);
        value_label = lv_obj_get_child(card, 1);
        if (label != NULL) {
            lv_obj_set_style_text_color(label, label_color, 0);
            lv_obj_set_style_text_font(label, body_font, 0);
        }
        if (value_label != NULL) {
            lv_obj_set_style_text_color(value_label, value_color, 0);
            lv_obj_set_style_text_font(value_label, body_font, 0);
        }
    }
}

static void gui_view_style_energy_labels(lv_obj_t *parent, lv_color_t text_color,
                                         gui_view_theme_t theme)
{
    uint32_t child_count;
    const lv_font_t *body_font;

    if (parent == NULL) {
        return;
    }

    body_font = gui_theme_get(theme)->body_font;

    child_count = lv_obj_get_child_cnt(parent);
    for (uint32_t index = 0; index < child_count; index++) {
        lv_obj_t *child = lv_obj_get_child(parent, (int32_t)index);

        if (child == NULL) {
            continue;
        }

        if (lv_obj_check_type(child, &lv_label_class)) {
            lv_obj_set_style_text_color(child, text_color, 0);
            lv_obj_set_style_text_font(child, body_font, 0);
        } else {
            uint32_t nested_count = lv_obj_get_child_cnt(child);

            for (uint32_t nested_index = 0; nested_index < nested_count; nested_index++) {
                lv_obj_t *nested_child = lv_obj_get_child(child, (int32_t)nested_index);

                if ((nested_child != NULL) && lv_obj_check_type(nested_child, &lv_label_class)) {
                    lv_obj_set_style_text_color(nested_child, text_color, 0);
                    lv_obj_set_style_text_font(nested_child, body_font, 0);
                }
            }
        }
    }
}

static void gui_view_style_forecast_day_card(lv_obj_t *card, lv_color_t card_bg,
                                             lv_color_t card_border, lv_color_t title_color,
                                             lv_color_t subtitle_color,
                                             lv_color_t accent_color,
                                             gui_view_theme_t theme)
{
    lv_obj_t *day_label;
    lv_obj_t *condition_label;
    lv_obj_t *temp_label;
    const lv_font_t *body_font;
    const lv_font_t *emphasis_font;

    if (card == NULL) {
        return;
    }

    body_font = gui_theme_get(theme)->body_font;
    emphasis_font = gui_theme_get(theme)->emphasis_font;

    lv_obj_set_style_bg_color(card, card_bg, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, card_border, 0);
    lv_obj_set_style_text_font(card, body_font, 0);

    day_label = lv_obj_get_child(card, 0);
    condition_label = lv_obj_get_child(card, 1);
    temp_label = lv_obj_get_child(card, 3);

    if (day_label != NULL) {
        lv_obj_set_style_text_color(day_label, title_color, 0);
        lv_obj_set_style_text_font(day_label, body_font, 0);
    }
    if (condition_label != NULL) {
        lv_obj_set_style_text_color(condition_label, subtitle_color, 0);
        lv_obj_set_style_text_font(condition_label, body_font, 0);
    }
    if (temp_label != NULL) {
        lv_obj_set_style_text_color(temp_label, accent_color, 0);
        lv_obj_set_style_text_font(temp_label, emphasis_font, 0);
    }
}

static void gui_view_style_forecast_card(lv_obj_t *card, lv_color_t card_bg,
                                         lv_color_t card_border, gui_view_theme_t theme)
{
    if (card == NULL) {
        return;
    }

    lv_obj_set_style_bg_color(card, card_bg, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, card_border, 0);
    lv_obj_set_style_text_font(card, gui_theme_get(theme)->body_font, 0);
}

static void gui_view_style_forecast_panel(gui_view_t *view, lv_color_t panel_bg,
                                          lv_color_t panel_border, lv_opa_t panel_bg_opa,
                                          lv_color_t card_bg, lv_color_t card_border,
                                          lv_color_t title_color,
                                          lv_color_t subtitle_color,
                                          lv_color_t accent_color,
                                          gui_view_theme_t theme)
{
    lv_obj_t *top_row;
    lv_obj_t *today_card;
    lv_obj_t *details_card;
    lv_obj_t *days_row;
    lv_obj_t *label;
    uint32_t day_card_count;
    const lv_font_t *body_font;
    const lv_font_t *emphasis_font;

    if ((view == NULL) || (view->forecast_panel == NULL)) {
        return;
    }

    body_font = gui_theme_get(theme)->body_font;
    emphasis_font = gui_theme_get(theme)->emphasis_font;

    lv_obj_set_style_bg_color(view->forecast_panel, panel_bg, 0);
    lv_obj_set_style_bg_opa(view->forecast_panel, panel_bg_opa, 0);
    lv_obj_set_style_border_color(view->forecast_panel, panel_border, 0);
    lv_obj_set_style_text_font(view->forecast_panel, body_font, 0);

    top_row = lv_obj_get_child(view->forecast_panel, 0);
    days_row = lv_obj_get_child(view->forecast_panel, 1);

    today_card = (top_row != NULL) ? lv_obj_get_child(top_row, 0) : NULL;
    details_card = (top_row != NULL) ? lv_obj_get_child(top_row, 1) : NULL;

    gui_view_style_forecast_card(today_card, card_bg, card_border, theme);
    gui_view_style_forecast_card(details_card, card_bg, card_border, theme);

    if (today_card != NULL) {
        label = lv_obj_get_child(today_card, 0);
        if (label != NULL) {
            lv_obj_set_style_text_color(label, subtitle_color, 0);
            lv_obj_set_style_text_font(label, body_font, 0);
        }
        label = lv_obj_get_child(today_card, 1);
        if (label != NULL) {
            lv_obj_set_style_text_color(label, title_color, 0);
            lv_obj_set_style_text_font(label, emphasis_font, 0);
        }
        label = lv_obj_get_child(today_card, 2);
        if (label != NULL) {
            lv_obj_set_style_text_color(label, accent_color, 0);
            lv_obj_set_style_text_font(label, emphasis_font, 0);
        }
        label = lv_obj_get_child(today_card, 3);
        if (label != NULL) {
            lv_obj_set_style_text_color(label, subtitle_color, 0);
            lv_obj_set_style_text_font(label, body_font, 0);
        }
        label = lv_obj_get_child(today_card, 4);
        if (label != NULL) {
            lv_obj_set_style_text_color(label, subtitle_color, 0);
            lv_obj_set_style_text_font(label, body_font, 0);
        }
    }

    if (details_card != NULL) {
        label = lv_obj_get_child(details_card, 0);
        if (label != NULL) {
            lv_obj_set_style_text_color(label, subtitle_color, 0);
            lv_obj_set_style_text_font(label, body_font, 0);
        }

        for (uint32_t index = 1; index < lv_obj_get_child_cnt(details_card); index++) {
            label = lv_obj_get_child(details_card, (int32_t)index);
            if (label != NULL) {
                lv_obj_set_style_text_color(label, title_color, 0);
                lv_obj_set_style_text_font(label, body_font, 0);
            }
        }
    }

    if (days_row != NULL) {
        day_card_count = lv_obj_get_child_cnt(days_row);
        for (uint32_t index = 0; index < day_card_count; index++) {
            gui_view_style_forecast_day_card(lv_obj_get_child(days_row, (int32_t)index),
                                             card_bg, card_border, title_color,
                                             subtitle_color, accent_color, theme);
        }
    }
}

static void gui_view_style_nav_button(lv_obj_t *button, gui_view_theme_t theme, bool is_active)
{
    const gui_theme_def_t *def = gui_theme_get(theme);
    lv_color_t bg_color;
    lv_color_t text_color;
    lv_color_t border_color;

    if (def == NULL) {
        return;
    }

    bg_color = lv_color_hex(is_active ? def->nav_active_bg : def->nav_inactive_bg);
    text_color = lv_color_hex(is_active ? def->nav_active_text : def->nav_inactive_text);
    border_color = lv_color_hex(is_active ? def->nav_active_border : def->nav_inactive_border);

    lv_obj_set_style_bg_color(button, bg_color, 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(button, border_color, 0);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_text_color(button, text_color, 0);
    lv_obj_set_style_text_font(button, def->body_font, 0);
}

static void gui_view_style_action_button(lv_obj_t *button, gui_view_theme_t theme,
                                         bool is_primary)
{
    const gui_theme_def_t *def = gui_theme_get(theme);
    lv_color_t bg_color;
    lv_color_t text_color;
    lv_color_t border_color;

    if ((button == NULL) || (def == NULL)) {
        return;
    }

    bg_color = lv_color_hex(is_primary ? def->action_primary_bg : def->action_secondary_bg);
    text_color = lv_color_hex(is_primary ? def->action_primary_text : def->action_secondary_text);
    border_color = lv_color_hex(is_primary ? def->action_primary_border
                                           : def->action_secondary_border);

    lv_obj_set_style_bg_color(button, bg_color, 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(button, border_color, 0);
    lv_obj_set_style_border_width(button, is_primary ? 0 : 1, 0);
    lv_obj_set_style_text_color(button, text_color, 0);
    lv_obj_set_style_text_font(button, def->body_font, 0);
}

static lv_obj_t *gui_view_create_nav_button(lv_obj_t *parent, lv_coord_t y, const char *label_text,
                                            lv_event_cb_t nav_event_cb, void *nav_user_data)
{
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_set_size(button, 150, 58);
    lv_obj_align(button, LV_ALIGN_TOP_MID, 0, y);
    lv_obj_set_style_radius(button, 18, 0);
    lv_obj_set_style_shadow_width(button, 0, 0);
    lv_obj_add_event_cb(button, nav_event_cb, LV_EVENT_PRESSED, nav_user_data);

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, label_text);
    lv_obj_center(label);

    return button;
}

void gui_view_apply_theme(gui_view_t *view, gui_view_theme_t theme, bool show_background_image,
                          bool night_variant_enabled)
{
    gui_view_theme_t effective_theme;
    lv_color_t screen_bg;
    lv_color_t screen_grad;
    lv_opa_t screen_bg_opa;
    lv_color_t sidebar_bg;
    lv_color_t sidebar_grad;
    lv_color_t sidebar_shadow;
    lv_opa_t sidebar_bg_opa;
    lv_color_t brand_text;
    lv_color_t content_bg;
    lv_color_t content_shadow;
    lv_opa_t content_bg_opa;
    lv_color_t title_text;
    lv_color_t subtitle_text;
    lv_color_t panel_bg;
    lv_color_t panel_border;
    lv_opa_t panel_bg_opa;
    lv_color_t card_bg;
    lv_color_t card_border;
    lv_color_t item_bg;
    lv_color_t item_border;
    lv_color_t muted_text;
    lv_color_t value_text;
    lv_color_t keyboard_bg;
    lv_color_t keyboard_border;
    lv_color_t keyboard_key_bg;
    lv_color_t keyboard_key_text;
    lv_color_t keyboard_special_bg;
    lv_color_t keyboard_special_text;
    lv_color_t keyboard_special_border;
    lv_color_t slider_bg;
    lv_color_t slider_knob_bg;
    lv_color_t dropdown_bg;
    lv_color_t dropdown_border;
    lv_color_t dropdown_selected_bg;
    lv_color_t dropdown_selected_text;
    lv_color_t accent_color;
    lv_color_t accent_soft_color;
    lv_color_t energy_chart_bg;
    lv_color_t energy_chart_grid;
    lv_color_t energy_chart_tick;
    lv_color_t energy_buy_color;
    lv_color_t energy_solar_color;
    lv_color_t energy_charge_color;
    lv_color_t energy_sell_color;
    const lv_font_t *body_font;
    const lv_font_t *emphasis_font;
    lv_obj_t *brand;
    lv_obj_t *dropdown_list;
    bool use_background_image;

    if (view == NULL) {
        return;
    }

    effective_theme = gui_view_effective_theme(theme, night_variant_enabled);
    view->current_theme = effective_theme;
    view->current_show_background_image = show_background_image;
    view->current_night_variant_enabled = night_variant_enabled;
    view->has_current_appearance = true;

    {
        const gui_theme_def_t *def = gui_theme_get(effective_theme);

        if (def == NULL) {
            return;
        }

        use_background_image = (def->background_image != NULL) && show_background_image;
        body_font     = def->body_font;
        emphasis_font = def->emphasis_font;

        screen_bg  = lv_color_hex(def->screen_bg);
        screen_grad = lv_color_hex(def->screen_grad);
        screen_bg_opa = use_background_image ? LV_OPA_TRANSP : LV_OPA_COVER;

        sidebar_bg     = lv_color_hex(def->sidebar_bg);
        sidebar_grad   = lv_color_hex(def->sidebar_grad);
        sidebar_shadow = lv_color_hex(def->sidebar_shadow);
        sidebar_bg_opa = use_background_image ? LV_OPA_TRANSP : LV_OPA_COVER;
        brand_text     = lv_color_hex(def->brand_text);

        content_bg     = lv_color_hex(def->content_bg);
        content_shadow = lv_color_hex(def->content_shadow);
        content_bg_opa = use_background_image ? LV_OPA_TRANSP : LV_OPA_90;

        title_text    = lv_color_hex(def->title_text);
        subtitle_text = lv_color_hex(def->subtitle_text);
        panel_bg      = lv_color_hex(def->panel_bg);
        panel_border  = lv_color_hex(def->panel_border);
        panel_bg_opa  = LV_OPA_COVER;
        card_bg       = lv_color_hex(def->card_bg);
        card_border   = lv_color_hex(def->card_border);
        item_bg       = lv_color_hex(def->item_bg);
        item_border   = lv_color_hex(def->item_border);
        muted_text    = lv_color_hex(def->muted_text);
        value_text    = lv_color_hex(def->value_text);

        keyboard_bg             = lv_color_hex(def->keyboard_bg);
        keyboard_border         = lv_color_hex(def->keyboard_border);
        keyboard_key_bg         = lv_color_hex(def->keyboard_key_bg);
        keyboard_key_text       = lv_color_hex(def->keyboard_key_text);
        keyboard_special_bg     = lv_color_hex(def->keyboard_special_bg);
        keyboard_special_text   = lv_color_hex(def->keyboard_special_text);
        keyboard_special_border = lv_color_hex(def->keyboard_special_border);

        slider_bg      = lv_color_hex(def->slider_bg);
        slider_knob_bg = lv_color_hex(def->slider_knob_bg);

        dropdown_bg            = lv_color_hex(def->dropdown_bg);
        dropdown_border        = lv_color_hex(def->dropdown_border);
        dropdown_selected_bg   = lv_color_hex(def->dropdown_selected_bg);
        dropdown_selected_text = lv_color_hex(def->dropdown_selected_text);

        accent_color      = lv_color_hex(def->accent_color);
        accent_soft_color = lv_color_hex(def->accent_soft_color);

        energy_chart_bg     = lv_color_hex(def->energy_chart_bg);
        energy_chart_grid   = lv_color_hex(def->energy_chart_grid);
        energy_chart_tick   = lv_color_hex(def->energy_chart_tick);
        energy_buy_color    = lv_color_hex(def->energy_buy_color);
        energy_solar_color  = lv_color_hex(def->energy_solar_color);
        energy_charge_color = lv_color_hex(def->energy_charge_color);
        energy_sell_color   = lv_color_hex(def->energy_sell_color);
    }

    if (view->background_image != NULL) {
        const lv_img_dsc_t *background_src = gui_theme_get(effective_theme) != NULL
                             ? gui_theme_get(effective_theme)->background_image
                             : NULL;

        if (background_src != NULL) {
            lv_img_set_src(view->background_image, background_src);
        }

        if (use_background_image && (background_src != NULL)) {
            lv_obj_clear_flag(view->background_image, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(view->background_image, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_move_background(view->background_image);
    }

    lv_obj_set_style_bg_color(view->screen, screen_bg, 0);
    lv_obj_set_style_bg_grad_color(view->screen, screen_grad, 0);
    lv_obj_set_style_bg_grad_dir(view->screen, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(view->screen, screen_bg_opa, 0);

    lv_obj_set_style_bg_color(view->sidebar, sidebar_bg, 0);
    lv_obj_set_style_bg_grad_color(view->sidebar, sidebar_grad, 0);
    lv_obj_set_style_bg_grad_dir(view->sidebar, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(view->sidebar, sidebar_bg_opa, 0);
    lv_obj_set_style_border_opa(view->sidebar,
                                use_background_image ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(view->sidebar, use_background_image ? 0 : 8, 0);
    lv_obj_set_style_shadow_opa(view->sidebar,
                                use_background_image ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_color(view->sidebar, sidebar_shadow, 0);
    brand = lv_obj_get_child(view->sidebar, 0);
    if (brand != NULL) {
        lv_obj_set_style_text_color(brand, brand_text, 0);
        lv_obj_set_style_text_font(brand, body_font, 0);
    }

    lv_obj_set_style_bg_color(view->content, content_bg, 0);
    lv_obj_set_style_bg_opa(view->content, content_bg_opa, 0);
    lv_obj_set_style_border_opa(view->content,
                                use_background_image ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(view->content, use_background_image ? 0 : 10, 0);
    lv_obj_set_style_shadow_opa(view->content,
                                use_background_image ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_color(view->content, content_shadow, 0);
    lv_obj_set_style_text_color(view->header_title, title_text, 0);
    lv_obj_set_style_text_color(view->header_subtitle, subtitle_text, 0);
    lv_obj_set_style_text_font(view->header_title, body_font, 0);
    lv_obj_set_style_text_font(view->header_subtitle, body_font, 0);

    lv_obj_set_style_bg_color(view->bme280_panel, panel_bg, 0);
    lv_obj_set_style_bg_opa(view->bme280_panel, panel_bg_opa, 0);
    lv_obj_set_style_border_color(view->bme280_panel, panel_border, 0);
    lv_obj_set_style_text_font(view->bme280_panel, body_font, 0);
    gui_view_style_bme280_cards(view, card_bg, card_border, subtitle_text, value_text,
                                effective_theme);

    lv_obj_set_style_bg_color(view->energy_plan_panel, panel_bg, 0);
    lv_obj_set_style_bg_opa(view->energy_plan_panel, panel_bg_opa, 0);
    lv_obj_set_style_border_color(view->energy_plan_panel, panel_border, 0);
    lv_obj_set_style_bg_color(view->energy_plan_chart, energy_chart_bg, 0);
    lv_obj_set_style_border_color(view->energy_plan_chart, panel_border, 0);
    lv_obj_set_style_line_color(view->energy_plan_chart, energy_chart_grid, LV_PART_MAIN);
    lv_obj_set_style_line_opa(view->energy_plan_chart, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_line_color(view->energy_plan_chart, energy_chart_tick, LV_PART_TICKS);
    lv_obj_set_style_line_opa(view->energy_plan_chart, LV_OPA_COVER, LV_PART_TICKS);
    lv_obj_set_style_text_color(view->energy_plan_chart, energy_chart_tick, LV_PART_TICKS);
    lv_obj_set_style_text_font(view->energy_plan_chart, body_font, LV_PART_TICKS);
    if (view->buy_series != NULL) {
        lv_chart_set_series_color(view->energy_plan_chart, view->buy_series, energy_buy_color);
    }
    if (view->solar_series != NULL) {
        lv_chart_set_series_color(view->energy_plan_chart, view->solar_series,
                                  energy_solar_color);
    }
    if (view->charge_series != NULL) {
        lv_chart_set_series_color(view->energy_plan_chart, view->charge_series,
                                  energy_charge_color);
    }
    if (view->sell_series != NULL) {
        lv_chart_set_series_color(view->energy_plan_chart, view->sell_series,
                                  energy_sell_color);
    }
    if (view->energy_legend_dots[0] != NULL) {
        lv_obj_set_style_bg_color(view->energy_legend_dots[0], energy_buy_color, 0);
    }
    if (view->energy_legend_dots[1] != NULL) {
        lv_obj_set_style_bg_color(view->energy_legend_dots[1], energy_solar_color, 0);
    }
    if (view->energy_legend_dots[2] != NULL) {
        lv_obj_set_style_bg_color(view->energy_legend_dots[2], energy_charge_color, 0);
    }
    if (view->energy_legend_dots[3] != NULL) {
        lv_obj_set_style_bg_color(view->energy_legend_dots[3], energy_sell_color, 0);
    }
    lv_obj_set_style_text_font(view->energy_plan_panel, body_font, 0);
    gui_view_style_energy_labels(view->energy_plan_panel, subtitle_text, effective_theme);

    gui_view_style_forecast_panel(view, panel_bg, panel_border, panel_bg_opa, card_bg,
                                  card_border, title_text, subtitle_text, accent_color,
                                  effective_theme);

    gui_view_style_settings_card(view->connectivity_card, card_bg, card_border, title_text,
                                 subtitle_text, effective_theme);
    gui_view_style_settings_card(view->other_settings_card, card_bg, card_border, title_text,
                                 subtitle_text, effective_theme);
    gui_view_style_wifi_card(view, item_bg, item_border, title_text, subtitle_text,
                             effective_theme);
    gui_view_style_settings_card(view->bluetooth_card, item_bg, item_border, title_text,
                                 subtitle_text, effective_theme);
    gui_view_style_settings_card(view->brightness_card, item_bg, item_border, title_text,
                                 subtitle_text, effective_theme);
    gui_view_style_settings_card(view->theme_card, item_bg, item_border, title_text,
                                 subtitle_text, effective_theme);
    if (view->wifi_status_label != NULL) {
        gui_wifi_state_t wifi_state = view->has_last_wifi_settings
                                      ? view->last_wifi_settings.state
                                      : GUI_WIFI_STATE_IDLE;

        lv_obj_set_style_text_color(view->wifi_status_label,
                                    gui_view_wifi_status_color(view->current_theme,
                                                               wifi_state),
                                    0);
        lv_obj_set_style_text_font(view->wifi_status_label, body_font, 0);
    }
    if (view->bluetooth_status_label != NULL) {
        lv_obj_set_style_text_color(view->bluetooth_status_label, muted_text, 0);
        lv_obj_set_style_text_font(view->bluetooth_status_label, body_font, 0);
    }
    if (view->brightness_value_label != NULL) {
        lv_obj_set_style_text_color(view->brightness_value_label, accent_soft_color, 0);
        lv_obj_set_style_text_font(view->brightness_value_label, emphasis_font, 0);
    }
    if (view->brightness_slider != NULL) {
        lv_obj_set_style_bg_color(view->brightness_slider, slider_bg, LV_PART_MAIN);
        lv_obj_set_style_bg_color(view->brightness_slider, accent_color, LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(view->brightness_slider, slider_knob_bg, LV_PART_KNOB);
        lv_obj_set_style_border_color(view->brightness_slider, accent_color, LV_PART_KNOB);
    }

    if (view->theme_dropdown != NULL) {
        lv_obj_set_style_bg_color(view->theme_dropdown, dropdown_bg, 0);
        lv_obj_set_style_bg_opa(view->theme_dropdown, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(view->theme_dropdown, dropdown_border, 0);
        lv_obj_set_style_border_width(view->theme_dropdown, 1, 0);
        lv_obj_set_style_text_color(view->theme_dropdown, title_text, 0);
        lv_obj_set_style_text_font(view->theme_dropdown, body_font, 0);
        dropdown_list = lv_dropdown_get_list(view->theme_dropdown);
        if (dropdown_list != NULL) {
            lv_obj_set_style_bg_color(dropdown_list, dropdown_bg, 0);
            lv_obj_set_style_bg_opa(dropdown_list, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(dropdown_list, dropdown_border, 0);
            lv_obj_set_style_text_color(dropdown_list, title_text, 0);
            lv_obj_set_style_text_font(dropdown_list, body_font, 0);
            lv_obj_set_style_bg_color(dropdown_list, dropdown_selected_bg,
                                      LV_PART_SELECTED | LV_STATE_CHECKED);
            lv_obj_set_style_bg_opa(dropdown_list, LV_OPA_COVER,
                                    LV_PART_SELECTED | LV_STATE_CHECKED);
            lv_obj_set_style_text_color(dropdown_list, dropdown_selected_text,
                                        LV_PART_SELECTED | LV_STATE_CHECKED);
            lv_obj_set_style_text_font(dropdown_list, body_font,
                                       LV_PART_SELECTED | LV_STATE_CHECKED);
            lv_obj_set_style_bg_color(dropdown_list, dropdown_selected_bg, LV_PART_SELECTED);
            lv_obj_set_style_text_color(dropdown_list, dropdown_selected_text,
                                        LV_PART_SELECTED);
            lv_obj_set_style_text_font(dropdown_list, body_font, LV_PART_SELECTED);
        }
    }

    if (view->theme_background_label != NULL) {
        lv_obj_set_style_text_color(view->theme_background_label, title_text, 0);
        lv_obj_set_style_text_font(view->theme_background_label, body_font, 0);
    }

    if (view->theme_night_label != NULL) {
        lv_obj_set_style_text_color(view->theme_night_label, title_text, 0);
        lv_obj_set_style_text_font(view->theme_night_label, body_font, 0);
    }

    if (view->sidebar_clock_label != NULL) {
        lv_obj_set_style_text_color(view->sidebar_clock_label, title_text, 0);
        lv_obj_set_style_text_font(view->sidebar_clock_label, emphasis_font, 0);
    }
    if (view->sidebar_date_label != NULL) {
        lv_obj_set_style_text_color(view->sidebar_date_label, subtitle_text, 0);
        lv_obj_set_style_text_font(view->sidebar_date_label, body_font, 0);
    }

    if (view->theme_background_switch != NULL) {
        lv_obj_set_style_bg_color(view->theme_background_switch, slider_bg, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(view->theme_background_switch, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(view->theme_background_switch, dropdown_border,
                                      LV_PART_MAIN);
        lv_obj_set_style_border_width(view->theme_background_switch, 1, LV_PART_MAIN);
        lv_obj_set_style_bg_color(view->theme_background_switch, slider_bg,
                                  LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(view->theme_background_switch, LV_OPA_COVER,
                                LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(view->theme_background_switch, accent_color,
                                  LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(view->theme_background_switch, LV_OPA_COVER,
                                LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_set_style_border_width(view->theme_background_switch, 0,
                                      LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(view->theme_background_switch, slider_knob_bg, LV_PART_KNOB);
        lv_obj_set_style_bg_opa(view->theme_background_switch, LV_OPA_COVER, LV_PART_KNOB);
        lv_obj_set_style_border_width(view->theme_background_switch, 0, LV_PART_KNOB);
    }

    if (view->theme_night_switch != NULL) {
        lv_obj_set_style_bg_color(view->theme_night_switch, slider_bg, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(view->theme_night_switch, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(view->theme_night_switch, dropdown_border, LV_PART_MAIN);
        lv_obj_set_style_border_width(view->theme_night_switch, 1, LV_PART_MAIN);
        lv_obj_set_style_bg_color(view->theme_night_switch, slider_bg, LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(view->theme_night_switch, LV_OPA_COVER, LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(view->theme_night_switch, accent_color,
                                  LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(view->theme_night_switch, LV_OPA_COVER,
                                LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_set_style_border_width(view->theme_night_switch, 0,
                                      LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(view->theme_night_switch, slider_knob_bg, LV_PART_KNOB);
        lv_obj_set_style_bg_opa(view->theme_night_switch, LV_OPA_COVER, LV_PART_KNOB);
        lv_obj_set_style_border_width(view->theme_night_switch, 0, LV_PART_KNOB);
        lv_obj_set_style_bg_color(view->theme_night_switch, lv_color_hex(0x6B7280),
                                  LV_PART_MAIN | LV_STATE_DISABLED);
        lv_obj_set_style_bg_opa(view->theme_night_switch, LV_OPA_COVER,
                                LV_PART_MAIN | LV_STATE_DISABLED);
        lv_obj_set_style_border_color(view->theme_night_switch, lv_color_hex(0x6B7280),
                                      LV_PART_MAIN | LV_STATE_DISABLED);
        lv_obj_set_style_bg_color(view->theme_night_switch, lv_color_hex(0x9CA3AF),
                                  LV_PART_INDICATOR | LV_STATE_DISABLED);
        lv_obj_set_style_bg_opa(view->theme_night_switch, LV_OPA_COVER,
                                LV_PART_INDICATOR | LV_STATE_DISABLED);
    }

    gui_view_style_action_button(view->scan_button, effective_theme, true);
    gui_view_style_action_button(view->disconnect_button, effective_theme, false);
    gui_view_style_action_button(view->network_dialog_cancel_button, effective_theme, false);
    gui_view_style_action_button(view->password_dialog_cancel_button, effective_theme, false);
    gui_view_style_action_button(view->password_dialog_connect_button, effective_theme, true);
    gui_view_style_action_button(view->password_dialog_disconnect_button, effective_theme, false);

    lv_obj_set_style_bg_color(view->network_dialog, panel_bg, 0);
    lv_obj_set_style_bg_opa(view->network_dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(view->network_dialog,
                                  (gui_theme_get(effective_theme) != NULL &&
                                   gui_theme_get(effective_theme)->dialog_has_border) ? 1 : 0,
                                  0);
    lv_obj_set_style_border_color(view->network_dialog, panel_border, 0);
    if (view->network_dialog_title != NULL) {
        lv_obj_set_style_text_color(view->network_dialog_title, title_text, 0);
        lv_obj_set_style_text_font(view->network_dialog_title, body_font, 0);
    }
    if (view->network_dialog_subtitle != NULL) {
        lv_obj_set_style_text_color(view->network_dialog_subtitle, subtitle_text, 0);
        lv_obj_set_style_text_font(view->network_dialog_subtitle, body_font, 0);
    }
    if (view->network_empty_label != NULL) {
        lv_obj_set_style_text_color(view->network_empty_label, subtitle_text, 0);
        lv_obj_set_style_text_font(view->network_empty_label, body_font, 0);
    }
    for (uint32_t index = 0; index < GUI_WIFI_NETWORK_COUNT; index++) {
        if (view->network_dialog_button_labels[index] != NULL) {
            lv_obj_set_style_text_font(view->network_dialog_button_labels[index], body_font, 0);
        }
    }

    lv_obj_set_style_bg_color(view->password_dialog, panel_bg, 0);
    lv_obj_set_style_bg_opa(view->password_dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(view->password_dialog,
                                  (gui_theme_get(effective_theme) != NULL &&
                                   gui_theme_get(effective_theme)->dialog_has_border) ? 1 : 0,
                                  0);
    lv_obj_set_style_border_color(view->password_dialog, panel_border, 0);
    if (view->password_dialog_title != NULL) {
        lv_obj_set_style_text_color(view->password_dialog_title, title_text, 0);
        lv_obj_set_style_text_font(view->password_dialog_title, body_font, 0);
    }
    if (view->password_dialog_network_label != NULL) {
        lv_obj_set_style_text_color(view->password_dialog_network_label, subtitle_text, 0);
        lv_obj_set_style_text_font(view->password_dialog_network_label, body_font, 0);
    }

    lv_obj_set_style_bg_color(view->wifi_password_textarea, dropdown_bg, 0);
    lv_obj_set_style_bg_opa(view->wifi_password_textarea, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(view->wifi_password_textarea, dropdown_border, 0);
    lv_obj_set_style_text_color(view->wifi_password_textarea, title_text, 0);
    lv_obj_set_style_text_color(view->wifi_password_textarea, subtitle_text,
                                LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_font(view->wifi_password_textarea, body_font, 0);
    lv_obj_set_style_text_font(view->wifi_password_textarea, body_font,
                                LV_PART_TEXTAREA_PLACEHOLDER);

    lv_obj_set_style_bg_color(view->wifi_keyboard, keyboard_bg, 0);
    lv_obj_set_style_bg_opa(view->wifi_keyboard, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(view->wifi_keyboard, keyboard_border, 0);
    lv_obj_set_style_bg_color(view->wifi_keyboard, keyboard_key_bg, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(view->wifi_keyboard, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_border_color(view->wifi_keyboard, keyboard_border, LV_PART_ITEMS);
    lv_obj_set_style_text_color(view->wifi_keyboard, keyboard_key_text, LV_PART_ITEMS);
    lv_obj_set_style_text_font(view->wifi_keyboard, emphasis_font, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(view->wifi_keyboard,
                              lv_color_darken(keyboard_key_bg, LV_OPA_40),
                              LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(view->wifi_keyboard, LV_OPA_COVER,
                            LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(view->wifi_keyboard,
                                  lv_color_darken(keyboard_border, LV_OPA_40),
                                  LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(view->wifi_keyboard, keyboard_key_text,
                                LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(view->wifi_keyboard, keyboard_special_bg,
                              LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(view->wifi_keyboard, LV_OPA_COVER,
                            LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_border_color(view->wifi_keyboard, keyboard_special_border,
                                  LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(view->wifi_keyboard, keyboard_special_text,
                                LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(view->wifi_keyboard,
                              lv_color_darken(keyboard_special_bg, LV_OPA_40),
                              LV_PART_ITEMS | LV_STATE_PRESSED | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(view->wifi_keyboard, LV_OPA_COVER,
                            LV_PART_ITEMS | LV_STATE_PRESSED | LV_STATE_CHECKED);
    lv_obj_set_style_border_color(view->wifi_keyboard,
                                  lv_color_darken(keyboard_special_border, LV_OPA_40),
                                  LV_PART_ITEMS | LV_STATE_PRESSED | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(view->wifi_keyboard, keyboard_special_text,
                                LV_PART_ITEMS | LV_STATE_PRESSED | LV_STATE_CHECKED);

    if (view->has_last_active_panel) {
        gui_view_style_nav_button(view->bme280_button, view->current_theme,
                                  view->last_active_panel == GUI_PANEL_BME280);
        gui_view_style_nav_button(view->energy_plan_button, view->current_theme,
                                  view->last_active_panel == GUI_PANEL_ENERGY_PLAN);
        gui_view_style_nav_button(view->forecast_button, view->current_theme,
                                  view->last_active_panel == GUI_PANEL_FORECAST);
        gui_view_style_nav_button(view->settings_button, view->current_theme,
                                  view->last_active_panel == GUI_PANEL_SETTINGS);
    }
}

void gui_view_hide_wifi_dialogs(gui_view_t *view)
{
    gui_view_hide_wifi_dialogs_impl(view);
}

void gui_view_show_network_dialog(gui_view_t *view)
{
    gui_view_show_network_dialog_impl(view);
}

void gui_view_show_password_dialog(gui_view_t *view)
{
    gui_view_show_password_dialog_impl(view);
}

void gui_view_update_sidebar_clock_labels(gui_view_t *view)
{
    gui_view_update_sidebar_clock_impl(view);
}

void gui_view_init(gui_view_t *view, const gui_view_model_t *model, lv_event_cb_t nav_event_cb,
                   lv_event_cb_t settings_event_cb, void *event_user_data)
{
    lv_obj_t *sidebar;
    lv_obj_t *content;
    lv_obj_t *brand;

    if ((view == NULL) || (model == NULL)) {
        return;
    }

    view->current_theme = GUI_VIEW_THEME_LIGHT;
    view->current_show_background_image = true;
    view->current_night_variant_enabled = false;
    view->has_current_appearance = false;

    view->screen = lv_scr_act();
    lv_obj_clean(view->screen);
    lv_obj_clear_flag(view->screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(view->screen, lv_color_hex(0xDCE8F5), 0);
    lv_obj_set_style_bg_grad_color(view->screen, lv_color_hex(0xF5F9FF), 0);
    lv_obj_set_style_bg_grad_dir(view->screen, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(view->screen, LV_OPA_COVER, 0);

    view->background_image = lv_img_create(view->screen);
    {
        const gui_theme_def_t *default_theme = gui_theme_get(GUI_VIEW_THEME_HELLO_KITTY);
        if ((default_theme != NULL) && (default_theme->background_image != NULL)) {
            lv_img_set_src(view->background_image, default_theme->background_image);
        }
    }
    lv_obj_center(view->background_image);
    lv_obj_add_flag(view->background_image, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(view->background_image, LV_OBJ_FLAG_SCROLLABLE);

    sidebar = lv_obj_create(view->screen);
    view->sidebar = sidebar;
    lv_obj_set_size(sidebar, 188, 560);
    lv_obj_align(sidebar, LV_ALIGN_LEFT_MID, 18, 0);
    lv_obj_set_style_radius(sidebar, 28, 0);
    lv_obj_set_style_bg_color(sidebar, lv_color_hex(0x111827), 0);
    lv_obj_set_style_bg_grad_color(sidebar, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_grad_dir(sidebar, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(sidebar, 0, 0);
    lv_obj_set_style_shadow_width(sidebar, 8, 0);
    lv_obj_set_style_shadow_color(sidebar, lv_color_hex(0x94A3B8), 0);
    lv_obj_clear_flag(sidebar, LV_OBJ_FLAG_SCROLLABLE);

    brand = lv_label_create(sidebar);
    lv_label_set_text(brand, "Redmole");
    lv_obj_set_style_text_font(brand, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(brand, lv_color_hex(0xF8FAFC), 0);
    lv_obj_align(brand, LV_ALIGN_TOP_MID, 0, 28);

    view->sidebar_clock_label = lv_label_create(sidebar);
    lv_label_set_text(view->sidebar_clock_label, "--:--");
    lv_obj_set_style_text_font(view->sidebar_clock_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(view->sidebar_clock_label, lv_color_hex(0xDCE6F5), 0);
    lv_obj_align(view->sidebar_clock_label, LV_ALIGN_TOP_MID, 0, 58);

    view->sidebar_date_label = lv_label_create(sidebar);
    lv_label_set_text(view->sidebar_date_label, "--- -- ---");
    lv_obj_set_style_text_font(view->sidebar_date_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(view->sidebar_date_label, lv_color_hex(0x94A3B8), 0);
    lv_obj_align(view->sidebar_date_label, LV_ALIGN_TOP_MID, 0, 84);

    view->bme280_button = gui_view_create_nav_button(sidebar, 140, "BME280", nav_event_cb,
                                                       event_user_data);
    view->energy_plan_button = gui_view_create_nav_button(sidebar, 212, "Energy plan",
                                                          nav_event_cb, event_user_data);
    view->forecast_button = gui_view_create_nav_button(sidebar, 284, "Forecast", nav_event_cb,
                                                       event_user_data);
    view->settings_button = gui_view_create_nav_button(sidebar, 356, "Settings", nav_event_cb,
                                                       event_user_data);

    // Create horizontal container for connectivity icons
    lv_obj_t * connectivity_container = lv_obj_create(sidebar);
    lv_obj_set_size(connectivity_container, 188, 50);
    lv_obj_align(connectivity_container, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_set_flex_flow(connectivity_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(connectivity_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(connectivity_container, LV_OBJ_FLAG_SCROLLABLE);

    // Make container invisible (for layout purposes only)
    lv_obj_set_style_bg_opa(connectivity_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(connectivity_container, LV_OPA_TRANSP, 0);

    view->sidebar_wifi_label = lv_label_create(connectivity_container);
    lv_label_set_text(view->sidebar_wifi_label, LV_SYMBOL_WIFI);

    lv_obj_align(view->sidebar_wifi_label, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_text_font(view->sidebar_wifi_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(view->sidebar_wifi_label, lv_color_white(), 0);

    view->sidebar_bluetooth_label = lv_label_create(connectivity_container);
    lv_label_set_text(view->sidebar_bluetooth_label, LV_SYMBOL_BLUETOOTH);

    lv_obj_align(view->sidebar_bluetooth_label, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_text_font(view->sidebar_bluetooth_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(view->sidebar_bluetooth_label, lv_color_white(), 0);

    view->sidebar_sd_card_label = lv_label_create(connectivity_container);
    lv_label_set_text(view->sidebar_sd_card_label, LV_SYMBOL_SD_CARD);

    lv_obj_align(view->sidebar_sd_card_label, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_text_font(view->sidebar_sd_card_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(view->sidebar_sd_card_label, lv_color_white(), 0);

    content = lv_obj_create(view->screen);
    view->content = content;
    lv_obj_set_size(content, 786, 560);
    lv_obj_align(content, LV_ALIGN_RIGHT_MID, -18, 0);
    lv_obj_set_style_radius(content, 32, 0);
    lv_obj_set_style_bg_color(content, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_90, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_shadow_width(content, 10, 0);
    lv_obj_set_style_shadow_color(content, lv_color_hex(0xB8C7DB), 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    view->header_title = lv_label_create(content);
    lv_obj_set_style_text_font(view->header_title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(view->header_title, lv_color_hex(0x10213D), 0);
    lv_obj_align(view->header_title, LV_ALIGN_TOP_LEFT, 34, 30);

    view->header_subtitle = lv_label_create(content);
    lv_obj_set_style_text_color(view->header_subtitle, lv_color_hex(0x607089), 0);
    lv_obj_align(view->header_subtitle, LV_ALIGN_TOP_LEFT, 36, 76);

    gui_view_init_bme280_panel(view, content);
    gui_view_init_settings_panel(view, settings_event_cb, event_user_data);
    gui_view_init_energy_panel(view, content);
    gui_view_init_forecast_panel(view, content);

    gui_view_apply(view, model);
    gui_view_update_sidebar_clock_impl(view);
}

void gui_view_apply(gui_view_t *view, const gui_view_model_t *model)
{
    bool panel_changed;

    if ((view == NULL) || (model == NULL)) {
        return;
    }

    panel_changed = !view->has_last_active_panel || (view->last_active_panel != model->active_panel);

    if (!view->has_current_appearance ||
        (view->current_theme != gui_view_effective_theme(model->appearance.theme,
                                                         model->appearance.night_variant_enabled)) ||
        (view->current_show_background_image != model->appearance.show_background_image) ||
        (view->current_night_variant_enabled != model->appearance.night_variant_enabled)) {
        gui_view_apply_theme(view, model->appearance.theme,
                             model->appearance.show_background_image,
                             model->appearance.night_variant_enabled);
    }

    gui_view_apply_connectivity_icon_colors(view, model);

    if (panel_changed) {
        gui_view_style_nav_button(view->bme280_button, view->current_theme,
                                  model->active_panel == GUI_PANEL_BME280);
        gui_view_style_nav_button(view->energy_plan_button, view->current_theme,
                                  model->active_panel == GUI_PANEL_ENERGY_PLAN);
        gui_view_style_nav_button(view->forecast_button, view->current_theme,
                                  model->active_panel == GUI_PANEL_FORECAST);
        gui_view_style_nav_button(view->settings_button, view->current_theme,
                                  model->active_panel == GUI_PANEL_SETTINGS);
        gui_view_apply_header(view, model);

        if (model->active_panel != GUI_PANEL_SETTINGS) {
            gui_view_hide_wifi_dialogs(view);
        }

        view->last_active_panel = model->active_panel;
        view->has_last_active_panel = true;
    }

    if (model->active_panel == GUI_PANEL_BME280) {
        gui_view_apply_bme280_panel(view, model);
    } else if (model->active_panel == GUI_PANEL_ENERGY_PLAN) {
        gui_view_apply_energy_panel(view, model);
    } else if (model->active_panel == GUI_PANEL_FORECAST) {
        gui_view_apply_forecast_panel(view, model);
    } else {
        gui_view_apply_settings_panel(view, model);
    }
}
