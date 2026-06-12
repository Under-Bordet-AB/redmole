#ifndef GUI_THEME_DEFS_H
#define GUI_THEME_DEFS_H

/**
 * @file gui_theme_defs.h
 * @brief Theme descriptors and lookup helpers for the GUI view layer.
 *
 * Theme descriptors use 0xRRGGBB color values that callers convert to
 * lv_color_t at the point of use.
 */

#include <stdbool.h>
#include <stddef.h>

#include "lvgl.h"

#include "gui_types.h"

/** Number of entries in the theme table; keep in sync with gui_view_theme_t. */
#define GUI_THEME_COUNT 8

/**
 * @brief Complete palette, fonts, and asset references for one GUI theme.
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
    const char *display_name;       /*!< Null-terminated label shown in the settings dropdown. */
    bool is_user_selectable;        /*!< False for internal variants such as night mode. */
    bool has_night_variant;         /*!< True when night_variant names a valid alternate theme. */
    gui_view_theme_t night_variant; /*!< Theme used when night mode is enabled. */
    bool dialog_has_border;         /*!< True when modal dialogs should show a border. */

    const lv_font_t *body_font;     /*!< LVGL body font pointer, or NULL for LVGL default handling. */
    const lv_font_t *emphasis_font; /*!< LVGL emphasis font pointer, or NULL for LVGL default handling. */

    const lv_img_dsc_t *background_image; /*!< Optional LVGL background image, or NULL when unsupported. */

    uint32_t screen_bg;             /*!< Screen background color, 0xRRGGBB. */
    uint32_t screen_grad;           /*!< Screen gradient color, 0xRRGGBB. */

    uint32_t sidebar_bg;            /*!< Sidebar background color, 0xRRGGBB. */
    uint32_t sidebar_grad;          /*!< Sidebar gradient color, 0xRRGGBB. */
    uint32_t sidebar_shadow;        /*!< Sidebar shadow color, 0xRRGGBB. */
    uint32_t brand_text;            /*!< Sidebar brand text color, 0xRRGGBB. */

    uint32_t content_bg;            /*!< Main content background color, 0xRRGGBB. */
    uint32_t content_shadow;        /*!< Main content shadow color, 0xRRGGBB. */

    uint32_t title_text;            /*!< Primary heading text color, 0xRRGGBB. */
    uint32_t subtitle_text;         /*!< Secondary heading text color, 0xRRGGBB. */
    uint32_t muted_text;            /*!< Muted/supporting text color, 0xRRGGBB. */
    uint32_t value_text;            /*!< Metric value text color, 0xRRGGBB. */

    uint32_t panel_bg;              /*!< Panel background color, 0xRRGGBB. */
    uint32_t panel_border;          /*!< Panel border color, 0xRRGGBB. */
    uint32_t card_bg;               /*!< Card background color, 0xRRGGBB. */
    uint32_t card_border;           /*!< Card border color, 0xRRGGBB. */
    uint32_t item_bg;               /*!< Nested item background color, 0xRRGGBB. */
    uint32_t item_border;           /*!< Nested item border color, 0xRRGGBB. */

    uint32_t accent_color;          /*!< Primary accent color, 0xRRGGBB. */
    uint32_t accent_soft_color;     /*!< Softer accent color, 0xRRGGBB. */

    uint32_t keyboard_bg;           /*!< On-screen keyboard background color, 0xRRGGBB. */
    uint32_t keyboard_border;       /*!< On-screen keyboard border color, 0xRRGGBB. */
    uint32_t keyboard_key_bg;       /*!< Normal keyboard key background color, 0xRRGGBB. */
    uint32_t keyboard_key_text;     /*!< Normal keyboard key text color, 0xRRGGBB. */
    uint32_t keyboard_special_bg;   /*!< Special keyboard key background color, 0xRRGGBB. */
    uint32_t keyboard_special_text; /*!< Special keyboard key text color, 0xRRGGBB. */
    uint32_t keyboard_special_border; /*!< Special keyboard key border color, 0xRRGGBB. */

    uint32_t slider_bg;             /*!< Slider track background color, 0xRRGGBB. */
    uint32_t slider_knob_bg;        /*!< Slider knob background color, 0xRRGGBB. */

    uint32_t dropdown_bg;           /*!< Dropdown background color, 0xRRGGBB. */
    uint32_t dropdown_border;       /*!< Dropdown border color, 0xRRGGBB. */
    uint32_t dropdown_selected_bg;  /*!< Selected dropdown row background color, 0xRRGGBB. */
    uint32_t dropdown_selected_text; /*!< Selected dropdown row text color, 0xRRGGBB. */

    uint32_t energy_chart_bg;       /*!< Energy chart background color, 0xRRGGBB. */
    uint32_t energy_chart_grid;     /*!< Energy chart grid color, 0xRRGGBB. */
    uint32_t energy_chart_tick;     /*!< Energy chart tick label color, 0xRRGGBB. */
    uint32_t energy_buy_color;      /*!< Buy-electricity series color, 0xRRGGBB. */
    uint32_t energy_solar_color;    /*!< Direct-solar-use series color, 0xRRGGBB. */
    uint32_t energy_charge_color;   /*!< Battery-charge series color, 0xRRGGBB. */
    uint32_t energy_sell_color;     /*!< Sell-excess series color, 0xRRGGBB. */

    uint32_t wifi_connected_color;  /*!< Connected status indicator color, 0xRRGGBB. */
    uint32_t wifi_idle_color;       /*!< Idle/unavailable status indicator color, 0xRRGGBB. */

    uint32_t nav_active_bg;         /*!< Active navigation button background color, 0xRRGGBB. */
    uint32_t nav_active_text;       /*!< Active navigation button text color, 0xRRGGBB. */
    uint32_t nav_active_border;     /*!< Active navigation button border color, 0xRRGGBB. */
    uint32_t nav_inactive_bg;       /*!< Inactive navigation button background color, 0xRRGGBB. */
    uint32_t nav_inactive_text;     /*!< Inactive navigation button text color, 0xRRGGBB. */
    uint32_t nav_inactive_border;   /*!< Inactive navigation button border color, 0xRRGGBB. */

    uint32_t action_primary_bg;     /*!< Primary action button background color, 0xRRGGBB. */
    uint32_t action_primary_text;   /*!< Primary action button text color, 0xRRGGBB. */
    uint32_t action_primary_border; /*!< Primary action button border color, 0xRRGGBB. */
    uint32_t action_secondary_bg;   /*!< Secondary action button background color, 0xRRGGBB. */
    uint32_t action_secondary_text; /*!< Secondary action button text color, 0xRRGGBB. */
    uint32_t action_secondary_border; /*!< Secondary action button border color, 0xRRGGBB. */

    uint32_t wifi_btn_bg;           /*!< Scanned Wi-Fi button default background color, 0xRRGGBB. */
    uint32_t wifi_btn_border;       /*!< Scanned Wi-Fi button default border color, 0xRRGGBB. */
    uint32_t wifi_btn_text;         /*!< Scanned Wi-Fi button default text color, 0xRRGGBB. */
    uint32_t wifi_btn_connected_bg; /*!< Connected Wi-Fi button background color, 0xRRGGBB. */
    uint32_t wifi_btn_connected_border; /*!< Connected Wi-Fi button border color, 0xRRGGBB. */
    uint32_t wifi_btn_connected_text; /*!< Connected Wi-Fi button text color, 0xRRGGBB. */
    uint32_t wifi_btn_known_bg;     /*!< Known Wi-Fi button background color, 0xRRGGBB. */
    uint32_t wifi_btn_known_border; /*!< Known Wi-Fi button border color, 0xRRGGBB. */
    uint32_t wifi_btn_known_text;   /*!< Known Wi-Fi button text color, 0xRRGGBB. */
    uint32_t wifi_btn_selected_bg;  /*!< Selected Wi-Fi button background color, 0xRRGGBB. */
    uint32_t wifi_btn_selected_border; /*!< Selected Wi-Fi button border color, 0xRRGGBB. */
    uint32_t wifi_btn_selected_text; /*!< Selected Wi-Fi button text color, 0xRRGGBB. */
} gui_theme_def_t;

/**
 * @brief Look up the descriptor for a theme enum value.
 *
 * @param theme Theme to resolve.
 * @return Theme descriptor, or NULL when the value is out of range.
 */
const gui_theme_def_t *gui_theme_get(gui_view_theme_t theme);

/**
 * @brief Return the default theme used when no explicit choice is available.
 *
 * @return Default theme enum value.
 */
gui_view_theme_t gui_theme_default(void);

/**
 * @brief Resolve a theme to a valid user-available fallback when needed.
 *
 * @param theme Requested theme.
 * @return Supported theme value that can be applied to the GUI.
 */
gui_view_theme_t gui_theme_resolve_available(gui_view_theme_t theme);

/**
 * @brief Build LVGL dropdown options for user-selectable themes.
 *
 * Writes a newline-delimited string suitable for lv_dropdown_set_options().
 *
 * @param buf Output character buffer, must not be NULL.
 * @param buf_size Size of buf in bytes.
 */
void gui_theme_build_dropdown_string(char *buf, size_t buf_size);

/**
 * @brief Convert a settings dropdown row index to a selectable theme.
 *
 * @param index Zero-based row index in the user-selectable theme list.
 * @param theme_out Output theme value, must not be NULL.
 * @return True when index maps to a selectable theme.
 */
bool gui_theme_dropdown_index_to_theme(uint16_t index, gui_view_theme_t *theme_out);

/**
 * @brief Convert a selectable theme to its settings dropdown row index.
 *
 * @param theme Theme to search for.
 * @param index_out Output row index, must not be NULL.
 * @return True when theme is user-selectable and an index was written.
 */
bool gui_theme_theme_to_dropdown_index(gui_view_theme_t theme, uint16_t *index_out);

#endif
