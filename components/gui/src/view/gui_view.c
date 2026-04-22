#include "gui_view.h"

#include "gui_view_common.h"
#include "panels/gui_view_bme280_panel.h"
#include "panels/gui_view_energy_panel.h"
#include "panels/gui_view_settings_panel.h"

LV_IMG_DECLARE(hk_bg);
LV_FONT_DECLARE(hellokitty18);
LV_FONT_DECLARE(hellokitty24);

static const lv_font_t *gui_view_body_font(gui_view_theme_t theme)
{
    return (theme == GUI_VIEW_THEME_HELLO_KITTY) ? &hellokitty18 : &lv_font_montserrat_18;
}

static const lv_font_t *gui_view_emphasis_font(gui_view_theme_t theme)
{
    return (theme == GUI_VIEW_THEME_HELLO_KITTY) ? &hellokitty24 : &lv_font_montserrat_24;
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
        lv_obj_add_flag(view->settings_panel, LV_OBJ_FLAG_HIDDEN);
    } else if (model->active_panel == GUI_PANEL_ENERGY_PLAN) {
        gui_view_set_label_text_if_changed(view->header_title, "LEOP Energy Plan");
        gui_view_set_label_text_if_changed(
            view->header_subtitle,
            "Recommendations for grid, solar, battery, and export decisions.");
        lv_obj_add_flag(view->bme280_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(view->energy_plan_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(view->settings_panel, LV_OBJ_FLAG_HIDDEN);
    } else {
        gui_view_set_label_text_if_changed(view->header_title, "");
        gui_view_set_label_text_if_changed(view->header_subtitle, "");
        lv_obj_add_flag(view->bme280_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(view->energy_plan_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(view->settings_panel, LV_OBJ_FLAG_HIDDEN);
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

    body_font = gui_view_body_font(theme);

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

static void gui_view_style_bme280_cards(gui_view_t *view, lv_color_t card_bg,
                                        lv_color_t card_border, lv_color_t label_color,
                                        lv_color_t value_color, gui_view_theme_t theme)
{
    uint32_t child_count;
    const lv_font_t *body_font;

    if ((view == NULL) || (view->bme280_panel == NULL)) {
        return;
    }

    body_font = gui_view_body_font(theme);

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

    body_font = gui_view_body_font(theme);

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

static void gui_view_style_nav_button(lv_obj_t *button, gui_view_theme_t theme, bool is_active)
{
    lv_color_t bg_color;
    lv_color_t text_color;
    lv_color_t border_color;

    if (theme == GUI_VIEW_THEME_DARK) {
        bg_color = is_active ? lv_color_hex(0x1D4ED8) : lv_color_hex(0x0F172A);
        text_color = is_active ? lv_color_hex(0xF8FAFC) : lv_color_hex(0xCBD5E1);
        border_color = is_active ? lv_color_hex(0x60A5FA) : lv_color_hex(0x334155);
    } else if (theme == GUI_VIEW_THEME_HELLO_KITTY) {
        bg_color = is_active ? lv_color_hex(0xFFE0EB) : lv_color_hex(0xFFF5F8);
        text_color = is_active ? lv_color_hex(0x8A1D47) : lv_color_hex(0xA13A64);
        border_color = is_active ? lv_color_hex(0xFB7185) : lv_color_hex(0xF4A3BE);
    } else {
        bg_color = is_active ? lv_color_hex(0xE8F0FF) : lv_color_hex(0x1B2437);
        text_color = is_active ? lv_color_hex(0x10213D) : lv_color_hex(0xDCE6F5);
        border_color = is_active ? lv_color_hex(0x8FB3FF) : lv_color_hex(0x2A3954);
    }

    lv_obj_set_style_bg_color(button, bg_color, 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(button, border_color, 0);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_text_color(button, text_color, 0);
    lv_obj_set_style_text_font(button, gui_view_body_font(theme), 0);
}

static void gui_view_style_action_button(lv_obj_t *button, gui_view_theme_t theme,
                                         bool is_primary)
{
    lv_color_t bg_color;
    lv_color_t text_color;
    lv_color_t border_color;

    if (button == NULL) {
        return;
    }

    if (theme == GUI_VIEW_THEME_DARK) {
        if (is_primary) {
            bg_color = lv_color_hex(0x2563EB);
            text_color = lv_color_hex(0xF8FAFC);
            border_color = lv_color_hex(0x60A5FA);
        } else {
            bg_color = lv_color_hex(0x172033);
            text_color = lv_color_hex(0xD7E3F4);
            border_color = lv_color_hex(0x475569);
        }
    } else if (theme == GUI_VIEW_THEME_HELLO_KITTY) {
        if (is_primary) {
            bg_color = lv_color_hex(0xFB7185);
            text_color = lv_color_hex(0xFFFDFE);
            border_color = lv_color_hex(0xF472B6);
        } else {
            bg_color = lv_color_hex(0xFFF0F6);
            text_color = lv_color_hex(0x8A1D47);
            border_color = lv_color_hex(0xF4A3BE);
        }
    } else {
        if (is_primary) {
            bg_color = lv_color_hex(0x1D4ED8);
            text_color = lv_color_hex(0xFFFFFF);
            border_color = lv_color_hex(0x1D4ED8);
        } else {
            bg_color = lv_color_hex(0xFFFFFF);
            text_color = lv_color_hex(0x10213D);
            border_color = lv_color_hex(0xD7E1EE);
        }
    }

    lv_obj_set_style_bg_color(button, bg_color, 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(button, border_color, 0);
    lv_obj_set_style_border_width(button, is_primary ? 0 : 1, 0);
    lv_obj_set_style_text_color(button, text_color, 0);
    lv_obj_set_style_text_font(button, gui_view_body_font(theme), 0);
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

void gui_view_apply_theme(gui_view_t *view, gui_view_theme_t theme, bool show_background_image)
{
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
    lv_color_t accent_color;
    lv_color_t accent_soft_color;
    lv_color_t energy_chart_bg;
    const lv_font_t *body_font;
    const lv_font_t *emphasis_font;
    lv_obj_t *brand;
    lv_obj_t *dropdown_list;
    bool use_background_image;

    if (view == NULL) {
        return;
    }

    view->current_theme = theme;
    view->current_show_background_image = show_background_image;
    view->has_current_appearance = true;
    use_background_image = (theme == GUI_VIEW_THEME_HELLO_KITTY) && show_background_image;
    body_font = gui_view_body_font(theme);
    emphasis_font = gui_view_emphasis_font(theme);

    if (theme == GUI_VIEW_THEME_DARK) {
        screen_bg = lv_color_hex(0x0B1220);
        screen_grad = lv_color_hex(0x172033);
        screen_bg_opa = LV_OPA_COVER;
        sidebar_bg = lv_color_hex(0x020617);
        sidebar_grad = lv_color_hex(0x111827);
        sidebar_shadow = lv_color_hex(0x020617);
        sidebar_bg_opa = LV_OPA_COVER;
        brand_text = lv_color_hex(0xF8FAFC);
        content_bg = lv_color_hex(0x111827);
        content_shadow = lv_color_hex(0x020617);
        content_bg_opa = LV_OPA_90;
        title_text = lv_color_hex(0xF8FAFC);
        subtitle_text = lv_color_hex(0xAAB7C8);
        panel_bg = lv_color_hex(0x1E293B);
        panel_border = lv_color_hex(0x334155);
        panel_bg_opa = LV_OPA_COVER;
        card_bg = lv_color_hex(0x0F172A);
        card_border = lv_color_hex(0x334155);
        item_bg = lv_color_hex(0x172033);
        item_border = lv_color_hex(0x334155);
        muted_text = lv_color_hex(0xCBD5E1);
        value_text = lv_color_hex(0xF8FAFC);
        keyboard_bg = lv_color_hex(0x243244);
        keyboard_border = lv_color_hex(0x334155);
        keyboard_key_bg = lv_color_hex(0x334155);
        keyboard_key_text = lv_color_hex(0xF8FAFC);
        keyboard_special_bg = lv_color_hex(0x2563EB);
        keyboard_special_text = lv_color_hex(0xF8FAFC);
        keyboard_special_border = lv_color_hex(0x60A5FA);
        slider_bg = lv_color_hex(0x334155);
        slider_knob_bg = lv_color_hex(0xE2E8F0);
        dropdown_bg = lv_color_hex(0x172033);
        dropdown_border = lv_color_hex(0x475569);
        accent_color = lv_color_hex(0x60A5FA);
        accent_soft_color = lv_color_hex(0x93C5FD);
        energy_chart_bg = lv_color_hex(0x0F172A);
    } else if (theme == GUI_VIEW_THEME_HELLO_KITTY) {
        screen_bg = lv_color_hex(0xFFDDE8);
        screen_grad = lv_color_hex(0xFFF7FB);
        screen_bg_opa = use_background_image ? LV_OPA_TRANSP : LV_OPA_COVER;
        sidebar_bg = lv_color_hex(0xFFF0F5);
        sidebar_grad = lv_color_hex(0xFFE3EC);
        sidebar_shadow = lv_color_hex(0xF4A3BE);
        sidebar_bg_opa = use_background_image ? LV_OPA_TRANSP : LV_OPA_COVER;
        brand_text = lv_color_hex(0xC2185B);
        content_bg = lv_color_hex(0xFFFDFE);
        content_shadow = lv_color_hex(0xF6B8CC);
        content_bg_opa = use_background_image ? LV_OPA_TRANSP : LV_OPA_90;
        title_text = lv_color_hex(0x8A1D47);
        subtitle_text = lv_color_hex(0xA65374);
        panel_bg = lv_color_hex(0xFFF5F8);
        panel_border = lv_color_hex(0xF6BDD0);
        panel_bg_opa = LV_OPA_COVER;
        card_bg = lv_color_hex(0xFFFFFF);
        card_border = lv_color_hex(0xF7C9D8);
        item_bg = lv_color_hex(0xFFF8FB);
        item_border = lv_color_hex(0xF7C9D8);
        muted_text = lv_color_hex(0xB0597E);
        value_text = lv_color_hex(0x8A1D47);
        keyboard_bg = lv_color_hex(0xFFD8E6);
        keyboard_border = lv_color_hex(0xF4A3BE);
        keyboard_key_bg = lv_color_hex(0xFFF5F8);
        keyboard_key_text = lv_color_hex(0x8A1D47);
        keyboard_special_bg = lv_color_hex(0xFB7185);
        keyboard_special_text = lv_color_hex(0xFFFDFE);
        keyboard_special_border = lv_color_hex(0xF472B6);
        slider_bg = lv_color_hex(0xF6C1D4);
        slider_knob_bg = lv_color_hex(0xFFFFFF);
        dropdown_bg = lv_color_hex(0xFFF8FB);
        dropdown_border = lv_color_hex(0xF4A3BE);
        accent_color = lv_color_hex(0xFB7185);
        accent_soft_color = lv_color_hex(0xF472B6);
        energy_chart_bg = lv_color_hex(0xFFFFFF);
    } else {
        screen_bg = lv_color_hex(0xDCE8F5);
        screen_grad = lv_color_hex(0xF5F9FF);
        screen_bg_opa = LV_OPA_COVER;
        sidebar_bg = lv_color_hex(0x111827);
        sidebar_grad = lv_color_hex(0x1E293B);
        sidebar_shadow = lv_color_hex(0x94A3B8);
        sidebar_bg_opa = LV_OPA_COVER;
        brand_text = lv_color_hex(0xF8FAFC);
        content_bg = lv_color_hex(0xFFFFFF);
        content_shadow = lv_color_hex(0xB8C7DB);
        content_bg_opa = LV_OPA_90;
        title_text = lv_color_hex(0x10213D);
        subtitle_text = lv_color_hex(0x607089);
        panel_bg = lv_color_hex(0xF8FBFF);
        panel_border = lv_color_hex(0xD9E3F1);
        panel_bg_opa = LV_OPA_COVER;
        card_bg = lv_color_hex(0xFFFFFF);
        card_border = lv_color_hex(0xD7E1EE);
        item_bg = lv_color_hex(0xF8FBFF);
        item_border = lv_color_hex(0xD9E3F1);
        muted_text = lv_color_hex(0x4A5C78);
        value_text = lv_color_hex(0x0F172A);
        keyboard_bg = lv_color_hex(0xE7EDF5);
        keyboard_border = lv_color_hex(0xD7E1EE);
        keyboard_key_bg = lv_color_hex(0xFFFFFF);
        keyboard_key_text = lv_color_hex(0x10213D);
        keyboard_special_bg = lv_color_hex(0x1D4ED8);
        keyboard_special_text = lv_color_hex(0xFFFFFF);
        keyboard_special_border = lv_color_hex(0x1D4ED8);
        slider_bg = lv_color_hex(0xD9E3F1);
        slider_knob_bg = lv_color_hex(0xFFFFFF);
        dropdown_bg = lv_color_hex(0xFFFFFF);
        dropdown_border = lv_color_hex(0xD7E1EE);
        accent_color = lv_color_hex(0x1D4ED8);
        accent_soft_color = lv_color_hex(0x1D4ED8);
        energy_chart_bg = lv_color_hex(0xFFFFFF);
    }

    if (view->background_image != NULL) {
        if (use_background_image) {
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
    gui_view_style_bme280_cards(view, card_bg, card_border, subtitle_text, value_text, theme);

    lv_obj_set_style_bg_color(view->energy_plan_panel, panel_bg, 0);
    lv_obj_set_style_bg_opa(view->energy_plan_panel, panel_bg_opa, 0);
    lv_obj_set_style_border_color(view->energy_plan_panel, panel_border, 0);
    lv_obj_set_style_bg_color(view->energy_plan_chart, energy_chart_bg, 0);
    lv_obj_set_style_border_color(view->energy_plan_chart, panel_border, 0);
    lv_obj_set_style_text_font(view->energy_plan_panel, body_font, 0);
    gui_view_style_energy_labels(view->energy_plan_panel, subtitle_text, theme);

    gui_view_style_settings_card(view->connectivity_card, card_bg, card_border, title_text,
                                 subtitle_text, theme);
    gui_view_style_settings_card(view->other_settings_card, card_bg, card_border, title_text,
                                 subtitle_text, theme);
    gui_view_style_settings_card(view->wifi_card, item_bg, item_border, title_text,
                                 subtitle_text, theme);
    gui_view_style_settings_card(view->bluetooth_card, item_bg, item_border, title_text,
                                 subtitle_text, theme);
    gui_view_style_settings_card(view->brightness_card, item_bg, item_border, title_text,
                                 subtitle_text, theme);
    gui_view_style_settings_card(view->theme_card, item_bg, item_border, title_text,
                                 subtitle_text, theme);
    if (view->bluetooth_status_label != NULL) {
        lv_obj_set_style_text_color(view->bluetooth_status_label, muted_text, 0);
        lv_obj_set_style_text_font(view->bluetooth_status_label, body_font, 0);
    }
    if (view->brightness_value_label != NULL) {
        lv_obj_set_style_text_color(view->brightness_value_label, accent_soft_color, 0);
        lv_obj_set_style_text_font(view->brightness_value_label, emphasis_font, 0);
    }
    if (view->wifi_status_label != NULL) {
        lv_obj_set_style_text_font(view->wifi_status_label, body_font, 0);
    }
    if (view->wifi_selected_label != NULL) {
        lv_obj_set_style_text_font(view->wifi_selected_label, body_font, 0);
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
        }
    }

    if (view->theme_background_label != NULL) {
        lv_obj_set_style_text_color(view->theme_background_label, title_text, 0);
        lv_obj_set_style_text_font(view->theme_background_label, body_font, 0);
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

    gui_view_style_action_button(view->scan_button, theme, true);
    gui_view_style_action_button(view->network_dialog_cancel_button, theme, false);
    gui_view_style_action_button(view->password_dialog_cancel_button, theme, false);
    gui_view_style_action_button(view->password_dialog_connect_button, theme, true);

    lv_obj_set_style_bg_color(view->network_dialog, panel_bg, 0);
    lv_obj_set_style_bg_opa(view->network_dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(view->network_dialog,
                                  (theme == GUI_VIEW_THEME_HELLO_KITTY) ? 1 : 0, 0);
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
                                  (theme == GUI_VIEW_THEME_HELLO_KITTY) ? 1 : 0, 0);
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
    lv_obj_set_style_bg_color(view->wifi_keyboard, keyboard_special_bg,
                              LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(view->wifi_keyboard, LV_OPA_COVER,
                            LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_border_color(view->wifi_keyboard, keyboard_special_border,
                                  LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(view->wifi_keyboard, keyboard_special_text,
                                LV_PART_ITEMS | LV_STATE_CHECKED);

    if (view->has_last_active_panel) {
        gui_view_style_nav_button(view->bme280_button, view->current_theme,
                                  view->last_active_panel == GUI_PANEL_BME280);
        gui_view_style_nav_button(view->energy_plan_button, view->current_theme,
                                  view->last_active_panel == GUI_PANEL_ENERGY_PLAN);
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
    view->has_current_appearance = false;

    view->screen = lv_scr_act();
    lv_obj_clean(view->screen);
    lv_obj_clear_flag(view->screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(view->screen, lv_color_hex(0xDCE8F5), 0);
    lv_obj_set_style_bg_grad_color(view->screen, lv_color_hex(0xF5F9FF), 0);
    lv_obj_set_style_bg_grad_dir(view->screen, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(view->screen, LV_OPA_COVER, 0);

    view->background_image = lv_img_create(view->screen);
    lv_img_set_src(view->background_image, &hk_bg);
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

    view->bme280_button = gui_view_create_nav_button(sidebar, 140, "BME280", nav_event_cb,
                                                       event_user_data);
    view->energy_plan_button = gui_view_create_nav_button(sidebar, 212, "Energy plan",
                                                          nav_event_cb, event_user_data);
    view->settings_button = gui_view_create_nav_button(sidebar, 284, "Settings", nav_event_cb,
                                                       event_user_data);

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

    gui_view_apply(view, model);
}

void gui_view_apply(gui_view_t *view, const gui_view_model_t *model)
{
    bool panel_changed;

    if ((view == NULL) || (model == NULL)) {
        return;
    }

    panel_changed = !view->has_last_active_panel || (view->last_active_panel != model->active_panel);

    if (!view->has_current_appearance ||
        (view->current_theme != model->appearance.theme) ||
        (view->current_show_background_image != model->appearance.show_background_image)) {
        gui_view_apply_theme(view, model->appearance.theme,
                             model->appearance.show_background_image);
    }

    if (panel_changed) {
        gui_view_style_nav_button(view->bme280_button, view->current_theme,
                                  model->active_panel == GUI_PANEL_BME280);
        gui_view_style_nav_button(view->energy_plan_button, view->current_theme,
                                  model->active_panel == GUI_PANEL_ENERGY_PLAN);
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
    } else {
        gui_view_apply_settings_panel(view, model);
    }
}