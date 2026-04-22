#include "gui_view.h"

#include "gui_view_common.h"
#include "panels/gui_view_bme280_panel.h"
#include "panels/gui_view_energy_panel.h"
#include "panels/gui_view_settings_panel.h"

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
            "24-hour recommendation for grid, solar, battery, and export decisions.");
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

static void gui_view_style_nav_button(lv_obj_t *button, bool is_active)
{
    lv_color_t bg_color = is_active ? lv_color_hex(0xE8F0FF) : lv_color_hex(0x1B2437);
    lv_color_t text_color = is_active ? lv_color_hex(0x10213D) : lv_color_hex(0xDCE6F5);
    lv_color_t border_color = is_active ? lv_color_hex(0x8FB3FF) : lv_color_hex(0x2A3954);

    lv_obj_set_style_bg_color(button, bg_color, 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(button, border_color, 0);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_text_color(button, text_color, 0);
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

    view->screen = lv_scr_act();
    lv_obj_clean(view->screen);
    lv_obj_clear_flag(view->screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(view->screen, lv_color_hex(0xDCE8F5), 0);
    lv_obj_set_style_bg_grad_color(view->screen, lv_color_hex(0xF5F9FF), 0);
    lv_obj_set_style_bg_grad_dir(view->screen, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(view->screen, LV_OPA_COVER, 0);

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
    lv_obj_set_style_text_font(brand, &lv_font_montserrat_14, 0);
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
    lv_obj_set_style_text_font(view->header_title, &lv_font_montserrat_14, 0);
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

    if (panel_changed) {
        gui_view_style_nav_button(view->bme280_button,
                                  model->active_panel == GUI_PANEL_BME280);
        gui_view_style_nav_button(view->energy_plan_button,
                                  model->active_panel == GUI_PANEL_ENERGY_PLAN);
        gui_view_style_nav_button(view->settings_button,
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