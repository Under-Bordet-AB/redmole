#include "gui_view_forecast_panel.h"

#include <stdint.h>
#include <string.h>

#include "../assets/weather_icons/gui_weather_icons.h"
#include "../gui_theme_defs.h"
#include "../gui_view_common.h"

#define FORECAST_TODAY_ICON_SIZE 136
//#define FORECAST_DAY_ICON_SIZE   76
#define FORECAST_DAY_ICON_SIZE   72
#define FORECAST_ENABLE_TODAY_ICON 1
#define FORECAST_ENABLE_DAY_ICONS  1
#define FORECAST_FORCE_TODAY_CLEAR_FOR_TEST 0
#define FORECAST_WEATHER_ICON_BASE_WIDTH 128
#define FORECAST_WEATHER_ICON_BASE_HEIGHT 129
#define FORECAST_DEFAULT_ACCENT_COLOR 0x1D4ED8
#define FORECAST_FEELS_LIKE_PREFIX "Feels like "

static void gui_view_forecast_set_label_text(lv_obj_t *parent, uint32_t child_index,
                                             const char *text)
{
    lv_obj_t *label;

    if ((parent == NULL) || (text == NULL)) {
        return;
    }

    label = lv_obj_get_child(parent, (int32_t)child_index);
    if (label == NULL) {
        return;
    }

    lv_label_set_text(label, text);
}

static uint32_t gui_view_forecast_accent_hex(const gui_view_t *view)
{
    const gui_theme_def_t *def;

    if (view == NULL) {
        return FORECAST_DEFAULT_ACCENT_COLOR;
    }

    def = gui_theme_get(view->current_theme);
    return (def != NULL) ? def->accent_color : FORECAST_DEFAULT_ACCENT_COLOR;
}

static void gui_view_forecast_set_feels_like_row_text(lv_obj_t *row, const char *text)
{
    lv_obj_t *prefix_label;
    lv_obj_t *value_label;
    size_t prefix_len;

    if ((row == NULL) || (text == NULL)) {
        return;
    }

    prefix_label = lv_obj_get_child(row, 0);
    value_label = lv_obj_get_child(row, 1);
    if ((prefix_label == NULL) || (value_label == NULL)) {
        return;
    }

    prefix_len = strlen(FORECAST_FEELS_LIKE_PREFIX);
    if ((strncmp(text, FORECAST_FEELS_LIKE_PREFIX, prefix_len) == 0) &&
        (text[prefix_len] != '\0')) {
        lv_label_set_text(prefix_label, "Feels like");
        lv_label_set_text(value_label, &text[prefix_len]);
        return;
    }

    lv_label_set_text(prefix_label, text);
    lv_label_set_text(value_label, "");
}

static void gui_view_forecast_set_detail_row_text(lv_obj_t *row, const char *text)
{
    lv_obj_t *name_label;
    lv_obj_t *value_label;
    const char *colon;
    const char *value_start;
    size_t name_len;

    if ((row == NULL) || (text == NULL)) {
        return;
    }

    name_label = lv_obj_get_child(row, 0);
    value_label = lv_obj_get_child(row, 1);
    if ((name_label == NULL) || (value_label == NULL)) {
        return;
    }

    colon = strchr(text, ':');
    if (colon == NULL) {
        lv_label_set_text(name_label, text);
        lv_label_set_text(value_label, "");
        return;
    }

    name_len = (size_t)(colon - text) + 1;
    value_start = colon + 1;
    while (*value_start == ' ') {
        value_start++;
    }

    lv_label_set_text_fmt(name_label, "%.*s", (int)name_len, text);
    lv_label_set_text(value_label, value_start);
}

static void gui_view_forecast_set_detail_text(lv_obj_t *parent, uint32_t child_index,
                                              const char *text)
{
    if ((parent == NULL) || (text == NULL)) {
        return;
    }

    gui_view_forecast_set_detail_row_text(lv_obj_get_child(parent, (int32_t)child_index),
                                          text);
}

static lv_obj_t *gui_view_forecast_create_feels_like_row(lv_obj_t *parent, const char *text)
{
    lv_obj_t *row;
    lv_obj_t *label;

    row = lv_obj_create(parent);
    lv_obj_set_width(row, LV_SIZE_CONTENT);
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_shadow_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_pad_column(row, 5, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    label = lv_label_create(row);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(label, lv_color_hex(0x607089), 0);

    label = lv_label_create(row);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(label, lv_color_hex(0x1D4ED8), 0);

    gui_view_forecast_set_feels_like_row_text(row, text);

    return row;
}

static lv_obj_t *gui_view_forecast_create_detail_row(lv_obj_t *parent, const char *text)
{
    lv_obj_t *row;
    lv_obj_t *label;

    row = lv_obj_create(parent);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_shadow_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_pad_column(row, 8, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    label = lv_label_create(row);
    lv_obj_set_width(label, 0);
    lv_obj_set_flex_grow(label, 1);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_color(label, lv_color_hex(0x10213D), 0);

    label = lv_label_create(row);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x1D4ED8), 0);

    gui_view_forecast_set_detail_row_text(row, text);

    return row;
}

static void gui_view_forecast_prepare_icon_slot(lv_obj_t *slot, lv_coord_t size)
{
    lv_obj_set_size(slot, size, size);
    lv_obj_set_style_bg_opa(slot, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(slot, 0, 0);
    lv_obj_set_style_pad_all(slot, 0, 0);
    lv_obj_set_style_shadow_width(slot, 0, 0);
    lv_obj_clear_flag(slot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(slot, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
}

static lv_color_t gui_view_forecast_icon_color(const gui_view_t *view)
{
    return lv_color_hex(gui_view_forecast_accent_hex(view));
}

static void gui_view_forecast_tint_icon_slot(lv_obj_t *slot, lv_color_t icon_color)
{
    lv_obj_t *label;

    if (slot == NULL) {
        return;
    }

    label = lv_obj_get_child(slot, 0);
    if (label == NULL) {
        return;
    }

    lv_obj_set_style_text_color(label, icon_color, 0);
}

static int32_t gui_view_forecast_icon_zoom(lv_coord_t size)
{
    if (size <= 0) {
        return LV_IMG_ZOOM_NONE;
    }

    return ((int32_t)size * LV_IMG_ZOOM_NONE + (FORECAST_WEATHER_ICON_BASE_WIDTH / 2)) /
           FORECAST_WEATHER_ICON_BASE_WIDTH;
}

static void gui_view_forecast_style_icon_label(lv_obj_t *label, lv_coord_t size,
                                               lv_color_t icon_color)
{
    if (label == NULL) {
        return;
    }

    lv_obj_set_size(label, FORECAST_WEATHER_ICON_BASE_WIDTH,
                    FORECAST_WEATHER_ICON_BASE_HEIGHT);
    lv_obj_set_style_text_font(label, &weather_icons_128, 0);
    lv_obj_set_style_text_color(label, icon_color, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_all(label, 0, 0);
    lv_obj_set_style_transform_zoom(label, gui_view_forecast_icon_zoom(size), 0);
    lv_obj_set_style_transform_pivot_x(label, FORECAST_WEATHER_ICON_BASE_WIDTH / 2, 0);
    lv_obj_set_style_transform_pivot_y(label, FORECAST_WEATHER_ICON_BASE_HEIGHT / 2, 0);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(label);
}

static void gui_view_forecast_apply_icon(lv_obj_t *slot, gui_weather_icon_t icon,
                                         lv_coord_t size, lv_color_t icon_color)
{
    lv_obj_t *label;
    void *icon_marker;

    if (slot == NULL) {
        return;
    }

    if ((icon < 0) || (icon >= GUI_WEATHER_ICON_COUNT)) {
        icon = GUI_WEATHER_ICON_CLOUDY;
    }

    if (((size == FORECAST_TODAY_ICON_SIZE) && !FORECAST_ENABLE_TODAY_ICON) ||
        ((size == FORECAST_DAY_ICON_SIZE) && !FORECAST_ENABLE_DAY_ICONS)) {
        lv_obj_clean(slot);
        lv_obj_set_user_data(slot, NULL);
        gui_view_forecast_prepare_icon_slot(slot, size);
        return;
    }

    icon_marker = (void *)(uintptr_t)(icon + 1);
    if ((lv_obj_get_user_data(slot) == icon_marker) && (lv_obj_get_child(slot, 0) != NULL)) {
        gui_view_forecast_tint_icon_slot(slot, icon_color);
        gui_view_forecast_style_icon_label(lv_obj_get_child(slot, 0), size, icon_color);
        return;
    }

    lv_obj_clean(slot);
    lv_obj_set_user_data(slot, icon_marker);
    gui_view_forecast_prepare_icon_slot(slot, size);

    label = lv_label_create(slot);
    lv_label_set_text(label, gui_weather_icons_get_symbol(icon));
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    gui_view_forecast_style_icon_label(label, size, icon_color);
}

static void gui_view_forecast_apply_day_card(lv_obj_t *card, const gui_forecast_day_t *day,
                                             lv_color_t icon_color)
{
    lv_obj_t *icon_area;

    if ((card == NULL) || (day == NULL)) {
        return;
    }

    gui_view_forecast_set_label_text(card, 0, day->label);
    gui_view_forecast_set_label_text(card, 1, day->date_text);
    icon_area = lv_obj_get_child(card, 2);
    gui_view_forecast_apply_icon((icon_area != NULL) ? lv_obj_get_child(icon_area, 0) : NULL,
                                 day->icon, FORECAST_DAY_ICON_SIZE, icon_color);
    gui_view_forecast_set_label_text(card, 3, day->range_text);
}

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
                                                   const char *date_text,
                                                   gui_weather_icon_t icon,
                                                   const char *temp_text)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_t *icon_area;
    lv_obj_t *icon_slot;
    lv_obj_t *label;

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
    lv_label_set_text(label, date_text);
    lv_obj_set_style_text_color(label, lv_color_hex(0x607089), 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, LV_PCT(100));

    icon_area = lv_obj_create(card);
    lv_obj_set_size(icon_area, LV_PCT(100), 0);
    lv_obj_set_style_bg_opa(icon_area, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(icon_area, 0, 0);
    lv_obj_set_style_pad_all(icon_area, 0, 0);
    lv_obj_set_style_shadow_width(icon_area, 0, 0);
    lv_obj_clear_flag(icon_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(icon_area, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(icon_area, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(icon_area, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_flex_grow(icon_area, 1);

    icon_slot = lv_obj_create(icon_area);
    gui_view_forecast_prepare_icon_slot(icon_slot, FORECAST_DAY_ICON_SIZE);
    gui_view_forecast_apply_icon(icon_slot, icon, FORECAST_DAY_ICON_SIZE,
                                 lv_color_hex(0x1D4ED8));

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
    lv_obj_t *today_icon_slot;
    lv_obj_t *today_text_column;
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
    lv_obj_set_flex_flow(today_card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(today_card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(today_card, 16, 0);
    lv_obj_set_flex_grow(today_card, 2);

    today_text_column = lv_obj_create(today_card);
    lv_obj_set_size(today_text_column, 0, LV_PCT(100));
    lv_obj_set_style_bg_opa(today_text_column, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(today_text_column, 0, 0);
    lv_obj_set_style_shadow_width(today_text_column, 0, 0);
    lv_obj_set_style_pad_all(today_text_column, 0, 0);
    lv_obj_set_style_pad_row(today_text_column, 6, 0);
    lv_obj_clear_flag(today_text_column, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(today_text_column, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(today_text_column, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(today_text_column, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_flex_grow(today_text_column, 1);

    label = lv_label_create(today_text_column);
    lv_label_set_text(label, "Right now");
    lv_obj_set_style_text_color(label, lv_color_hex(0x607089), 0);

    label = lv_label_create(today_text_column);
    lv_label_set_text(label, "Mostly cloudy");
    lv_obj_set_style_text_color(label, lv_color_hex(0x10213D), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);

    label = lv_label_create(today_text_column);
    lv_label_set_text(label, "18 C");
    lv_obj_set_style_text_color(label, lv_color_hex(0x1D4ED8), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);

    (void)gui_view_forecast_create_feels_like_row(today_text_column, "Feels like 18 C");

    label = lv_label_create(today_text_column);
    lv_label_set_text(label, "Stays mild later today.");
    lv_obj_set_width(label, LV_PCT(100));
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(label, lv_color_hex(0x607089), 0);

    today_icon_slot = lv_obj_create(today_card);
    gui_view_forecast_prepare_icon_slot(today_icon_slot, FORECAST_TODAY_ICON_SIZE);
    gui_view_forecast_apply_icon(today_icon_slot, GUI_WEATHER_ICON_CLOUDY,
                                 FORECAST_TODAY_ICON_SIZE, lv_color_hex(0x1D4ED8));

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
    lv_label_set_text(label, "Details");
    lv_obj_set_style_text_color(label, lv_color_hex(0x607089), 0);

    (void)gui_view_forecast_create_detail_row(details_card, "Rain chance: 20%");
    (void)gui_view_forecast_create_detail_row(details_card, "Wind: 4 m/s NW");
    (void)gui_view_forecast_create_detail_row(details_card, "Humidity: 61%");
    (void)gui_view_forecast_create_detail_row(details_card, "UV index: 3");

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

    (void)gui_view_create_forecast_day_card(days_row, "Mon", "May 25",
                                            GUI_WEATHER_ICON_CLOUDY, "20 / 12 C");
    (void)gui_view_create_forecast_day_card(days_row, "Tue", "May 26",
                                            GUI_WEATHER_ICON_RAIN, "17 / 10 C");
    (void)gui_view_create_forecast_day_card(days_row, "Wed", "May 27",
                                            GUI_WEATHER_ICON_CLEAR, "22 / 11 C");
    (void)gui_view_create_forecast_day_card(days_row, "Thu", "May 28",
                                            GUI_WEATHER_ICON_CLOUDY, "19 / 9 C");
    (void)gui_view_create_forecast_day_card(days_row, "Fri", "May 29",
                                            GUI_WEATHER_ICON_PARTLY_CLOUDY, "21 / 13 C");

    gui_view_layout_forecast_panel(view);
}

void gui_view_apply_forecast_panel(gui_view_t *view, const gui_view_model_t *model)
{
    lv_obj_t *top_row;
    lv_obj_t *today_card;
    lv_obj_t *today_icon_slot;
    lv_obj_t *today_text_column;
    lv_obj_t *details_card;
    lv_obj_t *days_row;
    lv_color_t icon_color;
    uint32_t day_index;

    gui_view_layout_forecast_panel(view);

    if ((view == NULL) || (model == NULL) || (view->forecast_panel == NULL)) {
        return;
    }

    top_row = lv_obj_get_child(view->forecast_panel, 0);
    days_row = lv_obj_get_child(view->forecast_panel, 1);
    if ((top_row == NULL) || (days_row == NULL)) {
        return;
    }

    today_card = lv_obj_get_child(top_row, 0);
    details_card = lv_obj_get_child(top_row, 1);
    if ((today_card == NULL) || (details_card == NULL)) {
        return;
    }

    today_text_column = lv_obj_get_child(today_card, 0);
    today_icon_slot = lv_obj_get_child(today_card, 1);
    icon_color = gui_view_forecast_icon_color(view);
    gui_view_forecast_set_label_text(today_text_column, 0, model->forecast.title);
    gui_view_forecast_set_label_text(today_text_column, 1, model->forecast.condition);
    gui_view_forecast_set_label_text(today_text_column, 2,
                                     model->forecast.current_temperature);
    gui_view_forecast_set_feels_like_row_text(lv_obj_get_child(today_text_column, 3),
                                              model->forecast.feels_like_temperature);
    gui_view_forecast_set_label_text(today_text_column, 4, model->forecast.summary);
    gui_view_forecast_apply_icon(today_icon_slot,
#if FORECAST_FORCE_TODAY_CLEAR_FOR_TEST
                                 GUI_WEATHER_ICON_CLEAR,
#else
                                 model->forecast.current_icon,
#endif
                                 FORECAST_TODAY_ICON_SIZE, icon_color);

    gui_view_forecast_set_label_text(details_card, 0, "Details");
    gui_view_forecast_set_detail_text(details_card, 1, model->forecast.details.rain_chance);
    gui_view_forecast_set_detail_text(details_card, 2, model->forecast.details.wind);
    gui_view_forecast_set_detail_text(details_card, 3, model->forecast.details.humidity);
    gui_view_forecast_set_detail_text(details_card, 4, model->forecast.details.uv_index);

    for (day_index = 0; day_index < GUI_FORECAST_DAY_COUNT; day_index++) {
        gui_view_forecast_apply_day_card(lv_obj_get_child(days_row, (int32_t)day_index),
                                         &model->forecast.days[day_index], icon_color);
    }
}
