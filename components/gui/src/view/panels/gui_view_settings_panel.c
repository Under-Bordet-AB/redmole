#include "gui_view_settings_panel.h"

#include <stdio.h>
#include <string.h>

#include "../gui_view_common.h"

static bool gui_view_wifi_network_equals(const gui_wifi_network_t *left,
                                         const gui_wifi_network_t *right)
{
    if ((left == NULL) || (right == NULL)) {
        return false;
    }

    return (strcmp(left->ssid, right->ssid) == 0) &&
           (left->signal_strength_pct == right->signal_strength_pct) &&
           (left->secured == right->secured);
}

static bool gui_view_wifi_settings_changed(gui_view_t *view, const gui_wifi_settings_t *wifi)
{
    const gui_wifi_settings_t *last_wifi;
    uint8_t index;

    if ((view == NULL) || (wifi == NULL)) {
        return false;
    }

    if (!view->has_last_wifi_settings) {
        return true;
    }

    last_wifi = &view->last_wifi_settings;
    if ((last_wifi->network_count != wifi->network_count) ||
        (last_wifi->known_network_count != wifi->known_network_count) ||
        (last_wifi->selected_network_index != wifi->selected_network_index) ||
        (last_wifi->selected_known_network_index != wifi->selected_known_network_index) ||
        (last_wifi->state != wifi->state) ||
        (strcmp(last_wifi->selected_ssid, wifi->selected_ssid) != 0) ||
        (strcmp(last_wifi->password, wifi->password) != 0) ||
        (strcmp(last_wifi->status_text, wifi->status_text) != 0)) {
        return true;
    }

    for (index = 0; index < GUI_WIFI_NETWORK_COUNT; index++) {
        if (!gui_view_wifi_network_equals(&last_wifi->networks[index], &wifi->networks[index])) {
            return true;
        }
    }

    for (index = 0; index < GUI_WIFI_KNOWN_NETWORK_COUNT; index++) {
        if (!gui_view_wifi_network_equals(&last_wifi->known_networks[index],
                                          &wifi->known_networks[index])) {
            return true;
        }
    }

    return false;
}

static bool gui_view_appearance_settings_changed(gui_view_t *view,
                                                 const gui_appearance_settings_t *appearance)
{
    bool background_enabled;

    if ((view == NULL) || (appearance == NULL)) {
        return false;
    }

    if ((view->theme_dropdown == NULL) || (view->theme_background_switch == NULL)) {
        return true;
    }

    if (lv_dropdown_get_selected(view->theme_dropdown) != (uint16_t)appearance->theme) {
        return true;
    }

    background_enabled = lv_obj_has_state(view->theme_background_switch, LV_STATE_CHECKED);
    return background_enabled != appearance->show_background_image;
}

static lv_obj_t *gui_view_create_settings_card(lv_obj_t *parent, const char *title_text,
                                               const char *subtitle_text, lv_coord_t height)
{
    lv_obj_t *card = lv_obj_create(parent);

    lv_obj_set_width(card, LV_PCT(100));
    lv_obj_set_height(card, height);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_radius(card, 22, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xD7E1EE), 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_style_pad_top(card, 18, 0);
    lv_obj_set_style_pad_left(card, 18, 0);
    lv_obj_set_style_pad_right(card, 18, 0);
    lv_obj_set_style_pad_bottom(card, 18, 0);
    lv_obj_set_style_pad_row(card, 8, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, title_text);
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_color(title, lv_color_hex(0x10213D), 0);

    lv_obj_t *subtitle = lv_label_create(card);
    lv_label_set_text(subtitle, subtitle_text);
    lv_obj_set_width(subtitle, LV_PCT(100));
    lv_label_set_long_mode(subtitle, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x607089), 0);

    return card;
}

static lv_obj_t *gui_view_create_setting_item_card(lv_obj_t *parent, const char *title_text,
                                                   const char *subtitle_text,
                                                   lv_coord_t height)
{
    lv_obj_t *card = lv_obj_create(parent);

    lv_obj_set_width(card, LV_PCT(100));
    lv_obj_set_height(card, height);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_radius(card, 18, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xF8FBFF), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xD9E3F1), 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_style_pad_top(card, 14, 0);
    lv_obj_set_style_pad_left(card, 14, 0);
    lv_obj_set_style_pad_right(card, 14, 0);
    lv_obj_set_style_pad_bottom(card, 14, 0);
    lv_obj_set_style_pad_row(card, 10, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, title_text);
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_color(title, lv_color_hex(0x10213D), 0);

    lv_obj_t *subtitle = lv_label_create(card);
    lv_label_set_text(subtitle, subtitle_text);
    lv_obj_set_width(subtitle, LV_PCT(100));
    lv_label_set_long_mode(subtitle, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x607089), 0);

    return card;
}

static lv_obj_t *gui_view_create_settings_page(lv_obj_t *parent)
{
    lv_obj_t *page = lv_obj_create(parent);

    lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));
    lv_obj_center(page);
    lv_obj_set_style_bg_opa(page, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_shadow_width(page, 0, 0);
    lv_obj_set_style_radius(page, 0, 0);
    lv_obj_set_style_pad_all(page, 0, 0);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);

    return page;
}

void gui_view_hide_wifi_dialogs_impl(gui_view_t *view)
{
    if (view == NULL) {
        return;
    }

    if (view->dialog_scrim != NULL) {
        lv_obj_add_flag(view->dialog_scrim, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_clear_flag(view->settings_home_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(view->network_dialog, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(view->password_dialog, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(view->wifi_keyboard, NULL);
    lv_obj_add_flag(view->wifi_keyboard, LV_OBJ_FLAG_HIDDEN);
}

void gui_view_show_network_dialog_impl(gui_view_t *view)
{
    if (view == NULL) {
        return;
    }

    if (view->dialog_scrim != NULL) {
        lv_obj_add_flag(view->dialog_scrim, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_add_flag(view->settings_home_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(view->network_dialog, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(view->password_dialog, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(view->wifi_keyboard, NULL);
    lv_obj_add_flag(view->wifi_keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(view->network_dialog);
}

void gui_view_show_password_dialog_impl(gui_view_t *view)
{
    if (view == NULL) {
        return;
    }

    if (view->dialog_scrim != NULL) {
        lv_obj_add_flag(view->dialog_scrim, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_add_flag(view->settings_home_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(view->network_dialog, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(view->password_dialog, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_state(view->wifi_password_textarea, LV_STATE_FOCUSED);
    lv_keyboard_set_textarea(view->wifi_keyboard, view->wifi_password_textarea);
    lv_obj_clear_flag(view->wifi_keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(view->password_dialog);
    lv_obj_move_foreground(view->wifi_keyboard);
}

void gui_view_init_settings_panel(gui_view_t *view, lv_event_cb_t settings_event_cb,
                                  void *event_user_data)
{
    lv_obj_t *settings_grid;
    lv_obj_t *connectivity_card;
    lv_obj_t *connectivity_stack;
    lv_obj_t *other_settings_card;
    lv_obj_t *other_settings_stack;
    lv_obj_t *wifi_card;
    lv_obj_t *bluetooth_card;
    lv_obj_t *brightness_card;
    lv_obj_t *theme_card;
    lv_obj_t *bluetooth_status;
    lv_obj_t *settings_text;
    lv_obj_t *dialog_title;
    uint8_t network_index;

    if (view == NULL) {
        return;
    }

    view->settings_panel = lv_obj_create(view->content);
    lv_obj_set_size(view->settings_panel, 734, 500);
    lv_obj_center(view->settings_panel);
    lv_obj_set_style_radius(view->settings_panel, 0, 0);
    lv_obj_set_style_bg_opa(view->settings_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(view->settings_panel, 0, 0);
    lv_obj_set_style_shadow_width(view->settings_panel, 0, 0);
    lv_obj_clear_flag(view->settings_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(view->settings_panel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(view->settings_panel, 0, 0);
    lv_obj_set_style_pad_row(view->settings_panel, 0, 0);

    view->settings_home_panel = gui_view_create_settings_page(view->settings_panel);

    settings_grid = lv_obj_create(view->settings_home_panel);
    lv_obj_set_size(settings_grid, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(settings_grid, 0, 0);
    lv_obj_set_style_bg_opa(settings_grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(settings_grid, 0, 0);
    lv_obj_set_style_shadow_width(settings_grid, 0, 0);
    lv_obj_set_style_pad_all(settings_grid, 0, 0);
    lv_obj_set_style_pad_column(settings_grid, 14, 0);
    lv_obj_set_style_pad_row(settings_grid, 0, 0);
    lv_obj_clear_flag(settings_grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(settings_grid, LV_LAYOUT_GRID);
    {
        static const lv_coord_t grid_columns[] = {LV_GRID_FR(1), LV_GRID_FR(1),
                              LV_GRID_TEMPLATE_LAST};
        static const lv_coord_t grid_rows[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        lv_obj_set_style_grid_column_dsc_array(settings_grid, grid_columns, 0);
        lv_obj_set_style_grid_row_dsc_array(settings_grid, grid_rows, 0);
    }

    connectivity_card = gui_view_create_settings_card(settings_grid, "Connectivity",
                                                      "Manage how Redmole connects to nearby devices and networks.",
                                                      LV_SIZE_CONTENT);
    view->connectivity_card = connectivity_card;
    lv_obj_set_grid_cell(connectivity_card, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_START, 0,
                         1);

    other_settings_card = gui_view_create_settings_card(
        settings_grid, "Other settings",
        "Display and system controls for the device.", LV_SIZE_CONTENT);
    view->other_settings_card = other_settings_card;
    lv_obj_set_grid_cell(other_settings_card, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_START,
                         0, 1);

    connectivity_stack = lv_obj_create(connectivity_card);
    lv_obj_set_size(connectivity_stack, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(connectivity_stack, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(connectivity_stack, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(connectivity_stack, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_opa(connectivity_stack, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(connectivity_stack, 0, 0);
    lv_obj_set_style_shadow_width(connectivity_stack, 0, 0);
    lv_obj_set_style_pad_all(connectivity_stack, 0, 0);
    lv_obj_set_style_pad_row(connectivity_stack, 12, 0);
    lv_obj_clear_flag(connectivity_stack, LV_OBJ_FLAG_SCROLLABLE);

    wifi_card = gui_view_create_setting_item_card(connectivity_stack, "Wi-Fi",
                                                  "Scan to find networks, then choose one to connect.",
                                                  LV_SIZE_CONTENT);
    view->wifi_card = wifi_card;

    view->wifi_status_dot = lv_obj_create(wifi_card);
    lv_obj_set_size(view->wifi_status_dot, 10, 10);
    lv_obj_add_flag(view->wifi_status_dot, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(view->wifi_status_dot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(view->wifi_status_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(view->wifi_status_dot, 0, 0);
    lv_obj_set_style_shadow_width(view->wifi_status_dot, 0, 0);
    lv_obj_set_style_bg_opa(view->wifi_status_dot, LV_OPA_COVER, 0);
    lv_obj_align(view->wifi_status_dot, LV_ALIGN_TOP_RIGHT, -4, 4);

    view->scan_button = gui_view_create_action_button(wifi_card, 0, 0, 120, "Scan",
                                                      LV_EVENT_CLICKED, settings_event_cb,
                                                      event_user_data);

    bluetooth_card = gui_view_create_setting_item_card(
        connectivity_stack, "Bluetooth",
        "Reserved for nearby device discovery and pairing.", LV_SIZE_CONTENT);
    view->bluetooth_card = bluetooth_card;

    bluetooth_status = lv_label_create(bluetooth_card);
    view->bluetooth_status_label = bluetooth_status;
    lv_label_set_text(bluetooth_status, "Coming later.");
    lv_obj_set_width(bluetooth_status, LV_PCT(100));
    lv_obj_set_style_text_color(bluetooth_status, lv_color_hex(0x4A5C78), 0);

    other_settings_stack = lv_obj_create(other_settings_card);
    lv_obj_set_size(other_settings_stack, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(other_settings_stack, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(other_settings_stack, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(other_settings_stack, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_opa(other_settings_stack, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(other_settings_stack, 0, 0);
    lv_obj_set_style_shadow_width(other_settings_stack, 0, 0);
    lv_obj_set_style_pad_all(other_settings_stack, 0, 0);
    lv_obj_set_style_pad_row(other_settings_stack, 12, 0);
    lv_obj_clear_flag(other_settings_stack, LV_OBJ_FLAG_SCROLLABLE);

    brightness_card = gui_view_create_setting_item_card(
        other_settings_stack, "Screen brightness",
        "Adjust the display backlight level for readability and power use.",
        LV_SIZE_CONTENT);
    view->brightness_card = brightness_card;

    lv_obj_t *brightness_row = lv_obj_create(brightness_card);
    lv_obj_set_width(brightness_row, LV_PCT(100));
    lv_obj_set_height(brightness_row, LV_SIZE_CONTENT);
    lv_obj_set_layout(brightness_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(brightness_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(brightness_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_opa(brightness_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(brightness_row, 0, 0);
    lv_obj_set_style_shadow_width(brightness_row, 0, 0);
    lv_obj_set_style_pad_all(brightness_row, 0, 0);
    lv_obj_set_style_pad_column(brightness_row, 12, 0);
    lv_obj_clear_flag(brightness_row, LV_OBJ_FLAG_SCROLLABLE);

    view->brightness_slider = lv_slider_create(brightness_row);
    lv_obj_set_height(view->brightness_slider, 18);
    lv_obj_set_flex_grow(view->brightness_slider, 1);
    lv_slider_set_range(view->brightness_slider, 5, 100);
    lv_obj_add_event_cb(view->brightness_slider, settings_event_cb, LV_EVENT_VALUE_CHANGED,
                        event_user_data);
    lv_obj_set_style_bg_color(view->brightness_slider, lv_color_hex(0xD9E3F1), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(view->brightness_slider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(view->brightness_slider, lv_color_hex(0x1D4ED8),
                              LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(view->brightness_slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(view->brightness_slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(view->brightness_slider, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_border_width(view->brightness_slider, 2, LV_PART_KNOB);
    lv_obj_set_style_border_color(view->brightness_slider, lv_color_hex(0x1D4ED8), LV_PART_KNOB);
    lv_obj_set_style_pad_all(view->brightness_slider, 4, LV_PART_KNOB);

    view->brightness_value_label = lv_label_create(brightness_row);
    lv_label_set_text(view->brightness_value_label, "82%");
    lv_obj_set_style_text_color(view->brightness_value_label, lv_color_hex(0x1D4ED8), 0);
    lv_obj_set_style_text_font(view->brightness_value_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_align(view->brightness_value_label, LV_TEXT_ALIGN_RIGHT, 0);

    theme_card = gui_view_create_setting_item_card(
        other_settings_stack, "Theme",
        "Choose how the interface should look.", LV_SIZE_CONTENT);
    view->theme_card = theme_card;

    view->theme_dropdown = lv_dropdown_create(theme_card);
    lv_obj_set_size(view->theme_dropdown, LV_PCT(100), 46);
    lv_dropdown_set_options(view->theme_dropdown,
                            "Light mode\nDark mode\nHello Kitty\nTerminal");
    lv_dropdown_set_selected(view->theme_dropdown, 0);
    lv_obj_add_event_cb(view->theme_dropdown, settings_event_cb, LV_EVENT_VALUE_CHANGED,
                        event_user_data);
    lv_obj_set_style_radius(view->theme_dropdown, 14, 0);
    lv_obj_set_style_shadow_width(view->theme_dropdown, 0, 0);
    lv_obj_set_style_border_width(view->theme_dropdown, 1, 0);
    lv_obj_set_style_border_color(view->theme_dropdown, lv_color_hex(0xD7E1EE), 0);
    lv_obj_set_style_bg_color(view->theme_dropdown, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(view->theme_dropdown, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(view->theme_dropdown, lv_color_hex(0x10213D), 0);

    lv_obj_t *theme_background_row = lv_obj_create(theme_card);
    lv_obj_set_width(theme_background_row, LV_PCT(100));
    lv_obj_set_height(theme_background_row, LV_SIZE_CONTENT);
    lv_obj_set_layout(theme_background_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(theme_background_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(theme_background_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_opa(theme_background_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(theme_background_row, 0, 0);
    lv_obj_set_style_shadow_width(theme_background_row, 0, 0);
    lv_obj_set_style_pad_all(theme_background_row, 0, 0);
    lv_obj_set_style_pad_column(theme_background_row, 12, 0);
    lv_obj_clear_flag(theme_background_row, LV_OBJ_FLAG_SCROLLABLE);

    view->theme_background_label = lv_label_create(theme_background_row);
    lv_label_set_text(view->theme_background_label, "Show background image");
    lv_obj_set_width(view->theme_background_label, LV_PCT(100));
    lv_obj_set_style_text_color(view->theme_background_label, lv_color_hex(0x10213D), 0);
    lv_obj_set_style_text_align(view->theme_background_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_flex_grow(view->theme_background_label, 1);

    view->theme_background_switch = lv_switch_create(theme_background_row);
    lv_obj_add_state(view->theme_background_switch, LV_STATE_CHECKED);
    lv_obj_add_event_cb(view->theme_background_switch, settings_event_cb, LV_EVENT_VALUE_CHANGED,
                        event_user_data);
    lv_obj_set_style_bg_color(view->theme_background_switch, lv_color_hex(0xD9E3F1),
                              LV_PART_MAIN);
    lv_obj_set_style_bg_opa(view->theme_background_switch, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(view->theme_background_switch, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(view->theme_background_switch, lv_color_hex(0xD7E1EE),
                                  LV_PART_MAIN);
    lv_obj_set_style_bg_color(view->theme_background_switch, lv_color_hex(0x1D4ED8),
                              LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(view->theme_background_switch, LV_OPA_COVER,
                            LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_border_width(view->theme_background_switch, 0,
                                  LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(view->theme_background_switch, lv_color_hex(0xFFFFFF),
                              LV_PART_KNOB);
    lv_obj_set_style_bg_opa(view->theme_background_switch, LV_OPA_COVER, LV_PART_KNOB);

    view->dialog_scrim = NULL;

    view->network_dialog = gui_view_create_settings_page(view->settings_panel);
    lv_obj_set_style_pad_top(view->network_dialog, 4, 0);
    lv_obj_set_style_radius(view->network_dialog, 26, 0);
    lv_obj_set_style_bg_color(view->network_dialog, lv_color_hex(0xF8FBFF), 0);
    lv_obj_set_style_bg_opa(view->network_dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(view->network_dialog, 0, 0);

    dialog_title = lv_label_create(view->network_dialog);
    view->network_dialog_title = dialog_title;
    lv_obj_set_width(dialog_title, 662);
    lv_label_set_text(dialog_title, "Finding Networks");
    lv_obj_set_style_text_color(dialog_title, lv_color_hex(0x10213D), 0);
    lv_obj_set_style_text_align(dialog_title, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(dialog_title, LV_ALIGN_TOP_MID, 0, 8);

    settings_text = lv_label_create(view->network_dialog);
    view->network_dialog_subtitle = settings_text;
    lv_obj_set_width(settings_text, 662);
    lv_label_set_long_mode(settings_text, LV_LABEL_LONG_WRAP);
    lv_label_set_text(settings_text, "Only the scan results are shown here. Pick a network to continue.");
    lv_obj_set_style_text_color(settings_text, lv_color_hex(0x607089), 0);
    lv_obj_set_style_text_align(settings_text, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(settings_text, LV_ALIGN_TOP_MID, 0, 34);

    for (network_index = 0; network_index < GUI_WIFI_NETWORK_COUNT; network_index++) {
        view->network_dialog_buttons[network_index] = lv_btn_create(view->network_dialog);
        lv_obj_set_size(view->network_dialog_buttons[network_index], 662, 54);
        lv_obj_align(view->network_dialog_buttons[network_index], LV_ALIGN_TOP_MID, 0,
                     88 + ((lv_coord_t)network_index * 64));
        lv_obj_add_event_cb(view->network_dialog_buttons[network_index], settings_event_cb,
                            LV_EVENT_CLICKED, event_user_data);
        gui_view_style_scanned_wifi_button(view->network_dialog_buttons[network_index],
                           view->current_theme, false, false, false);
        view->network_dialog_button_labels[network_index] =
            lv_label_create(view->network_dialog_buttons[network_index]);
        lv_obj_center(view->network_dialog_button_labels[network_index]);
    }

    view->network_empty_label = lv_label_create(view->network_dialog);
    lv_obj_set_width(view->network_empty_label, 662);
    lv_label_set_long_mode(view->network_empty_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(view->network_empty_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(view->network_empty_label, "Scanning for Wi-Fi networks...");
    lv_obj_set_style_text_color(view->network_empty_label, lv_color_hex(0x607089), 0);
    lv_obj_align(view->network_empty_label, LV_ALIGN_CENTER, 0, 0);

    view->network_dialog_cancel_button = gui_view_create_action_button(
        view->network_dialog, 562, 444, 136, "Back", LV_EVENT_CLICKED, settings_event_cb,
        event_user_data);

    view->password_dialog = gui_view_create_settings_page(view->settings_panel);
    lv_obj_set_style_pad_top(view->password_dialog, 4, 0);
    lv_obj_set_style_radius(view->password_dialog, 26, 0);
    lv_obj_set_style_bg_color(view->password_dialog, lv_color_hex(0xF8FBFF), 0);
    lv_obj_set_style_bg_opa(view->password_dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(view->password_dialog, 0, 0);

    dialog_title = lv_label_create(view->password_dialog);
    view->password_dialog_title = dialog_title;
    lv_obj_set_width(dialog_title, 662);
    lv_label_set_text(dialog_title, "Enter Password");
    lv_obj_set_style_text_color(dialog_title, lv_color_hex(0x10213D), 0);
    lv_obj_set_style_text_align(dialog_title, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(dialog_title, LV_ALIGN_TOP_MID, 0, 8);

    view->password_dialog_network_label = lv_label_create(view->password_dialog);
    lv_obj_set_width(view->password_dialog_network_label, 662);
    lv_label_set_long_mode(view->password_dialog_network_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(view->password_dialog_network_label, lv_color_hex(0x607089), 0);
    lv_obj_set_style_text_font(view->password_dialog_network_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(view->password_dialog_network_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(view->password_dialog_network_label, LV_ALIGN_TOP_MID, 0, 34);

    view->wifi_password_textarea = lv_textarea_create(view->password_dialog);
    lv_obj_set_size(view->wifi_password_textarea, 662, 52);
    lv_obj_align(view->wifi_password_textarea, LV_ALIGN_TOP_MID, 0, 78);
    lv_textarea_set_one_line(view->wifi_password_textarea, true);
    lv_textarea_set_password_mode(view->wifi_password_textarea, true);
    lv_textarea_set_placeholder_text(view->wifi_password_textarea, "Enter Wi-Fi password");
    lv_obj_add_event_cb(view->wifi_password_textarea, settings_event_cb, LV_EVENT_ALL,
                        event_user_data);

    view->wifi_keyboard = lv_keyboard_create(view->password_dialog);
    lv_obj_set_size(view->wifi_keyboard, LV_PCT(100), 292);
    lv_obj_align(view->wifi_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_radius(view->wifi_keyboard, 18, 0);
    lv_obj_set_style_bg_color(view->wifi_keyboard, lv_color_hex(0xE7EDF5), 0);
    lv_obj_set_style_bg_opa(view->wifi_keyboard, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(view->wifi_keyboard, 0, 0);
    lv_obj_set_style_border_width(view->wifi_keyboard, 1, 0);
    lv_obj_set_style_border_color(view->wifi_keyboard, lv_color_hex(0xD7E1EE), 0);
    lv_obj_set_style_text_font(view->wifi_keyboard, &lv_font_montserrat_24, LV_PART_ITEMS);

    view->password_dialog_cancel_button = gui_view_create_action_button(
        view->password_dialog, 426, 146, 128, "Back", LV_EVENT_CLICKED, settings_event_cb,
        event_user_data);
    view->password_dialog_connect_button = gui_view_create_action_button(
        view->password_dialog, 570, 146, 132, "Connect", LV_EVENT_CLICKED, settings_event_cb,
        event_user_data);
    lv_obj_set_style_bg_color(view->password_dialog_connect_button, lv_color_hex(0x1D4ED8), 0);
    lv_obj_set_style_text_color(view->password_dialog_connect_button, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(view->password_dialog_connect_button, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(view->password_dialog_connect_button, 0, 0);

    gui_view_hide_wifi_dialogs_impl(view);
}

void gui_view_apply_settings_panel(gui_view_t *view, const gui_view_model_t *model)
{
    char wifi_text[72];
    const char *network_empty_text = "No networks found yet.";
    bool wifi_changed;
    uint8_t network_index;

    if ((view == NULL) || (model == NULL)) {
        return;
    }

    wifi_changed = gui_view_wifi_settings_changed(view, &model->wifi);
    if (!wifi_changed && !gui_view_appearance_settings_changed(view, &model->appearance)) {
        return;
    }

    if ((view->theme_dropdown != NULL) &&
        (lv_dropdown_get_selected(view->theme_dropdown) != (uint16_t)model->appearance.theme)) {
        lv_dropdown_set_selected(view->theme_dropdown, (uint16_t)model->appearance.theme);
    }

    if (view->theme_background_switch != NULL) {
        bool background_enabled = lv_obj_has_state(view->theme_background_switch, LV_STATE_CHECKED);

        if (background_enabled != model->appearance.show_background_image) {
            if (model->appearance.show_background_image) {
                lv_obj_add_state(view->theme_background_switch, LV_STATE_CHECKED);
            } else {
                lv_obj_clear_state(view->theme_background_switch, LV_STATE_CHECKED);
            }
        }
    }

    if (view->wifi_status_dot != NULL) {
        lv_obj_set_style_bg_color(view->wifi_status_dot,
                                  gui_view_wifi_status_color(view->current_theme,
                                                             model->wifi.state),
                                  0);
    }

    gui_view_set_label_text_if_changed(view->wifi_status_label, "");
    if (model->wifi.selected_ssid[0] != '\0') {
        gui_view_set_label_text_if_changed(view->password_dialog_network_label,
                                           model->wifi.selected_ssid);
    } else if ((model->wifi.selected_network_index >= 0) &&
               (model->wifi.selected_network_index < (int8_t)model->wifi.network_count)) {
        gui_view_set_label_text_if_changed(
            view->password_dialog_network_label,
            model->wifi.networks[(uint8_t)model->wifi.selected_network_index].ssid);
    } else {
        gui_view_set_label_text_if_changed(view->password_dialog_network_label,
                                           "No network selected yet.");
    }
    if (!lv_obj_has_state(view->wifi_password_textarea, LV_STATE_FOCUSED)) {
        gui_view_set_textarea_text_if_changed(view->wifi_password_textarea, model->wifi.password);
    }

    if (model->wifi.network_count == 0U) {
        if (model->wifi.status_text[0] != '\0') {
            network_empty_text = model->wifi.status_text;
        }

        gui_view_set_label_text_if_changed(view->network_empty_label, network_empty_text);
        lv_obj_clear_flag(view->network_empty_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(view->network_empty_label, LV_OBJ_FLAG_HIDDEN);
    }

    for (network_index = 0; network_index < GUI_WIFI_NETWORK_COUNT; network_index++) {
        if (network_index < model->wifi.network_count) {
            const char *ssid = model->wifi.networks[network_index].ssid;
            bool is_known = gui_view_find_known_network_index(model, ssid) >= 0;
            bool is_connected = gui_view_wifi_is_connected(model, ssid);

            gui_view_style_scanned_wifi_button(
                view->network_dialog_buttons[network_index], view->current_theme,
                model->wifi.selected_network_index == (int8_t)network_index, is_known,
                is_connected);

            if (is_connected) {
                snprintf(wifi_text, sizeof(wifi_text), "%s  |  Connected", ssid);
            } else if (is_known) {
                snprintf(wifi_text, sizeof(wifi_text), "%s  |  Saved network", ssid);
            } else if (model->wifi.networks[network_index].secured) {
                snprintf(wifi_text, sizeof(wifi_text), "%s  |  %u%% signal  |  Secured", ssid,
                         model->wifi.networks[network_index].signal_strength_pct);
            } else {
                snprintf(wifi_text, sizeof(wifi_text), "%s  |  %u%% signal  |  Open", ssid,
                         model->wifi.networks[network_index].signal_strength_pct);
            }

            gui_view_set_label_text_if_changed(view->network_dialog_button_labels[network_index],
                                               wifi_text);
            lv_obj_clear_flag(view->network_dialog_buttons[network_index], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(view->network_dialog_buttons[network_index], LV_OBJ_FLAG_HIDDEN);
        }
    }

    view->last_wifi_settings = model->wifi;
    view->has_last_wifi_settings = true;
}