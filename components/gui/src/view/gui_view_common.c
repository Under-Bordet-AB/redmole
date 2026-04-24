#include "gui_view_common.h"

#include <string.h>

int32_t gui_view_abs_i32(int32_t value)
{
    return (value < 0) ? -value : value;
}

void gui_view_set_label_text_if_changed(lv_obj_t *label, const char *text)
{
    const char *current_text;

    if ((label == NULL) || (text == NULL)) {
        return;
    }

    current_text = lv_label_get_text(label);
    if ((current_text != NULL) && (strcmp(current_text, text) == 0)) {
        return;
    }

    lv_label_set_text(label, text);
}

void gui_view_set_textarea_text_if_changed(lv_obj_t *textarea, const char *text)
{
    const char *current_text;

    if ((textarea == NULL) || (text == NULL)) {
        return;
    }

    current_text = lv_textarea_get_text(textarea);
    if ((current_text != NULL) && (strcmp(current_text, text) == 0)) {
        return;
    }

    lv_textarea_set_text(textarea, text);
}

void gui_view_style_scanned_wifi_button(lv_obj_t *button, gui_view_theme_t theme,
                                        bool is_selected, bool is_known, bool is_connected)
{
    lv_color_t bg_color;
    lv_color_t border_color;
    lv_color_t text_color;

    if (theme == GUI_VIEW_THEME_DARK) {
        bg_color = lv_color_hex(0x182334);
        border_color = lv_color_hex(0x314155);
        text_color = lv_color_hex(0xD7E3F4);
    } else if (theme == GUI_VIEW_THEME_HELLO_KITTY) {
        bg_color = lv_color_hex(0xFFF7FB);
        border_color = lv_color_hex(0xF9B7CD);
        text_color = lv_color_hex(0x8A284E);
    } else if (theme == GUI_VIEW_THEME_HELLO_KITTY_NIGHT) {
        bg_color = lv_color_hex(0x2A2744);
        border_color = lv_color_hex(0x3A3660);
        text_color = lv_color_hex(0xA890B8);
    } else if (theme == GUI_VIEW_THEME_TERMINAL) {
        bg_color = lv_color_hex(0x08171B);
        border_color = lv_color_hex(0x12848A);
        text_color = lv_color_hex(0x9BFCFF);
    } else {
        bg_color = lv_color_hex(0xFFFFFF);
        border_color = lv_color_hex(0xD7E1EE);
        text_color = lv_color_hex(0x334155);
    }

    if (is_connected) {
        if (theme == GUI_VIEW_THEME_DARK) {
            bg_color = lv_color_hex(0x153528);
            border_color = lv_color_hex(0x4ADE80);
            text_color = lv_color_hex(0xBBF7D0);
        } else if (theme == GUI_VIEW_THEME_HELLO_KITTY) {
            bg_color = lv_color_hex(0xFFE7EF);
            border_color = lv_color_hex(0xFF6B9A);
            text_color = lv_color_hex(0x8A1D47);
        } else if (theme == GUI_VIEW_THEME_HELLO_KITTY_NIGHT) {
            bg_color = lv_color_hex(0x222040);
            border_color = lv_color_hex(0xD46A80);
            text_color = lv_color_hex(0xE8D8F0);
        } else if (theme == GUI_VIEW_THEME_TERMINAL) {
            bg_color = lv_color_hex(0x0A1B20);
            border_color = lv_color_hex(0x03F5FA);
            text_color = lv_color_hex(0xD8FEFF);
        } else {
            bg_color = lv_color_hex(0xDCFCE7);
            border_color = lv_color_hex(0x22C55E);
            text_color = lv_color_hex(0x14532D);
        }
    } else if (is_known) {
        if (theme == GUI_VIEW_THEME_DARK) {
            bg_color = lv_color_hex(0x162D22);
            border_color = lv_color_hex(0x86EFAC);
            text_color = lv_color_hex(0xD1FAE5);
        } else if (theme == GUI_VIEW_THEME_HELLO_KITTY) {
            bg_color = lv_color_hex(0xFFF0F6);
            border_color = lv_color_hex(0xF9A8C4);
            text_color = lv_color_hex(0x9D174D);
        } else if (theme == GUI_VIEW_THEME_HELLO_KITTY_NIGHT) {
            bg_color = lv_color_hex(0x222040);
            border_color = lv_color_hex(0xC97090);
            text_color = lv_color_hex(0xE8D8F0);
        } else if (theme == GUI_VIEW_THEME_TERMINAL) {
            bg_color = lv_color_hex(0x061419);
            border_color = lv_color_hex(0x0E5E63);
            text_color = lv_color_hex(0x9BFCFF);
        } else {
            bg_color = lv_color_hex(0xF0FDF4);
            border_color = lv_color_hex(0x86EFAC);
            text_color = lv_color_hex(0x166534);
        }
    } else if (is_selected) {
        if (theme == GUI_VIEW_THEME_DARK) {
            bg_color = lv_color_hex(0x1A2B45);
            border_color = lv_color_hex(0x60A5FA);
            text_color = lv_color_hex(0xBFDBFE);
        } else if (theme == GUI_VIEW_THEME_HELLO_KITTY) {
            bg_color = lv_color_hex(0xFFE2EC);
            border_color = lv_color_hex(0xFB7185);
            text_color = lv_color_hex(0x881337);
        } else if (theme == GUI_VIEW_THEME_HELLO_KITTY_NIGHT) {
            bg_color = lv_color_hex(0x2A2744);
            border_color = lv_color_hex(0xD46A80);
            text_color = lv_color_hex(0xE8D8F0);
        } else if (theme == GUI_VIEW_THEME_TERMINAL) {
            bg_color = lv_color_hex(0x0B2127);
            border_color = lv_color_hex(0x03F5FA);
            text_color = lv_color_hex(0xD8FEFF);
        } else {
            bg_color = lv_color_hex(0xE6F0FF);
            border_color = lv_color_hex(0x7CA6F8);
            text_color = lv_color_hex(0x123364);
        }
    }

    lv_obj_set_style_radius(button, 14, 0);
    lv_obj_set_style_shadow_width(button, 0, 0);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(button, bg_color, 0);
    lv_obj_set_style_border_color(button, border_color, 0);
    lv_obj_set_style_text_color(button, text_color, 0);
}

bool gui_view_wifi_is_connected(const gui_view_model_t *model, const char *ssid)
{
    if ((model == NULL) || (ssid == NULL) || (ssid[0] == '\0')) {
        return false;
    }

    return (model->wifi.state == GUI_WIFI_STATE_CONNECTED) &&
           (strcmp(model->wifi.selected_ssid, ssid) == 0);
}

int8_t gui_view_find_known_network_index(const gui_view_model_t *model, const char *ssid)
{
    uint8_t network_index;

    if ((model == NULL) || (ssid == NULL)) {
        return -1;
    }

    for (network_index = 0; network_index < model->wifi.known_network_count; network_index++) {
        if (strcmp(model->wifi.known_networks[network_index].ssid, ssid) == 0) {
            return (int8_t)network_index;
        }
    }

    return -1;
}

lv_obj_t *gui_view_create_action_button(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                                        lv_coord_t width, const char *text,
                                        lv_event_code_t event_code, lv_event_cb_t event_cb,
                                        void *event_user_data)
{
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_set_size(button, width, 40);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_style_radius(button, 14, 0);
    lv_obj_set_style_shadow_width(button, 0, 0);
    lv_obj_add_event_cb(button, event_cb, event_code, event_user_data);

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return button;
}

lv_obj_t *gui_view_create_legend_item(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                                      lv_coord_t width, lv_color_t color,
                                      const char *text)
{
    lv_obj_t *item = lv_obj_create(parent);
    lv_obj_set_size(item, width, 20);
    lv_obj_set_pos(item, x, y);
    lv_obj_set_style_bg_opa(item, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(item, 0, 0);
    lv_obj_set_style_pad_all(item, 0, 0);
    lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *dot = lv_obj_create(item);
    lv_obj_set_size(dot, 12, 12);
    lv_obj_align(dot, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, color, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_pad_all(dot, 0, 0);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_label_create(item);
    lv_label_set_text(label, text);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, width - 18);
    lv_obj_set_style_text_color(label, lv_color_hex(0x4A5C78), 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 18, 0);

    return dot;
}

lv_obj_t *gui_view_create_metric_card(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                                      const char *label_text, lv_obj_t **value_label)
{
    (void)x;
    (void)y;

    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 176, 168);
    lv_obj_set_style_radius(card, 22, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xD9E3F1), 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xF8FBFF), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(card, 18, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_label_create(card);
    lv_label_set_text(label, label_text);
    lv_obj_set_style_text_color(label, lv_color_hex(0x52637F), 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);

    *value_label = lv_label_create(card);
    lv_obj_set_style_text_color(*value_label, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_text_font(*value_label, &lv_font_montserrat_18, 0);
    lv_obj_align(*value_label, LV_ALIGN_TOP_LEFT, 0, 48);

    return card;
}

void gui_view_apply_energy_series(lv_obj_t *chart, lv_chart_series_t *series,
                                  const uint16_t *values)
{
    uint16_t point_index;

    if ((chart == NULL) || (series == NULL) || (values == NULL)) {
        return;
    }

    for (point_index = 0; point_index < GUI_ENERGY_PLAN_POINT_COUNT; point_index++) {
        lv_chart_set_value_by_id(chart, series, point_index, (lv_coord_t)values[point_index]);
    }
}