#ifndef GUI_THEME_DEFS_H
#define GUI_THEME_DEFS_H

#include <stdbool.h>
#include <stddef.h>

#include "lvgl.h"

#include "gui_types.h"

/*
 * Number of entries in the theme table.
 * Update when adding new enum values to gui_view_theme_t.
 */
#define GUI_THEME_COUNT 7

/*
 * Per-theme descriptor. All colors are stored as 0xRRGGBB uint32_t values.
 * Convert to lv_color_t at usage time with lv_color_hex().
 *
 * To add a new theme:
 *   1. Add an enum value to gui_view_theme_t in gui_types.h.
 *   2. Increment GUI_THEME_COUNT above.
 *   3. Add a table entry in gui_theme_defs.c matching the new enum value.
 *      Set is_user_selectable = true so it appears in the settings dropdown.
 *   4. If the theme has a background image, add the asset files and reference
 *      them via background_image. If it has a night variant, set has_night_variant
 *      and night_variant.
 */
typedef struct {
    /* Identity */
    const char *display_name;      /* Label shown in the settings dropdown */
    bool is_user_selectable;       /* False for internal variants (e.g. night mode) */
    bool has_night_variant;        /* True if a night-mode variant exists */
    gui_view_theme_t night_variant; /* Only valid when has_night_variant is true */
    bool dialog_has_border;        /* True when modal dialogs should show a border */

    /* Fonts */
    const lv_font_t *body_font;
    const lv_font_t *emphasis_font;

    /* Background image (NULL if not supported) */
    const lv_img_dsc_t *background_image;

    /* Screen gradient */
    uint32_t screen_bg;
    uint32_t screen_grad;

    /* Sidebar */
    uint32_t sidebar_bg;
    uint32_t sidebar_grad;
    uint32_t sidebar_shadow;
    uint32_t brand_text;

    /* Content area */
    uint32_t content_bg;
    uint32_t content_shadow;

    /* Typography */
    uint32_t title_text;
    uint32_t subtitle_text;
    uint32_t muted_text;
    uint32_t value_text;

    /* Panels and cards */
    uint32_t panel_bg;
    uint32_t panel_border;
    uint32_t card_bg;
    uint32_t card_border;
    uint32_t item_bg;
    uint32_t item_border;

    /* Accents */
    uint32_t accent_color;
    uint32_t accent_soft_color;

    /* Keyboard */
    uint32_t keyboard_bg;
    uint32_t keyboard_border;
    uint32_t keyboard_key_bg;
    uint32_t keyboard_key_text;
    uint32_t keyboard_special_bg;
    uint32_t keyboard_special_text;
    uint32_t keyboard_special_border;

    /* Slider and toggle switches */
    uint32_t slider_bg;
    uint32_t slider_knob_bg;

    /* Dropdowns */
    uint32_t dropdown_bg;
    uint32_t dropdown_border;
    uint32_t dropdown_selected_bg;
    uint32_t dropdown_selected_text;

    /* Energy chart */
    uint32_t energy_chart_bg;
    uint32_t energy_chart_grid;
    uint32_t energy_chart_tick;
    uint32_t energy_buy_color;
    uint32_t energy_solar_color;
    uint32_t energy_charge_color;
    uint32_t energy_sell_color;

    /* WiFi status indicator */
    uint32_t wifi_connected_color;
    uint32_t wifi_idle_color;

    /* Navigation sidebar buttons */
    uint32_t nav_active_bg;
    uint32_t nav_active_text;
    uint32_t nav_active_border;
    uint32_t nav_inactive_bg;
    uint32_t nav_inactive_text;
    uint32_t nav_inactive_border;

    /* Primary / secondary action buttons */
    uint32_t action_primary_bg;
    uint32_t action_primary_text;
    uint32_t action_primary_border;
    uint32_t action_secondary_bg;
    uint32_t action_secondary_text;
    uint32_t action_secondary_border;

    /* Scanned WiFi network list buttons */
    uint32_t wifi_btn_bg;
    uint32_t wifi_btn_border;
    uint32_t wifi_btn_text;
    uint32_t wifi_btn_connected_bg;
    uint32_t wifi_btn_connected_border;
    uint32_t wifi_btn_connected_text;
    uint32_t wifi_btn_known_bg;
    uint32_t wifi_btn_known_border;
    uint32_t wifi_btn_known_text;
    uint32_t wifi_btn_selected_bg;
    uint32_t wifi_btn_selected_border;
    uint32_t wifi_btn_selected_text;
} gui_theme_def_t;

/*
 * Returns the descriptor for the given theme, or NULL if the value is
 * out of range.
 */
const gui_theme_def_t *gui_theme_get(gui_view_theme_t theme);

/*
 * Writes a newline-delimited string of user-selectable theme names into buf.
 * Suitable for passing directly to lv_dropdown_set_options().
 */
void gui_theme_build_dropdown_string(char *buf, size_t buf_size);

#endif
