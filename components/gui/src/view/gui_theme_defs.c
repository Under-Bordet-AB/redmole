#include "gui_theme_defs.h"

#include <string.h>

#include "lvgl.h"

LV_IMG_DECLARE(hk_bg);
LV_IMG_DECLARE(hk_bg_night);
LV_IMG_DECLARE(terminal_bg);
LV_IMG_DECLARE(deathnote_bg);
LV_IMG_DECLARE(spongebob_bg);
LV_FONT_DECLARE(hellokitty18);
LV_FONT_DECLARE(hellokitty24);
LV_FONT_DECLARE(terminal20);
LV_FONT_DECLARE(terminal26);
LV_FONT_DECLARE(deathnote20);
LV_FONT_DECLARE(deathnote26);
LV_FONT_DECLARE(spongebob18);
LV_FONT_DECLARE(spongebob24);

/*
 * Theme table indexed by gui_view_theme_t enum value.
 * Entries must be in the same order as the enum.
 *
 * To add a new theme:
 *   1. Append a new enum value in gui_types.h (before any sentinel).
 *   2. Increment GUI_THEME_COUNT in gui_theme_defs.h.
 *   3. Add an entry below at the matching index position.
 */
static const gui_theme_def_t gui_themes[GUI_THEME_COUNT] = {

    /* ------------------------------------------------------------------ */
    /* [0] GUI_VIEW_THEME_LIGHT                                            */
    /* ------------------------------------------------------------------ */
    {
        .display_name        = "Light mode",
        .is_user_selectable  = true,
        .has_night_variant   = false,
        .night_variant       = GUI_VIEW_THEME_LIGHT,
        .dialog_has_border   = false,
        .body_font           = &lv_font_montserrat_18,
        .emphasis_font       = &lv_font_montserrat_24,
        .background_image    = NULL,

        .screen_bg           = 0xDCE8F5,
        .screen_grad         = 0xF5F9FF,
        .sidebar_bg          = 0x111827,
        .sidebar_grad        = 0x1E293B,
        .sidebar_shadow      = 0x94A3B8,
        .brand_text          = 0xF8FAFC,
        .content_bg          = 0xFFFFFF,
        .content_shadow      = 0xB8C7DB,
        .title_text          = 0x10213D,
        .subtitle_text       = 0x607089,
        .muted_text          = 0x4A5C78,
        .value_text          = 0x0F172A,
        .panel_bg            = 0xF8FBFF,
        .panel_border        = 0xD9E3F1,
        .card_bg             = 0xFFFFFF,
        .card_border         = 0xD7E1EE,
        .item_bg             = 0xF8FBFF,
        .item_border         = 0xD9E3F1,
        .accent_color        = 0x1D4ED8,
        .accent_soft_color   = 0x1D4ED8,
        .keyboard_bg         = 0xE7EDF5,
        .keyboard_border     = 0xD7E1EE,
        .keyboard_key_bg     = 0xFFFFFF,
        .keyboard_key_text   = 0x10213D,
        .keyboard_special_bg     = 0x1D4ED8,
        .keyboard_special_text   = 0xFFFFFF,
        .keyboard_special_border = 0x1D4ED8,
        .slider_bg           = 0xD9E3F1,
        .slider_knob_bg      = 0xFFFFFF,
        .dropdown_bg         = 0xFFFFFF,
        .dropdown_border     = 0xD7E1EE,
        .dropdown_selected_bg   = 0x1D4ED8,
        .dropdown_selected_text = 0xFFFFFF,
        .energy_chart_bg     = 0xFFFFFF,
        .energy_chart_grid   = 0xD9E3F1,
        .energy_chart_tick   = 0x607089,
        .energy_buy_color    = 0x1D4ED8,
        .energy_solar_color  = 0xF59E0B,
        .energy_charge_color = 0x10B981,
        .energy_sell_color   = 0xEF4444,
        .wifi_connected_color = 0x1D4ED8,
        .wifi_idle_color      = 0x4A5C78,
        .nav_active_bg       = 0xE8F0FF,
        .nav_active_text     = 0x10213D,
        .nav_active_border   = 0x8FB3FF,
        .nav_inactive_bg     = 0x1B2437,
        .nav_inactive_text   = 0xDCE6F5,
        .nav_inactive_border = 0x2A3954,
        .action_primary_bg     = 0x1D4ED8,
        .action_primary_text   = 0xFFFFFF,
        .action_primary_border = 0x1D4ED8,
        .action_secondary_bg     = 0xFFFFFF,
        .action_secondary_text   = 0x10213D,
        .action_secondary_border = 0xD7E1EE,
        .wifi_btn_bg             = 0xFFFFFF,
        .wifi_btn_border         = 0xD7E1EE,
        .wifi_btn_text           = 0x334155,
        .wifi_btn_connected_bg   = 0xDCFCE7,
        .wifi_btn_connected_border = 0x22C55E,
        .wifi_btn_connected_text   = 0x14532D,
        .wifi_btn_known_bg       = 0xF0FDF4,
        .wifi_btn_known_border   = 0x86EFAC,
        .wifi_btn_known_text     = 0x166534,
        .wifi_btn_selected_bg     = 0xE6F0FF,
        .wifi_btn_selected_border = 0x7CA6F8,
        .wifi_btn_selected_text   = 0x123364,
    },

    /* ------------------------------------------------------------------ */
    /* [1] GUI_VIEW_THEME_DARK                                             */
    /* ------------------------------------------------------------------ */
    {
        .display_name        = "Dark mode",
        .is_user_selectable  = true,
        .has_night_variant   = false,
        .night_variant       = GUI_VIEW_THEME_DARK,
        .dialog_has_border   = false,
        .body_font           = &lv_font_montserrat_18,
        .emphasis_font       = &lv_font_montserrat_24,
        .background_image    = NULL,

        .screen_bg           = 0x0B1220,
        .screen_grad         = 0x172033,
        .sidebar_bg          = 0x020617,
        .sidebar_grad        = 0x111827,
        .sidebar_shadow      = 0x020617,
        .brand_text          = 0xF8FAFC,
        .content_bg          = 0x111827,
        .content_shadow      = 0x020617,
        .title_text          = 0xF8FAFC,
        .subtitle_text       = 0xAAB7C8,
        .muted_text          = 0xCBD5E1,
        .value_text          = 0xF8FAFC,
        .panel_bg            = 0x1E293B,
        .panel_border        = 0x334155,
        .card_bg             = 0x0F172A,
        .card_border         = 0x334155,
        .item_bg             = 0x172033,
        .item_border         = 0x334155,
        .accent_color        = 0x60A5FA,
        .accent_soft_color   = 0x93C5FD,
        .keyboard_bg         = 0x243244,
        .keyboard_border     = 0x334155,
        .keyboard_key_bg     = 0x334155,
        .keyboard_key_text   = 0xF8FAFC,
        .keyboard_special_bg     = 0x2563EB,
        .keyboard_special_text   = 0xF8FAFC,
        .keyboard_special_border = 0x60A5FA,
        .slider_bg           = 0x334155,
        .slider_knob_bg      = 0xE2E8F0,
        .dropdown_bg         = 0x172033,
        .dropdown_border     = 0x475569,
        .dropdown_selected_bg   = 0x2563EB,
        .dropdown_selected_text = 0xF8FAFC,
        .energy_chart_bg     = 0x0F172A,
        .energy_chart_grid   = 0x334155,
        .energy_chart_tick   = 0xAAB7C8,
        .energy_buy_color    = 0x60A5FA,
        .energy_solar_color  = 0xFCD34D,
        .energy_charge_color = 0x34D399,
        .energy_sell_color   = 0xF87171,
        .wifi_connected_color = 0x60A5FA,
        .wifi_idle_color      = 0xCBD5E1,
        .nav_active_bg       = 0x1D4ED8,
        .nav_active_text     = 0xF8FAFC,
        .nav_active_border   = 0x60A5FA,
        .nav_inactive_bg     = 0x0F172A,
        .nav_inactive_text   = 0xCBD5E1,
        .nav_inactive_border = 0x334155,
        .action_primary_bg     = 0x2563EB,
        .action_primary_text   = 0xF8FAFC,
        .action_primary_border = 0x60A5FA,
        .action_secondary_bg     = 0x172033,
        .action_secondary_text   = 0xD7E3F4,
        .action_secondary_border = 0x475569,
        .wifi_btn_bg             = 0x182334,
        .wifi_btn_border         = 0x314155,
        .wifi_btn_text           = 0xD7E3F4,
        .wifi_btn_connected_bg   = 0x153528,
        .wifi_btn_connected_border = 0x4ADE80,
        .wifi_btn_connected_text   = 0xBBF7D0,
        .wifi_btn_known_bg       = 0x162D22,
        .wifi_btn_known_border   = 0x86EFAC,
        .wifi_btn_known_text     = 0xD1FAE5,
        .wifi_btn_selected_bg     = 0x1A2B45,
        .wifi_btn_selected_border = 0x60A5FA,
        .wifi_btn_selected_text   = 0xBFDBFE,
    },

    /* ------------------------------------------------------------------ */
    /* [2] GUI_VIEW_THEME_HELLO_KITTY                                      */
    /* ------------------------------------------------------------------ */
    {
        .display_name        = "Hello Kitty",
        .is_user_selectable  = true,
        .has_night_variant   = true,
        .night_variant       = GUI_VIEW_THEME_HELLO_KITTY_NIGHT,
        .dialog_has_border   = true,
        .body_font           = &hellokitty18,
        .emphasis_font       = &hellokitty24,
        .background_image    = &hk_bg,

        .screen_bg           = 0xFFDDE8,
        .screen_grad         = 0xFFF7FB,
        .sidebar_bg          = 0xFFF0F5,
        .sidebar_grad        = 0xFFE3EC,
        .sidebar_shadow      = 0xF4A3BE,
        .brand_text          = 0xC2185B,
        .content_bg          = 0xFFFDFE,
        .content_shadow      = 0xF6B8CC,
        .title_text          = 0x8A1D47,
        .subtitle_text       = 0xA65374,
        .muted_text          = 0xB0597E,
        .value_text          = 0x8A1D47,
        .panel_bg            = 0xFFF5F8,
        .panel_border        = 0xF6BDD0,
        .card_bg             = 0xFFFFFF,
        .card_border         = 0xF7C9D8,
        .item_bg             = 0xFFF8FB,
        .item_border         = 0xF7C9D8,
        .accent_color        = 0xFB7185,
        .accent_soft_color   = 0xF472B6,
        .keyboard_bg         = 0xFFD8E6,
        .keyboard_border     = 0xF4A3BE,
        .keyboard_key_bg     = 0xFFF5F8,
        .keyboard_key_text   = 0x8A1D47,
        .keyboard_special_bg     = 0xFB7185,
        .keyboard_special_text   = 0xFFFDFE,
        .keyboard_special_border = 0xF472B6,
        .slider_bg           = 0xF6C1D4,
        .slider_knob_bg      = 0xFFFFFF,
        .dropdown_bg         = 0xFFF8FB,
        .dropdown_border     = 0xF4A3BE,
        .dropdown_selected_bg   = 0xFB7185,
        .dropdown_selected_text = 0xFFFDFE,
        .energy_chart_bg     = 0xFFFFFF,
        .energy_chart_grid   = 0xF6BDD0,
        .energy_chart_tick   = 0xA65374,
        .energy_buy_color    = 0xEC4899,
        .energy_solar_color  = 0xFCD34D,
        .energy_charge_color = 0xA7F3D0,
        .energy_sell_color   = 0xF43F5E,
        .wifi_connected_color = 0xFB7185,
        .wifi_idle_color      = 0xB0597E,
        .nav_active_bg       = 0xFFE0EB,
        .nav_active_text     = 0x8A1D47,
        .nav_active_border   = 0xFB7185,
        .nav_inactive_bg     = 0xFFF5F8,
        .nav_inactive_text   = 0xA13A64,
        .nav_inactive_border = 0xF4A3BE,
        .action_primary_bg     = 0xFB7185,
        .action_primary_text   = 0xFFFDFE,
        .action_primary_border = 0xF472B6,
        .action_secondary_bg     = 0xFFF0F6,
        .action_secondary_text   = 0x8A1D47,
        .action_secondary_border = 0xF4A3BE,
        .wifi_btn_bg             = 0xFFF7FB,
        .wifi_btn_border         = 0xF9B7CD,
        .wifi_btn_text           = 0x8A284E,
        .wifi_btn_connected_bg   = 0xFFE7EF,
        .wifi_btn_connected_border = 0xFF6B9A,
        .wifi_btn_connected_text   = 0x8A1D47,
        .wifi_btn_known_bg       = 0xFFF0F6,
        .wifi_btn_known_border   = 0xF9A8C4,
        .wifi_btn_known_text     = 0x9D174D,
        .wifi_btn_selected_bg     = 0xFFE2EC,
        .wifi_btn_selected_border = 0xFB7185,
        .wifi_btn_selected_text   = 0x881337,
    },

    /* ------------------------------------------------------------------ */
    /* [3] GUI_VIEW_THEME_TERMINAL                                         */
    /* ------------------------------------------------------------------ */
    {
        .display_name        = "Terminal",
        .is_user_selectable  = true,
        .has_night_variant   = false,
        .night_variant       = GUI_VIEW_THEME_TERMINAL,
        .dialog_has_border   = true,
        .body_font           = &terminal20,
        .emphasis_font       = &terminal26,
        .background_image    = &terminal_bg,

        .screen_bg           = 0x051316,
        .screen_grad         = 0x0A2024,
        .sidebar_bg          = 0x041014,
        .sidebar_grad        = 0x0A2329,
        .sidebar_shadow      = 0x0D7B80,
        .brand_text          = 0x03F5FA,
        .content_bg          = 0x08171B,
        .content_shadow      = 0x0E6A70,
        .title_text          = 0x03F5FA,
        .subtitle_text       = 0x7CEBED,
        .muted_text          = 0x69DDE0,
        .value_text          = 0xD8FEFF,
        .panel_bg            = 0x0A1B20,
        .panel_border        = 0x12848A,
        .card_bg             = 0x061419,
        .card_border         = 0x0E5E63,
        .item_bg             = 0x0B2127,
        .item_border         = 0x12848A,
        .accent_color        = 0x03F5FA,
        .accent_soft_color   = 0x9BFCFF,
        .keyboard_bg         = 0x08171B,
        .keyboard_border     = 0x0E5E63,
        .keyboard_key_bg     = 0x0A1B20,
        .keyboard_key_text   = 0x9BFCFF,
        .keyboard_special_bg     = 0x03F5FA,
        .keyboard_special_text   = 0x031215,
        .keyboard_special_border = 0x9BFCFF,
        .slider_bg           = 0x16434A,
        .slider_knob_bg      = 0x03F5FA,
        .dropdown_bg         = 0x08171B,
        .dropdown_border     = 0x12848A,
        .dropdown_selected_bg   = 0x03F5FA,
        .dropdown_selected_text = 0x031215,
        .energy_chart_bg     = 0x061419,
        .energy_chart_grid   = 0x0E5E63,
        .energy_chart_tick   = 0x69DDE0,
        .energy_buy_color    = 0x03F5FA,
        .energy_solar_color  = 0xB7FF5A,
        .energy_charge_color = 0x3CFFE4,
        .energy_sell_color   = 0xFF6B8B,
        .wifi_connected_color = 0xFFA206,
        .wifi_idle_color      = 0x69DDE0,
        .nav_active_bg       = 0x03F5FA,
        .nav_active_text     = 0x031215,
        .nav_active_border   = 0x9BFCFF,
        .nav_inactive_bg     = 0x0A1B20,
        .nav_inactive_text   = 0xC9FEFF,
        .nav_inactive_border = 0x12848A,
        .action_primary_bg     = 0x03F5FA,
        .action_primary_text   = 0x031215,
        .action_primary_border = 0x9BFCFF,
        .action_secondary_bg     = 0x0A1B20,
        .action_secondary_text   = 0xC9FEFF,
        .action_secondary_border = 0x12848A,
        .wifi_btn_bg             = 0x08171B,
        .wifi_btn_border         = 0x12848A,
        .wifi_btn_text           = 0x9BFCFF,
        .wifi_btn_connected_bg   = 0x0A1B20,
        .wifi_btn_connected_border = 0x03F5FA,
        .wifi_btn_connected_text   = 0xD8FEFF,
        .wifi_btn_known_bg       = 0x061419,
        .wifi_btn_known_border   = 0x0E5E63,
        .wifi_btn_known_text     = 0x9BFCFF,
        .wifi_btn_selected_bg     = 0x0B2127,
        .wifi_btn_selected_border = 0x03F5FA,
        .wifi_btn_selected_text   = 0xD8FEFF,
    },

    /* ------------------------------------------------------------------ */
    /* [4] GUI_VIEW_THEME_HELLO_KITTY_NIGHT                                */
    /* Internal variant — not shown directly in the settings dropdown.     */
    /* Activated by the night-mode toggle when Hello Kitty is selected.    */
    /* ------------------------------------------------------------------ */
    {
        .display_name        = "Hello Kitty Night",
        .is_user_selectable  = false,
        .has_night_variant   = false,
        .night_variant       = GUI_VIEW_THEME_HELLO_KITTY_NIGHT,
        .dialog_has_border   = true,
        .body_font           = &hellokitty18,
        .emphasis_font       = &hellokitty24,
        .background_image    = &hk_bg_night,

        .screen_bg           = 0x1A1830,
        .screen_grad         = 0x16142B,
        .sidebar_bg          = 0x1C1A30,
        .sidebar_grad        = 0x16142B,
        .sidebar_shadow      = 0x0D0C1A,
        .brand_text          = 0xE8D8F0,
        .content_bg          = 0x1E1C34,
        .content_shadow      = 0x0D0C1A,
        .title_text          = 0xE8D8F0,
        .subtitle_text       = 0xA890B8,
        .muted_text          = 0xA890B8,
        .value_text          = 0xE8D8F0,
        .panel_bg            = 0x222040,
        .panel_border        = 0x3A3660,
        .card_bg             = 0x2A2744,
        .card_border         = 0x3A3660,
        .item_bg             = 0x242240,
        .item_border         = 0x3A3660,
        .accent_color        = 0xD46A80,
        .accent_soft_color   = 0xC97090,
        .keyboard_bg         = 0x1E1C34,
        .keyboard_border     = 0x3A3660,
        .keyboard_key_bg     = 0x2A2744,
        .keyboard_key_text   = 0xE8D8F0,
        .keyboard_special_bg     = 0xD46A80,
        .keyboard_special_text   = 0xF0E0F0,
        .keyboard_special_border = 0xC97090,
        .slider_bg           = 0x3A3660,
        .slider_knob_bg      = 0xE8D8F0,
        .dropdown_bg         = 0x2A2744,
        .dropdown_border     = 0x3A3660,
        .dropdown_selected_bg   = 0xD46A80,
        .dropdown_selected_text = 0xF0E0F0,
        .energy_chart_bg     = 0x2A2744,
        .energy_chart_grid   = 0x3A3660,
        .energy_chart_tick   = 0xA890B8,
        .energy_buy_color    = 0xC97090,
        .energy_solar_color  = 0xD4965A,
        .energy_charge_color = 0x8A9BD0,
        .energy_sell_color   = 0xD46A80,
        .wifi_connected_color = 0xD46A80,
        .wifi_idle_color      = 0x6B6080,
        .nav_active_bg       = 0x3A3660,
        .nav_active_text     = 0xE8D8F0,
        .nav_active_border   = 0xD46A80,
        .nav_inactive_bg     = 0x2A2744,
        .nav_inactive_text   = 0xA890B8,
        .nav_inactive_border = 0x3A3660,
        .action_primary_bg     = 0xD46A80,
        .action_primary_text   = 0xF0E0F0,
        .action_primary_border = 0xD46A80,
        .action_secondary_bg     = 0x2A2744,
        .action_secondary_text   = 0xA890B8,
        .action_secondary_border = 0x3A3660,
        .wifi_btn_bg             = 0x2A2744,
        .wifi_btn_border         = 0x3A3660,
        .wifi_btn_text           = 0xA890B8,
        .wifi_btn_connected_bg   = 0x222040,
        .wifi_btn_connected_border = 0xD46A80,
        .wifi_btn_connected_text   = 0xE8D8F0,
        .wifi_btn_known_bg       = 0x222040,
        .wifi_btn_known_border   = 0xC97090,
        .wifi_btn_known_text     = 0xE8D8F0,
        .wifi_btn_selected_bg     = 0x2A2744,
        .wifi_btn_selected_border = 0xD46A80,
        .wifi_btn_selected_text   = 0xE8D8F0,
    },

    /* ------------------------------------------------------------------ */
    /* [5] GUI_VIEW_THEME_DEATH_NOTE                                       */
    /* ------------------------------------------------------------------ */
    {
        .display_name        = "Death Note",
        .is_user_selectable  = true,
        .has_night_variant   = false,
        .night_variant       = GUI_VIEW_THEME_DEATH_NOTE,
        .dialog_has_border   = true,
        .body_font           = &deathnote20,
        .emphasis_font       = &deathnote26,
        .background_image    = &deathnote_bg,

        .screen_bg           = 0x020304,
        .screen_grad         = 0x0A0B0D,
        .sidebar_bg          = 0x090A0C,
        .sidebar_grad        = 0x121316,
        .sidebar_shadow      = 0x2A2D33,
        .brand_text          = 0xE8E8EA,
        .content_bg          = 0x0B0C0F,
        .content_shadow      = 0x23262C,
        .title_text          = 0xF2F2F4,
        .subtitle_text       = 0xB7B9BF,
        .muted_text          = 0x8D9098,
        .value_text          = 0xE5E7EB,
        .panel_bg            = 0x111317,
        .panel_border        = 0x2A2D33,
        .card_bg             = 0x0D0F12,
        .card_border         = 0x2E323A,
        .item_bg             = 0x14171C,
        .item_border         = 0x353A43,
        .accent_color        = 0x8E1B25,
        .accent_soft_color   = 0xB33A45,
        .keyboard_bg         = 0x0E1013,
        .keyboard_border     = 0x2D3138,
        .keyboard_key_bg     = 0x181B21,
        .keyboard_key_text   = 0xD6D9DF,
        .keyboard_special_bg     = 0x8E1B25,
        .keyboard_special_text   = 0xF3EDEE,
        .keyboard_special_border = 0xB33A45,
        .slider_bg           = 0x3A3F49,
        .slider_knob_bg      = 0xD9DCE2,
        .dropdown_bg         = 0x0F1115,
        .dropdown_border     = 0x343943,
        .dropdown_selected_bg   = 0x8E1B25,
        .dropdown_selected_text = 0xF6F1F2,
        .energy_chart_bg     = 0x0C0E11,
        .energy_chart_grid   = 0x2B3038,
        .energy_chart_tick   = 0x9A9EA8,
        .energy_buy_color    = 0xBFC6D2,
        .energy_solar_color  = 0xCBAA69,
        .energy_charge_color = 0x7FA39A,
        .energy_sell_color   = 0xB33A45,
        .wifi_connected_color = 0xB33A45,
        .wifi_idle_color      = 0x8D9098,
        .nav_active_bg       = 0x8E1B25,
        .nav_active_text     = 0xF6F1F2,
        .nav_active_border   = 0xB33A45,
        .nav_inactive_bg     = 0x111317,
        .nav_inactive_text   = 0xC3C7CF,
        .nav_inactive_border = 0x2F343D,
        .action_primary_bg     = 0x8E1B25,
        .action_primary_text   = 0xF6F1F2,
        .action_primary_border = 0xB33A45,
        .action_secondary_bg     = 0x13161B,
        .action_secondary_text   = 0xD0D4DC,
        .action_secondary_border = 0x353A43,
        .wifi_btn_bg             = 0x111318,
        .wifi_btn_border         = 0x343943,
        .wifi_btn_text           = 0xD0D4DC,
        .wifi_btn_connected_bg   = 0x1A1A1E,
        .wifi_btn_connected_border = 0x8E1B25,
        .wifi_btn_connected_text   = 0xF1E4E6,
        .wifi_btn_known_bg       = 0x171A1F,
        .wifi_btn_known_border   = 0x4A505A,
        .wifi_btn_known_text     = 0xC8CCD5,
        .wifi_btn_selected_bg     = 0x221419,
        .wifi_btn_selected_border = 0xB33A45,
        .wifi_btn_selected_text   = 0xF6F1F2,
    },

    /* ------------------------------------------------------------------ */
    /* [6] GUI_VIEW_THEME_SPONGEBOB                                        */
    /* ------------------------------------------------------------------ */
    {
        .display_name        = "Spongebob",
        .is_user_selectable  = true,
        .has_night_variant   = false,
        .night_variant       = GUI_VIEW_THEME_SPONGEBOB,
        .dialog_has_border   = true,
        .body_font           = &spongebob18,
        .emphasis_font       = &spongebob24,
        .background_image    = &spongebob_bg,

        .screen_bg           = 0xFFF17A,
        .screen_grad         = 0xFFF9C4,
        .sidebar_bg          = 0x0F3B66,
        .sidebar_grad        = 0x1F5A94,
        .sidebar_shadow      = 0x2B6FA8,
        .brand_text          = 0xFFE066,
        .content_bg          = 0xFFFDE8,
        .content_shadow      = 0xE6D27A,
        .title_text          = 0x6B3F06,
        .subtitle_text       = 0x8B5A11,
        .muted_text          = 0x6F7F8F,
        .value_text          = 0x3F2D10,
        .panel_bg            = 0xFFF4A3,
        .panel_border        = 0xD9C15B,
        .card_bg             = 0xFFF8C2,
        .card_border         = 0xE0C866,
        .item_bg             = 0xFFFCE0,
        .item_border         = 0xD8C56A,
        .accent_color        = 0x00AEEF,
        .accent_soft_color   = 0x65D3FF,
        .keyboard_bg         = 0xFFE889,
        .keyboard_border     = 0xD8C15A,
        .keyboard_key_bg     = 0xFFF6C8,
        .keyboard_key_text   = 0x4A2F0A,
        .keyboard_special_bg     = 0x00AEEF,
        .keyboard_special_text   = 0xFFFFFF,
        .keyboard_special_border = 0x007EB0,
        .slider_bg           = 0xD8C15A,
        .slider_knob_bg      = 0xFFFFFF,
        .dropdown_bg         = 0xFFF9D5,
        .dropdown_border     = 0xD8C15A,
        .dropdown_selected_bg   = 0x00AEEF,
        .dropdown_selected_text = 0xFFFFFF,
        .energy_chart_bg     = 0xFFFCE6,
        .energy_chart_grid   = 0xD6C163,
        .energy_chart_tick   = 0x6A7380,
        .energy_buy_color    = 0x00AEEF,
        .energy_solar_color  = 0xF39B17,
        .energy_charge_color = 0x26C485,
        .energy_sell_color   = 0xE85D75,
        .wifi_connected_color = 0x00AEEF,
        .wifi_idle_color      = 0x6F7F8F,
        .nav_active_bg       = 0x00AEEF,
        .nav_active_text     = 0xFFFFFF,
        .nav_active_border   = 0x007EB0,
        .nav_inactive_bg     = 0x13446F,
        .nav_inactive_text   = 0xE1F4FF,
        .nav_inactive_border = 0x2C679F,
        .action_primary_bg     = 0x00AEEF,
        .action_primary_text   = 0xFFFFFF,
        .action_primary_border = 0x007EB0,
        .action_secondary_bg     = 0xFFF7C7,
        .action_secondary_text   = 0x5B3A0D,
        .action_secondary_border = 0xD6C05F,
        .wifi_btn_bg             = 0xFFFADB,
        .wifi_btn_border         = 0xD6C05F,
        .wifi_btn_text           = 0x4D320B,
        .wifi_btn_connected_bg   = 0xD9F2FF,
        .wifi_btn_connected_border = 0x00AEEF,
        .wifi_btn_connected_text   = 0x0A4F74,
        .wifi_btn_known_bg       = 0xFFF0B4,
        .wifi_btn_known_border   = 0xE0B94A,
        .wifi_btn_known_text     = 0x5C3A0D,
        .wifi_btn_selected_bg     = 0xC7EEFF,
        .wifi_btn_selected_border = 0x00AEEF,
        .wifi_btn_selected_text   = 0x0A4F74,
    },
};

const gui_theme_def_t *gui_theme_get(gui_view_theme_t theme)
{
    if (((int)theme < 0) || ((uint32_t)theme >= GUI_THEME_COUNT)) {
        return NULL;
    }

    return &gui_themes[(uint32_t)theme];
}

void gui_theme_build_dropdown_string(char *buf, size_t buf_size)
{
    bool first = true;
    uint32_t index;

    if ((buf == NULL) || (buf_size == 0)) {
        return;
    }

    buf[0] = '\0';

    for (index = 0; index < GUI_THEME_COUNT; index++) {
        if (!gui_themes[index].is_user_selectable) {
            continue;
        }

        if (!first) {
            strncat(buf, "\n", buf_size - strlen(buf) - 1);
        }

        strncat(buf, gui_themes[index].display_name, buf_size - strlen(buf) - 1);
        first = false;
    }
}

bool gui_theme_dropdown_index_to_theme(uint16_t index, gui_view_theme_t *theme_out)
{
    uint16_t selectable_index = 0;
    uint32_t theme_index;

    if (theme_out == NULL) {
        return false;
    }

    for (theme_index = 0; theme_index < GUI_THEME_COUNT; theme_index++) {
        if (!gui_themes[theme_index].is_user_selectable) {
            continue;
        }

        if (selectable_index == index) {
            *theme_out = (gui_view_theme_t)theme_index;
            return true;
        }

        selectable_index++;
    }

    return false;
}

bool gui_theme_theme_to_dropdown_index(gui_view_theme_t theme, uint16_t *index_out)
{
    uint16_t selectable_index = 0;
    uint32_t theme_index;

    if ((index_out == NULL) || ((int)theme < 0) || ((uint32_t)theme >= GUI_THEME_COUNT) ||
        !gui_themes[(uint32_t)theme].is_user_selectable) {
        return false;
    }

    for (theme_index = 0; theme_index < GUI_THEME_COUNT; theme_index++) {
        if (!gui_themes[theme_index].is_user_selectable) {
            continue;
        }

        if (theme_index == (uint32_t)theme) {
            *index_out = selectable_index;
            return true;
        }

        selectable_index++;
    }

    return false;
}
