#include "gui_view_forecast_panel.h"

#include "../gui_view_common.h"

static void gui_view_layout_forecast_panel(gui_view_t *view)
{
    lv_obj_t *top_row;
    lv_obj_t *days_row;
    lv_obj_t *today_card;
    lv_obj_t *details_card;
    lv_coord_t panel_width;
    lv_coord_t panel_height;

    if ((view == NULL) || (view->content == NULL) || (view->forecast_panel == NULL)) {
        return;
    }

    panel_width = lv_obj_get_content_width(view->content) - 48;
    panel_height = lv_obj_get_content_height(view->content) - 48;
    if ((panel_width <= 0) || (panel_height <= 0)) {
        return;
    }

    lv_obj_set_size(view->forecast_panel, panel_width, panel_height);
    lv_obj_align(view->forecast_panel, LV_ALIGN_CENTER, 0, 0);

    top_row = lv_obj_get_child(view->forecast_panel, 0);
    days_row = lv_obj_get_child(view->forecast_panel, 1);
    if ((top_row == NULL) || (days_row == NULL)) {
        return;
    }

    lv_obj_set_width(top_row, LV_PCT(100));
    lv_obj_set_height(top_row, (panel_height * 2) / 5);
    lv_obj_set_width(days_row, LV_PCT(100));

    today_card = lv_obj_get_child(top_row, 0);
    details_card = lv_obj_get_child(top_row, 1);
    if (today_card != NULL) {
        lv_obj_set_width(today_card, 0);
        lv_obj_set_height(today_card, LV_PCT(100));
        lv_obj_set_flex_grow(today_card, 2);
    }
    if (details_card != NULL) {
        lv_obj_set_width(details_card, 0);
        lv_obj_set_height(details_card, LV_PCT(100));
        lv_obj_set_flex_grow(details_card, 1);
    }
}

static lv_obj_t *gui_view_create_forecast_day_card(lv_obj_t *parent, const char *day_text,
                                                   const char *condition_text,
                                                   const char *temp_text)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_t *label;
    lv_obj_t *spacer;

    lv_obj_set_size(card, 130, LV_PCT(100));
    lv_obj_set_style_radius(card, 18, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xD9E3F1), 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 14, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(card, 0);
    lv_obj_set_flex_grow(card, 1);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(card, 6, 0);

    label = lv_label_create(card);
    lv_label_set_text(label, day_text);
    lv_obj_set_style_text_color(label, lv_color_hex(0x10213D), 0);

    label = lv_label_create(card);
    lv_label_set_text(label, condition_text);
    lv_obj_set_style_text_color(label, lv_color_hex(0x607089), 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, LV_PCT(100));

    spacer = lv_obj_create(card);
    lv_obj_set_size(spacer, LV_PCT(100), 0);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_set_style_pad_all(spacer, 0, 0);
    lv_obj_set_style_shadow_width(spacer, 0, 0);
    lv_obj_clear_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_grow(spacer, 1);

    label = lv_label_create(card);
    lv_label_set_text(label, temp_text);
    lv_obj_set_style_text_color(label, lv_color_hex(0x1D4ED8), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);

    return card;
}

void gui_view_init_forecast_panel(gui_view_t *view, lv_obj_t *content)
{
    lv_obj_t *top_row;
    lv_obj_t *today_card;
    lv_obj_t *details_card;
    lv_obj_t *days_row;
    lv_obj_t *label;

    if ((view == NULL) || (content == NULL)) {
        return;
    }

    view->forecast_panel = lv_obj_create(content);
    lv_obj_set_size(view->forecast_panel, 746, 432);
    lv_obj_align(view->forecast_panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(view->forecast_panel, 26, 0);
    lv_obj_set_style_bg_color(view->forecast_panel, lv_color_hex(0xF8FBFF), 0);
    lv_obj_set_style_bg_opa(view->forecast_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(view->forecast_panel, 1, 0);
    lv_obj_set_style_border_color(view->forecast_panel, lv_color_hex(0xD9E3F1), 0);
    lv_obj_set_style_pad_all(view->forecast_panel, 24, 0);
    lv_obj_set_style_pad_row(view->forecast_panel, 20, 0);
    lv_obj_clear_flag(view->forecast_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(view->forecast_panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(view->forecast_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(view->forecast_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);

    top_row = lv_obj_create(view->forecast_panel);
    lv_obj_set_size(top_row, LV_PCT(100), 188);
    lv_obj_set_style_bg_opa(top_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(top_row, 0, 0);
    lv_obj_set_style_shadow_width(top_row, 0, 0);
    lv_obj_set_style_pad_all(top_row, 0, 0);
    lv_obj_set_style_pad_column(top_row, 20, 0);
    lv_obj_clear_flag(top_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(top_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(top_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);

    today_card = lv_obj_create(top_row);
    lv_obj_set_size(today_card, 0, LV_PCT(100));
    lv_obj_set_style_radius(today_card, 22, 0);
    lv_obj_set_style_bg_color(today_card, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(today_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(today_card, 1, 0);
    lv_obj_set_style_border_color(today_card, lv_color_hex(0xD9E3F1), 0);
    lv_obj_set_style_shadow_width(today_card, 0, 0);
    lv_obj_set_style_pad_all(today_card, 20, 0);
    lv_obj_clear_flag(today_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(today_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(today_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(today_card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(today_card, 8, 0);
    lv_obj_set_flex_grow(today_card, 2);

    label = lv_label_create(today_card);
    lv_label_set_text(label, "Today");
    lv_obj_set_style_text_color(label, lv_color_hex(0x607089), 0);

    label = lv_label_create(today_card);
    lv_label_set_text(label, "Mostly cloudy");
    lv_obj_set_style_text_color(label, lv_color_hex(0x10213D), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);

    label = lv_label_create(today_card);
    lv_label_set_text(label, "18 C");
    lv_obj_set_style_text_color(label, lv_color_hex(0x1D4ED8), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);

    label = lv_label_create(today_card);
    lv_label_set_text(label, "High 21 C  |  Low 13 C");
    lv_obj_set_style_text_color(label, lv_color_hex(0x607089), 0);

    label = lv_label_create(today_card);
    lv_label_set_text(label, "Cool morning, brighter later, light winds.");
    lv_obj_set_width(label, LV_PCT(100));
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(label, lv_color_hex(0x607089), 0);

    details_card = lv_obj_create(top_row);
    lv_obj_set_size(details_card, 0, LV_PCT(100));
    lv_obj_set_style_radius(details_card, 22, 0);
    lv_obj_set_style_bg_color(details_card, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(details_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(details_card, 1, 0);
    lv_obj_set_style_border_color(details_card, lv_color_hex(0xD9E3F1), 0);
    lv_obj_set_style_shadow_width(details_card, 0, 0);
    lv_obj_set_style_pad_all(details_card, 20, 0);
    lv_obj_clear_flag(details_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(details_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(details_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(details_card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(details_card, 10, 0);
    lv_obj_set_flex_grow(details_card, 1);

    label = lv_label_create(details_card);
    lv_label_set_text(label, "Forecast details");
    lv_obj_set_style_text_color(label, lv_color_hex(0x607089), 0);

    label = lv_label_create(details_card);
    lv_label_set_text(label, "Rain chance: 20%");
    lv_obj_set_style_text_color(label, lv_color_hex(0x10213D), 0);

    label = lv_label_create(details_card);
    lv_label_set_text(label, "Wind: 4 m/s NW");
    lv_obj_set_style_text_color(label, lv_color_hex(0x10213D), 0);

    label = lv_label_create(details_card);
    lv_label_set_text(label, "Humidity: 61%");
    lv_obj_set_style_text_color(label, lv_color_hex(0x10213D), 0);

    label = lv_label_create(details_card);
    lv_label_set_text(label, "UV index: 3");
    lv_obj_set_style_text_color(label, lv_color_hex(0x10213D), 0);

    days_row = lv_obj_create(view->forecast_panel);
    lv_obj_set_size(days_row, LV_PCT(100), 0);
    lv_obj_set_style_bg_opa(days_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(days_row, 0, 0);
    lv_obj_set_style_shadow_width(days_row, 0, 0);
    lv_obj_set_style_pad_all(days_row, 0, 0);
    lv_obj_set_style_pad_column(days_row, 12, 0);
    lv_obj_set_style_pad_row(days_row, 12, 0);
    lv_obj_clear_flag(days_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(days_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(days_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(days_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_flex_grow(days_row, 1);

    (void)gui_view_create_forecast_day_card(days_row, "Mon", "Cloudy", "20 / 12 C");
    (void)gui_view_create_forecast_day_card(days_row, "Tue", "Light rain", "17 / 10 C");
    (void)gui_view_create_forecast_day_card(days_row, "Wed", "Sunny", "22 / 11 C");
    (void)gui_view_create_forecast_day_card(days_row, "Thu", "Windy", "19 / 9 C");
    (void)gui_view_create_forecast_day_card(days_row, "Fri", "Partly sunny", "21 / 13 C");

    gui_view_layout_forecast_panel(view);
}

void gui_view_apply_forecast_panel(gui_view_t *view, const gui_view_model_t *model)
{
    gui_view_layout_forecast_panel(view);
    (void)view;
    (void)model;
}
