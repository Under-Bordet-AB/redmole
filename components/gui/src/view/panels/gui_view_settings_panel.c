/**
 * @file gui_view_settings_panel.c
 * @brief LVGL widgets, dialogs, and cached updates for GUI settings.
 */

#include "gui_view_settings_panel.h"

#include <stdio.h>
#include <string.h>

#include "../gui_view_common.h"
#include "../gui_theme_defs.h"

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
        (last_wifi->connect_requested != wifi->connect_requested) ||
        (last_wifi->can_disconnect != wifi->can_disconnect) ||
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
    bool desired_night_enabled;
    bool night_enabled;
    bool night_switch_disabled;
    bool should_disable_night_switch;
    const gui_theme_def_t *theme_def;
    gui_view_theme_t resolved_theme;
    uint16_t theme_dropdown_index;

    if ((view == NULL) || (appearance == NULL)) {
        return false;
    }

    if ((view->theme_dropdown == NULL) || (view->theme_background_switch == NULL) ||
        (view->theme_night_switch == NULL)) {
        return true;
    }

    resolved_theme = gui_theme_resolve_available(appearance->theme);
    theme_def = gui_theme_get(resolved_theme);

    if (gui_theme_theme_to_dropdown_index(resolved_theme, &theme_dropdown_index) &&
        (lv_dropdown_get_selected(view->theme_dropdown) != theme_dropdown_index)) {
        return true;
    }

    background_enabled = lv_obj_has_state(view->theme_background_switch, LV_STATE_CHECKED);
    if (background_enabled != appearance->show_background_image) {
        return true;
    }

    desired_night_enabled = appearance->night_variant_enabled && (theme_def != NULL) &&
                            theme_def->has_night_variant;
    night_enabled = lv_obj_has_state(view->theme_night_switch, LV_STATE_CHECKED);
    if (night_enabled != desired_night_enabled) {
        return true;
    }

    night_switch_disabled = lv_obj_has_state(view->theme_night_switch, LV_STATE_DISABLED);
    should_disable_night_switch = (theme_def == NULL) || !theme_def->has_night_variant;
    return night_switch_disabled != should_disable_night_switch;
}

static bool gui_view_location_settings_changed(gui_view_t *view,
                                               const gui_location_settings_t *location)
{
    if ((view == NULL) || (location == NULL)) {
        return false;
    }

    if (!view->has_last_location_settings) {
        return true;
    }

    return (strcmp(view->last_location_settings.latitude, location->latitude) != 0) ||
           (strcmp(view->last_location_settings.longitude, location->longitude) != 0);
}

static uint16_t gui_view_spot_price_area_dropdown_index(gui_spot_price_area_t area)
{
    switch (area) {
        case GUI_SPOT_PRICE_AREA_SE1:
            return 0U;
        case GUI_SPOT_PRICE_AREA_SE2:
            return 1U;
        case GUI_SPOT_PRICE_AREA_SE3:
            return 2U;
        case GUI_SPOT_PRICE_AREA_SE4:
            return 3U;
        default:
            return 2U;
    }
}

static bool gui_view_spot_price_area_changed(gui_view_t *view,
                                             gui_spot_price_area_t area)
{
    if (view == NULL) {
        return false;
    }

    return !view->has_last_spot_price_area ||
           (view->last_spot_price_area != area);
}

static const char *gui_view_wifi_card_status_text(const gui_wifi_settings_t *wifi)
{
    if (wifi == NULL) {
        return "Not connected";
    }

    switch (wifi->state) {
        case GUI_WIFI_STATE_CONNECTED:
            if (wifi->selected_ssid[0] != '\0') {
                return wifi->selected_ssid;
            }
            return "Not connected";
        case GUI_WIFI_STATE_CONNECTING:
            return "Connecting";
        case GUI_WIFI_STATE_FAILED:
            return "Failed";
        case GUI_WIFI_STATE_IDLE:
        case GUI_WIFI_STATE_SCANNED:
        default:
            return "Not connected";
    }
}

static const char *gui_view_wifi_dialog_status_text(const gui_wifi_settings_t *wifi)
{
    if (wifi == NULL) {
        return "";
    }

    if (wifi->status_text[0] != '\0') {
        return wifi->status_text;
    }

    if (wifi->state == GUI_WIFI_STATE_CONNECTING) {
        return "Connecting to Wi-Fi...";
    }

    if (wifi->state == GUI_WIFI_STATE_FAILED) {
        return "Wi-Fi connection failed. Check the password and try again.";
    }

    return "";
}

static void gui_view_update_password_dialog_feedback(gui_view_t *view,
                                                     const gui_wifi_settings_t *wifi)
{
    bool is_connecting;
    bool is_failed;
    bool show_status;
    const char *connect_text;
    const char *status_text;
    lv_color_t status_color;

    if ((view == NULL) || (wifi == NULL)) {
        return;
    }

    is_connecting = wifi->state == GUI_WIFI_STATE_CONNECTING;
    is_failed = wifi->state == GUI_WIFI_STATE_FAILED;
    show_status = is_connecting || is_failed;
    connect_text = is_connecting ? "Connecting" : (is_failed ? "Retry" : "Connect");
    status_text = gui_view_wifi_dialog_status_text(wifi);
    status_color = gui_view_wifi_status_color(view->current_theme, wifi->state);

    gui_view_set_label_text_if_changed(view->password_dialog_connect_button_label,
                                       connect_text);

    if (view->password_dialog_status_label != NULL) {
        gui_view_set_label_text_if_changed(view->password_dialog_status_label, status_text);
        lv_obj_set_style_text_color(view->password_dialog_status_label, status_color, 0);
        if (show_status) {
            lv_obj_clear_flag(view->password_dialog_status_label, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(view->password_dialog_status_label, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (view->password_dialog_spinner != NULL) {
        lv_obj_set_style_arc_color(view->password_dialog_spinner, status_color,
                                   LV_PART_INDICATOR);
        if (is_connecting) {
            lv_obj_clear_flag(view->password_dialog_spinner, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(view->password_dialog_spinner, LV_OBJ_FLAG_HIDDEN);
        }
    }
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

static lv_obj_t *gui_view_create_setting_item_card_shell(lv_obj_t *parent, lv_coord_t height)
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

static lv_obj_t *gui_view_create_settings_category_button(lv_obj_t *parent,
                                                          const char *title_text,
                                                          const char *subtitle_text,
                                                          lv_event_cb_t settings_event_cb,
                                                          void *event_user_data)
{
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_t *title;
    lv_obj_t *subtitle;

    lv_obj_set_width(button, LV_PCT(100));
    lv_obj_set_height(button, 112);
    lv_obj_set_layout(button, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(button, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(button, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_radius(button, 22, 0);
    lv_obj_set_style_shadow_width(button, 0, 0);
    lv_obj_set_style_pad_top(button, 18, 0);
    lv_obj_set_style_pad_left(button, 18, 0);
    lv_obj_set_style_pad_right(button, 18, 0);
    lv_obj_set_style_pad_bottom(button, 18, 0);
    lv_obj_set_style_pad_row(button, 8, 0);
    lv_obj_add_event_cb(button, settings_event_cb, LV_EVENT_CLICKED, event_user_data);

    title = lv_label_create(button);
    lv_label_set_text(title, title_text);
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_LEFT, 0);

    subtitle = lv_label_create(button);
    lv_label_set_text(subtitle, subtitle_text);
    lv_obj_set_width(subtitle, LV_PCT(100));
    lv_label_set_long_mode(subtitle, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(subtitle, LV_TEXT_ALIGN_LEFT, 0);

    return button;
}

static lv_obj_t *gui_view_create_settings_subpage_stack(lv_obj_t *parent)
{
    lv_obj_t *stack = lv_obj_create(parent);

    lv_obj_set_size(stack, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(stack, 0, 0);
    lv_obj_set_layout(stack, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(stack, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(stack, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_opa(stack, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(stack, 0, 0);
    lv_obj_set_style_shadow_width(stack, 0, 0);
    lv_obj_set_style_pad_all(stack, 0, 0);
    lv_obj_set_style_pad_row(stack, 14, 0);
    lv_obj_clear_flag(stack, LV_OBJ_FLAG_SCROLLABLE);

    return stack;
}

static lv_obj_t *gui_view_create_settings_card_grid(lv_obj_t *parent)
{
    lv_obj_t *grid = lv_obj_create(parent);

    lv_obj_set_width(grid, LV_PCT(100));
    lv_obj_set_height(grid, 0);
    lv_obj_set_flex_grow(grid, 1);
    lv_obj_set_layout(grid, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_shadow_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_set_style_pad_column(grid, 12, 0);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

    return grid;
}

static void gui_view_set_settings_grid_card(lv_obj_t *card)
{
    if (card == NULL) {
        return;
    }

    lv_obj_set_width(card, 0);
    lv_obj_set_flex_grow(card, 1);
}

static lv_obj_t *gui_view_create_settings_action_row(lv_obj_t *parent)
{
    lv_obj_t *row = lv_obj_create(parent);

    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_shadow_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_pad_column(row, 12, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    return row;
}

static lv_obj_t *gui_view_create_settings_field_row(lv_obj_t *parent, lv_obj_t **label_out,
                                                    lv_obj_t **textarea_out,
                                                    const char *label_text,
                                                    const char *placeholder_text,
                                                    lv_event_cb_t settings_event_cb,
                                                    void *event_user_data)
{
    lv_obj_t *row = lv_obj_create(parent);

    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_shadow_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_pad_row(row, 8, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    *label_out = lv_label_create(row);
    lv_label_set_text(*label_out, label_text);
    lv_obj_set_width(*label_out, LV_PCT(100));
    lv_obj_set_style_text_color(*label_out, lv_color_hex(0x10213D), 0);

    *textarea_out = lv_textarea_create(row);
    lv_obj_set_size(*textarea_out, LV_PCT(100), 48);
    lv_textarea_set_one_line(*textarea_out, true);
    lv_textarea_set_accepted_chars(*textarea_out, "+-0123456789.");
    lv_textarea_set_max_length(*textarea_out, GUI_LOCATION_TEXT_MAX_LEN);
    lv_textarea_set_placeholder_text(*textarea_out, placeholder_text);
    lv_obj_add_event_cb(*textarea_out, settings_event_cb, LV_EVENT_ALL, event_user_data);
    lv_obj_set_style_radius(*textarea_out, 14, 0);
    lv_obj_set_style_shadow_width(*textarea_out, 0, 0);
    lv_obj_set_style_border_width(*textarea_out, 1, 0);
    lv_obj_set_style_border_color(*textarea_out, lv_color_hex(0xD7E1EE), 0);
    lv_obj_set_style_bg_color(*textarea_out, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(*textarea_out, LV_OPA_COVER, 0);

    return row;
}

static void gui_view_set_settings_subpage_visibility(gui_view_t *view,
                                                     gui_settings_subpage_t subpage)
{
    if (view == NULL) {
        return;
    }

    if (view->settings_home_panel != NULL) {
        if (subpage == GUI_SETTINGS_SUBPAGE_HOME) {
            lv_obj_clear_flag(view->settings_home_panel, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(view->settings_home_panel, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (view->settings_connectivity_panel != NULL) {
        if (subpage == GUI_SETTINGS_SUBPAGE_CONNECTIVITY) {
            lv_obj_clear_flag(view->settings_connectivity_panel, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(view->settings_connectivity_panel, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (view->settings_display_panel != NULL) {
        if (subpage == GUI_SETTINGS_SUBPAGE_DISPLAY) {
            lv_obj_clear_flag(view->settings_display_panel, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(view->settings_display_panel, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (view->settings_system_panel != NULL) {
        if (subpage == GUI_SETTINGS_SUBPAGE_SYSTEM) {
            lv_obj_clear_flag(view->settings_system_panel, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(view->settings_system_panel, LV_OBJ_FLAG_HIDDEN);
        }
    }

    view->active_settings_subpage = subpage;
}

static void gui_view_layout_password_dialog_buttons(gui_view_t *view, bool show_disconnect)
{
    if ((view == NULL) || (view->password_dialog_cancel_button == NULL) ||
        (view->password_dialog_connect_button == NULL) ||
        (view->password_dialog_disconnect_button == NULL)) {
        return;
    }

    lv_obj_set_pos(view->password_dialog_connect_button, 570, 148);

    if (show_disconnect) {
        lv_obj_set_pos(view->password_dialog_cancel_button, 282, 148);
        lv_obj_set_pos(view->password_dialog_disconnect_button, 426, 148);
    } else {
        lv_obj_set_pos(view->password_dialog_cancel_button, 426, 148);
        lv_obj_set_pos(view->password_dialog_disconnect_button, 426, 148);
    }
}

void gui_view_hide_wifi_dialogs_impl(gui_view_t *view)
{
    if (view == NULL) {
        return;
    }

    if (view->dialog_scrim != NULL) {
        lv_obj_add_flag(view->dialog_scrim, LV_OBJ_FLAG_HIDDEN);
    }
    gui_view_set_settings_subpage_visibility(view, view->active_settings_subpage);
    lv_obj_add_flag(view->network_dialog, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(view->password_dialog, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(view->wifi_keyboard, NULL);
    lv_obj_add_flag(view->wifi_keyboard, LV_OBJ_FLAG_HIDDEN);
    if (view->location_keyboard != NULL) {
        lv_keyboard_set_textarea(view->location_keyboard, NULL);
        lv_obj_add_flag(view->location_keyboard, LV_OBJ_FLAG_HIDDEN);
    }
}

void gui_view_show_network_dialog_impl(gui_view_t *view)
{
    if (view == NULL) {
        return;
    }

    if (view->dialog_scrim != NULL) {
        lv_obj_add_flag(view->dialog_scrim, LV_OBJ_FLAG_HIDDEN);
    }
    gui_view_set_settings_subpage_visibility(view, GUI_SETTINGS_SUBPAGE_CONNECTIVITY);
    lv_obj_clear_flag(view->network_dialog, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(view->password_dialog, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(view->wifi_keyboard, NULL);
    lv_obj_add_flag(view->wifi_keyboard, LV_OBJ_FLAG_HIDDEN);
    if (view->location_keyboard != NULL) {
        lv_keyboard_set_textarea(view->location_keyboard, NULL);
        lv_obj_add_flag(view->location_keyboard, LV_OBJ_FLAG_HIDDEN);
    }
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
    gui_view_set_settings_subpage_visibility(view, GUI_SETTINGS_SUBPAGE_CONNECTIVITY);
    lv_obj_add_flag(view->network_dialog, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(view->password_dialog, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_state(view->wifi_password_textarea, LV_STATE_FOCUSED);
    lv_keyboard_set_textarea(view->wifi_keyboard, view->wifi_password_textarea);
    lv_obj_clear_flag(view->wifi_keyboard, LV_OBJ_FLAG_HIDDEN);
    if (view->location_keyboard != NULL) {
        lv_keyboard_set_textarea(view->location_keyboard, NULL);
        lv_obj_add_flag(view->location_keyboard, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_move_foreground(view->password_dialog);
    lv_obj_move_foreground(view->wifi_keyboard);
}

void gui_view_show_settings_subpage_impl(gui_view_t *view, gui_settings_subpage_t subpage)
{
    if (view == NULL) {
        return;
    }

    lv_obj_add_flag(view->network_dialog, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(view->password_dialog, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(view->wifi_keyboard, NULL);
    lv_obj_add_flag(view->wifi_keyboard, LV_OBJ_FLAG_HIDDEN);
    if (view->location_keyboard != NULL) {
        lv_keyboard_set_textarea(view->location_keyboard, NULL);
        lv_obj_add_flag(view->location_keyboard, LV_OBJ_FLAG_HIDDEN);
    }
    gui_view_set_settings_subpage_visibility(view, subpage);
}

void gui_view_reset_settings_navigation_impl(gui_view_t *view)
{
    gui_view_show_settings_subpage_impl(view, GUI_SETTINGS_SUBPAGE_HOME);
}

void gui_view_init_settings_panel(gui_view_t *view, lv_event_cb_t settings_event_cb,
                                  void *event_user_data)
{
    lv_obj_t *home_stack;
    lv_obj_t *page_stack;
    lv_obj_t *page_action_row;
    lv_obj_t *connectivity_stack;
    lv_obj_t *display_stack;
    lv_obj_t *system_stack;
    lv_obj_t *wifi_card;
    lv_obj_t *wifi_button_row;
    lv_obj_t *bluetooth_card;
    lv_obj_t *brightness_card;
    lv_obj_t *location_card;
    lv_obj_t *spot_price_area_card;
    lv_obj_t *reset_card;
    lv_obj_t *theme_card;
    lv_obj_t *bluetooth_status;
    lv_obj_t *settings_text;
    lv_obj_t *dialog_title;
    uint8_t network_index;

    if (view == NULL) {
        return;
    }

    view->other_settings_card = NULL;
    view->location_card = NULL;
    view->location_latitude_label = NULL;
    view->location_latitude_textarea = NULL;
    view->location_longitude_label = NULL;
    view->location_longitude_textarea = NULL;
    view->location_keyboard = NULL;
    view->spot_price_area_card = NULL;
    view->spot_price_area_dropdown = NULL;
    view->reset_card = NULL;
    view->password_dialog_spinner = NULL;
    view->password_dialog_status_label = NULL;
    view->password_dialog_connect_button_label = NULL;

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

    home_stack = lv_obj_create(view->settings_home_panel);
    lv_obj_set_size(home_stack, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(home_stack, 0, 0);
    lv_obj_set_layout(home_stack, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(home_stack, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(home_stack, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_opa(home_stack, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(home_stack, 0, 0);
    lv_obj_set_style_shadow_width(home_stack, 0, 0);
    lv_obj_set_style_pad_all(home_stack, 0, 0);
    lv_obj_set_style_pad_row(home_stack, 14, 0);
    lv_obj_clear_flag(home_stack, LV_OBJ_FLAG_SCROLLABLE);

    view->settings_connectivity_button = gui_view_create_settings_category_button(
        home_stack, "Connectivity", "Wi-Fi, Bluetooth, and network-related controls.",
        settings_event_cb, event_user_data);
    view->settings_display_button = gui_view_create_settings_category_button(
        home_stack, "Display", "Brightness, theme selection, and appearance toggles.",
        settings_event_cb, event_user_data);
    view->settings_system_button = gui_view_create_settings_category_button(
        home_stack, "System", "Device-wide settings and diagnostics.",
        settings_event_cb, event_user_data);

    view->settings_connectivity_panel = gui_view_create_settings_page(view->settings_panel);
    page_stack = gui_view_create_settings_subpage_stack(view->settings_connectivity_panel);

    connectivity_stack = gui_view_create_settings_card_grid(page_stack);

    page_action_row = gui_view_create_settings_action_row(page_stack);
    view->settings_connectivity_back_button = gui_view_create_action_button(
        page_action_row, 0, 0, 150, 58, "Back", LV_EVENT_CLICKED, settings_event_cb,
        event_user_data);

    wifi_card = gui_view_create_setting_item_card_shell(connectivity_stack, LV_SIZE_CONTENT);
    gui_view_set_settings_grid_card(wifi_card);
    view->wifi_card = wifi_card;

    view->wifi_header_row = lv_obj_create(wifi_card);
    lv_obj_set_width(view->wifi_header_row, LV_PCT(100));
    lv_obj_set_height(view->wifi_header_row, LV_SIZE_CONTENT);
    lv_obj_set_layout(view->wifi_header_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(view->wifi_header_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(view->wifi_header_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(view->wifi_header_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(view->wifi_header_row, 0, 0);
    lv_obj_set_style_shadow_width(view->wifi_header_row, 0, 0);
    lv_obj_set_style_pad_all(view->wifi_header_row, 0, 0);
    lv_obj_set_style_pad_column(view->wifi_header_row, 12, 0);
    lv_obj_clear_flag(view->wifi_header_row, LV_OBJ_FLAG_SCROLLABLE);

    view->wifi_title_label = lv_label_create(view->wifi_header_row);
    lv_label_set_text(view->wifi_title_label, "Wi-Fi");
    lv_obj_set_flex_grow(view->wifi_title_label, 1);
    lv_obj_set_style_text_color(view->wifi_title_label, lv_color_hex(0x10213D), 0);

    view->wifi_status_label = lv_label_create(view->wifi_header_row);
    lv_obj_set_width(view->wifi_status_label, 220);
    lv_label_set_long_mode(view->wifi_status_label, LV_LABEL_LONG_DOT);
    lv_label_set_text(view->wifi_status_label, "Not connected");
    lv_obj_set_style_text_color(view->wifi_status_label, lv_color_hex(0x607089), 0);
    lv_obj_set_style_text_font(view->wifi_status_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(view->wifi_status_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_align(view->wifi_status_label, LV_ALIGN_RIGHT_MID, 0);

    view->wifi_subtitle_label = lv_label_create(wifi_card);
    lv_label_set_text(view->wifi_subtitle_label,
                      "Scan for networks and connect.");
    lv_obj_set_width(view->wifi_subtitle_label, LV_PCT(100));
    lv_label_set_long_mode(view->wifi_subtitle_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(view->wifi_subtitle_label, lv_color_hex(0x607089), 0);

    wifi_button_row = lv_obj_create(wifi_card);
    lv_obj_set_width(wifi_button_row, LV_PCT(100));
    lv_obj_set_height(wifi_button_row, LV_SIZE_CONTENT);
    lv_obj_set_layout(wifi_button_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(wifi_button_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(wifi_button_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_opa(wifi_button_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(wifi_button_row, 0, 0);
    lv_obj_set_style_shadow_width(wifi_button_row, 0, 0);
    lv_obj_set_style_pad_all(wifi_button_row, 0, 0);
    lv_obj_set_style_pad_column(wifi_button_row, 12, 0);
    lv_obj_clear_flag(wifi_button_row, LV_OBJ_FLAG_SCROLLABLE);

    view->scan_button = gui_view_create_action_button(wifi_button_row, 0, 0, 120, 40, "Scan",
                                                      LV_EVENT_CLICKED, settings_event_cb,
                                                      event_user_data);
    view->disconnect_button = gui_view_create_action_button(
        wifi_button_row, 0, 0, 120, 40, "Disconnect", LV_EVENT_CLICKED, settings_event_cb,
        event_user_data);
    lv_obj_add_flag(view->disconnect_button, LV_OBJ_FLAG_HIDDEN);

    bluetooth_card = gui_view_create_setting_item_card(
        connectivity_stack, "Bluetooth",
        "Connect to nearby devices.", LV_SIZE_CONTENT);
    gui_view_set_settings_grid_card(bluetooth_card);
    view->bluetooth_card = bluetooth_card;

    bluetooth_status = lv_label_create(bluetooth_card);
    view->bluetooth_status_label = bluetooth_status;
    lv_label_set_text(bluetooth_status, "Not connected.");
    lv_obj_set_width(bluetooth_status, LV_PCT(100));
    lv_obj_set_style_text_color(bluetooth_status, lv_color_hex(0x4A5C78), 0);

    view->settings_display_panel = gui_view_create_settings_page(view->settings_panel);
    page_stack = gui_view_create_settings_subpage_stack(view->settings_display_panel);

    display_stack = gui_view_create_settings_card_grid(page_stack);

    page_action_row = gui_view_create_settings_action_row(page_stack);
    view->settings_display_back_button = gui_view_create_action_button(
        page_action_row, 0, 0, 150, 58, "Back", LV_EVENT_CLICKED, settings_event_cb,
        event_user_data);

    brightness_card = gui_view_create_setting_item_card(
        display_stack, "Screen brightness",
        "Adjust the backlight level.",
        LV_SIZE_CONTENT);
    gui_view_set_settings_grid_card(brightness_card);
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
        display_stack, "Theme",
        "Choose interface style.", LV_SIZE_CONTENT);
    gui_view_set_settings_grid_card(theme_card);
    view->theme_card = theme_card;

    view->theme_dropdown = lv_dropdown_create(theme_card);
    lv_obj_set_size(view->theme_dropdown, LV_PCT(100), 46);
    {
        char theme_options[256] = "";
        gui_theme_build_dropdown_string(theme_options, sizeof(theme_options));
        lv_dropdown_set_options(view->theme_dropdown, theme_options);
    }
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

    lv_obj_t *theme_night_row = lv_obj_create(theme_card);
    lv_obj_set_width(theme_night_row, LV_PCT(100));
    lv_obj_set_height(theme_night_row, LV_SIZE_CONTENT);
    lv_obj_set_layout(theme_night_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(theme_night_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(theme_night_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_opa(theme_night_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(theme_night_row, 0, 0);
    lv_obj_set_style_shadow_width(theme_night_row, 0, 0);
    lv_obj_set_style_pad_all(theme_night_row, 0, 0);
    lv_obj_set_style_pad_column(theme_night_row, 12, 0);
    lv_obj_clear_flag(theme_night_row, LV_OBJ_FLAG_SCROLLABLE);

    view->theme_night_label = lv_label_create(theme_night_row);
    lv_label_set_text(view->theme_night_label, "Use night variant");
    lv_obj_set_width(view->theme_night_label, LV_PCT(100));
    lv_obj_set_style_text_color(view->theme_night_label, lv_color_hex(0x10213D), 0);
    lv_obj_set_style_text_align(view->theme_night_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_flex_grow(view->theme_night_label, 1);

    view->theme_night_switch = lv_switch_create(theme_night_row);
    lv_obj_add_event_cb(view->theme_night_switch, settings_event_cb, LV_EVENT_VALUE_CHANGED,
                        event_user_data);
    lv_obj_set_style_bg_color(view->theme_night_switch, lv_color_hex(0xD9E3F1), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(view->theme_night_switch, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(view->theme_night_switch, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(view->theme_night_switch, lv_color_hex(0xD7E1EE),
                                  LV_PART_MAIN);
    lv_obj_set_style_bg_color(view->theme_night_switch, lv_color_hex(0x1D4ED8),
                              LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(view->theme_night_switch, LV_OPA_COVER,
                            LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_border_width(view->theme_night_switch, 0,
                                  LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(view->theme_night_switch, lv_color_hex(0xFFFFFF),
                              LV_PART_KNOB);
    lv_obj_set_style_bg_opa(view->theme_night_switch, LV_OPA_COVER, LV_PART_KNOB);

    view->settings_system_panel = gui_view_create_settings_page(view->settings_panel);
    page_stack = gui_view_create_settings_subpage_stack(view->settings_system_panel);

    system_stack = gui_view_create_settings_card_grid(page_stack);

    page_action_row = gui_view_create_settings_action_row(page_stack);
    view->settings_system_back_button = gui_view_create_action_button(
        page_action_row, 0, 0, 150, 58, "Back", LV_EVENT_CLICKED, settings_event_cb,
        event_user_data);

    location_card = gui_view_create_setting_item_card(
        system_stack, "Location", "Latitude and longitude in decimal degrees.",
        LV_SIZE_CONTENT);
    gui_view_set_settings_grid_card(location_card);
    view->location_card = location_card;
    (void)gui_view_create_settings_field_row(location_card, &view->location_latitude_label,
                                             &view->location_latitude_textarea, "Latitude",
                                             "59.3293", settings_event_cb, event_user_data);
    (void)gui_view_create_settings_field_row(location_card, &view->location_longitude_label,
                                             &view->location_longitude_textarea, "Longitude",
                                             "18.0686", settings_event_cb, event_user_data);

    spot_price_area_card = gui_view_create_setting_item_card(
        system_stack, "Spot-price area", "Price area for spot-price data.",
        LV_SIZE_CONTENT);
    gui_view_set_settings_grid_card(spot_price_area_card);
    view->spot_price_area_card = spot_price_area_card;

    view->spot_price_area_dropdown = lv_dropdown_create(spot_price_area_card);
    lv_obj_set_size(view->spot_price_area_dropdown, LV_PCT(100), 46);
    lv_dropdown_set_options(view->spot_price_area_dropdown,
                            "SE1 / Lulea\nSE2 / Sundsvall\nSE3 / Stockholm\nSE4 / Malmo");
    lv_dropdown_set_selected(view->spot_price_area_dropdown, 2);
    lv_obj_add_event_cb(view->spot_price_area_dropdown, settings_event_cb,
                        LV_EVENT_VALUE_CHANGED, event_user_data);
    lv_obj_set_style_radius(view->spot_price_area_dropdown, 14, 0);
    lv_obj_set_style_shadow_width(view->spot_price_area_dropdown, 0, 0);
    lv_obj_set_style_border_width(view->spot_price_area_dropdown, 1, 0);
    lv_obj_set_style_border_color(view->spot_price_area_dropdown, lv_color_hex(0xD7E1EE), 0);
    lv_obj_set_style_bg_color(view->spot_price_area_dropdown, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(view->spot_price_area_dropdown, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(view->spot_price_area_dropdown, lv_color_hex(0x10213D), 0);

    // Reset card for the systems panel
    reset_card = gui_view_create_setting_item_card(
        system_stack, "Reset", "Reset to factory defaults.", LV_SIZE_CONTENT);
    gui_view_set_settings_grid_card(reset_card);

    view->reset_button = gui_view_create_action_button(reset_card, 0, 0, 120, 40, "Reset",
                                                       LV_EVENT_CLICKED, settings_event_cb,
                                                       event_user_data);

    view->reset_card = reset_card;

    view->location_keyboard = lv_keyboard_create(view->settings_system_panel);
    lv_obj_set_size(view->location_keyboard, 400, 212);
    lv_obj_align(view->location_keyboard, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_radius(view->location_keyboard, 18, 0);
    lv_obj_set_style_bg_color(view->location_keyboard, lv_color_hex(0xE7EDF5), 0);
    lv_obj_set_style_bg_opa(view->location_keyboard, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(view->location_keyboard, 0, 0);
    lv_obj_set_style_border_width(view->location_keyboard, 1, 0);
    lv_obj_set_style_border_color(view->location_keyboard, lv_color_hex(0xD7E1EE), 0);
    lv_obj_set_style_text_font(view->location_keyboard, &lv_font_montserrat_24, LV_PART_ITEMS);
    lv_keyboard_set_mode(view->location_keyboard, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_add_flag(view->location_keyboard, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
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
        view->network_dialog, 562, 444, 128, 48, "Back", LV_EVENT_CLICKED, settings_event_cb,
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
    lv_obj_align(view->wifi_password_textarea, LV_ALIGN_TOP_MID, 0, 80);
    lv_textarea_set_one_line(view->wifi_password_textarea, true);
    lv_textarea_set_password_mode(view->wifi_password_textarea, true);
    lv_textarea_set_placeholder_text(view->wifi_password_textarea, "Enter Wi-Fi password");
    lv_obj_add_event_cb(view->wifi_password_textarea, settings_event_cb, LV_EVENT_ALL,
                        event_user_data);

    view->password_dialog_spinner = lv_spinner_create(view->password_dialog, 900, 60);
    lv_obj_set_size(view->password_dialog_spinner, 22, 22);
    lv_obj_align(view->password_dialog_spinner, LV_ALIGN_TOP_LEFT, 34, 138);
    lv_obj_set_style_arc_width(view->password_dialog_spinner, 3, LV_PART_MAIN);
    lv_obj_set_style_arc_width(view->password_dialog_spinner, 3, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(view->password_dialog_spinner, lv_color_hex(0xD7E1EE),
                               LV_PART_MAIN);
    lv_obj_set_style_arc_color(view->password_dialog_spinner, lv_color_hex(0xF59E0B),
                               LV_PART_INDICATOR);
    lv_obj_add_flag(view->password_dialog_spinner, LV_OBJ_FLAG_HIDDEN);

    view->password_dialog_status_label = lv_label_create(view->password_dialog);
    lv_obj_set_width(view->password_dialog_status_label, 480);
    lv_label_set_long_mode(view->password_dialog_status_label, LV_LABEL_LONG_DOT);
    lv_label_set_text(view->password_dialog_status_label, "");
    lv_obj_set_style_text_color(view->password_dialog_status_label, lv_color_hex(0x607089), 0);
    lv_obj_set_style_text_font(view->password_dialog_status_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(view->password_dialog_status_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(view->password_dialog_status_label, LV_ALIGN_TOP_LEFT, 64, 138);
    lv_obj_add_flag(view->password_dialog_status_label, LV_OBJ_FLAG_HIDDEN);

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
        view->password_dialog, 282, 148, 128, 40, "Back", LV_EVENT_CLICKED, settings_event_cb,
        event_user_data);
    view->password_dialog_disconnect_button = gui_view_create_action_button(
        view->password_dialog, 426, 148, 128, 40, "Disconnect", LV_EVENT_CLICKED, settings_event_cb,
        event_user_data);
    lv_obj_add_flag(view->password_dialog_disconnect_button, LV_OBJ_FLAG_HIDDEN);
    view->password_dialog_connect_button = gui_view_create_action_button(
        view->password_dialog, 570, 148, 132, 40, "Connect", LV_EVENT_CLICKED, settings_event_cb,
        event_user_data);
    view->password_dialog_connect_button_label =
        lv_obj_get_child(view->password_dialog_connect_button, 0);
    lv_obj_set_style_bg_color(view->password_dialog_connect_button, lv_color_hex(0x1D4ED8), 0);
    lv_obj_set_style_text_color(view->password_dialog_connect_button, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(view->password_dialog_connect_button, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(view->password_dialog_connect_button, 0, 0);
    gui_view_layout_password_dialog_buttons(view, false);

    view->active_settings_subpage = GUI_SETTINGS_SUBPAGE_HOME;
    gui_view_hide_wifi_dialogs_impl(view);
}

void gui_view_apply_settings_panel(gui_view_t *view, const gui_view_model_t *model)
{
    char wifi_text[72];
    const char *network_empty_text = "No networks found yet.";
    bool can_disconnect;
    bool is_connecting;
    bool location_changed;
    bool spot_price_area_changed;
    bool wifi_changed;
    uint8_t network_index;

    if ((view == NULL) || (model == NULL)) {
        return;
    }

    wifi_changed = gui_view_wifi_settings_changed(view, &model->wifi);
    location_changed = gui_view_location_settings_changed(view, &model->location);
    spot_price_area_changed = gui_view_spot_price_area_changed(view, model->spot_price.area);
    if (!wifi_changed && !location_changed && !spot_price_area_changed &&
        !gui_view_appearance_settings_changed(view, &model->appearance)) {
        return;
    }

    if (view->theme_dropdown != NULL) {
        gui_view_theme_t resolved_theme = gui_theme_resolve_available(model->appearance.theme);
        uint16_t theme_dropdown_index;

        if (gui_theme_theme_to_dropdown_index(resolved_theme, &theme_dropdown_index) &&
            (lv_dropdown_get_selected(view->theme_dropdown) != theme_dropdown_index)) {
            lv_dropdown_set_selected(view->theme_dropdown, theme_dropdown_index);
        }
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

    if (view->theme_night_switch != NULL) {
        gui_view_theme_t resolved_theme = gui_theme_resolve_available(model->appearance.theme);
        bool desired_night_enabled;
        bool night_enabled = lv_obj_has_state(view->theme_night_switch, LV_STATE_CHECKED);
        const gui_theme_def_t *theme_def = gui_theme_get(resolved_theme);
        bool should_disable_night_switch = (theme_def == NULL) ||
                                           !theme_def->has_night_variant;

        desired_night_enabled = model->appearance.night_variant_enabled &&
                                !should_disable_night_switch;

        if (night_enabled != desired_night_enabled) {
            if (desired_night_enabled) {
                lv_obj_add_state(view->theme_night_switch, LV_STATE_CHECKED);
            } else {
                lv_obj_clear_state(view->theme_night_switch, LV_STATE_CHECKED);
            }
        }

        if (should_disable_night_switch) {
            lv_obj_clear_state(view->theme_night_switch, LV_STATE_CHECKED);
            lv_obj_add_state(view->theme_night_switch, LV_STATE_DISABLED);
        } else {
            lv_obj_clear_state(view->theme_night_switch, LV_STATE_DISABLED);
        }
    }

    if (view->wifi_status_label != NULL) {
        lv_obj_set_style_text_color(view->wifi_status_label,
                                    gui_view_wifi_status_color(view->current_theme,
                                                               model->wifi.state),
                                    0);
        gui_view_set_label_text_if_changed(view->wifi_status_label,
                                           gui_view_wifi_card_status_text(&model->wifi));
    }

    can_disconnect = model->wifi.can_disconnect;
    is_connecting = model->wifi.state == GUI_WIFI_STATE_CONNECTING;

    if (view->disconnect_button != NULL) {
        if (can_disconnect) {
            lv_obj_clear_flag(view->disconnect_button, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(view->disconnect_button, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (view->scan_button != NULL) {
        if (is_connecting) {
            lv_obj_add_state(view->scan_button, LV_STATE_DISABLED);
        } else {
            lv_obj_clear_state(view->scan_button, LV_STATE_DISABLED);
        }
    }

    if (view->password_dialog_connect_button != NULL) {
        if (is_connecting) {
            lv_obj_add_state(view->password_dialog_connect_button, LV_STATE_DISABLED);
        } else {
            lv_obj_clear_state(view->password_dialog_connect_button, LV_STATE_DISABLED);
        }
    }

    if (view->password_dialog_disconnect_button != NULL) {
        if (can_disconnect) {
            lv_obj_clear_flag(view->password_dialog_disconnect_button, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(view->password_dialog_disconnect_button, LV_OBJ_FLAG_HIDDEN);
        }
    }
    gui_view_layout_password_dialog_buttons(view, can_disconnect);

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
    gui_view_update_password_dialog_feedback(view, &model->wifi);
    if (!lv_obj_has_state(view->wifi_password_textarea, LV_STATE_FOCUSED)) {
        gui_view_set_textarea_text_if_changed(view->wifi_password_textarea, model->wifi.password);
    }
    if ((view->location_latitude_textarea != NULL) &&
        !lv_obj_has_state(view->location_latitude_textarea, LV_STATE_FOCUSED)) {
        gui_view_set_textarea_text_if_changed(view->location_latitude_textarea,
                                              model->location.latitude);
    }
    if ((view->location_longitude_textarea != NULL) &&
        !lv_obj_has_state(view->location_longitude_textarea, LV_STATE_FOCUSED)) {
        gui_view_set_textarea_text_if_changed(view->location_longitude_textarea,
                                              model->location.longitude);
    }
    if (view->spot_price_area_dropdown != NULL) {
        uint16_t selected_index = gui_view_spot_price_area_dropdown_index(model->spot_price.area);

        if (lv_dropdown_get_selected(view->spot_price_area_dropdown) != selected_index) {
            lv_dropdown_set_selected(view->spot_price_area_dropdown, selected_index);
        }
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
    view->last_location_settings = model->location;
    view->has_last_location_settings = true;
    view->last_spot_price_area = model->spot_price.area;
    view->has_last_spot_price_area = true;
}
