"use strict";

const STORAGE_PREFIX = "redmole-theme-editor:v1:";

const metadataFields = [
    "display_name",
    "is_user_selectable",
    "has_night_variant",
    "night_variant",
    "dialog_has_border",
    "body_font",
    "emphasis_font",
    "background_image",
];

const colorGroups = [
    {
        title: "Screen",
        fields: ["screen_bg", "screen_grad"],
    },
    {
        title: "Sidebar",
        fields: ["sidebar_bg", "sidebar_grad", "sidebar_shadow", "brand_text"],
    },
    {
        title: "Content",
        fields: ["content_bg", "content_shadow"],
    },
    {
        title: "Typography",
        fields: ["title_text", "subtitle_text", "muted_text", "value_text"],
    },
    {
        title: "Panels and Cards",
        fields: ["panel_bg", "panel_border", "card_bg", "card_border", "item_bg", "item_border"],
    },
    {
        title: "Accents",
        fields: ["accent_color", "accent_soft_color"],
    },
    {
        title: "Keyboard",
        fields: [
            "keyboard_bg",
            "keyboard_border",
            "keyboard_key_bg",
            "keyboard_key_text",
            "keyboard_special_bg",
            "keyboard_special_text",
            "keyboard_special_border",
        ],
    },
    {
        title: "Slider and Dropdown",
        fields: [
            "slider_bg",
            "slider_knob_bg",
            "dropdown_bg",
            "dropdown_border",
            "dropdown_selected_bg",
            "dropdown_selected_text",
        ],
    },
    {
        title: "Energy Chart",
        fields: [
            "energy_chart_bg",
            "energy_chart_grid",
            "energy_chart_tick",
            "energy_buy_color",
            "energy_solar_color",
            "energy_charge_color",
            "energy_sell_color",
        ],
    },
    {
        title: "Status",
        fields: ["wifi_connected_color", "wifi_idle_color"],
    },
    {
        title: "Navigation",
        fields: [
            "nav_active_bg",
            "nav_active_text",
            "nav_active_border",
            "nav_inactive_bg",
            "nav_inactive_text",
            "nav_inactive_border",
        ],
    },
    {
        title: "Actions",
        fields: [
            "action_primary_bg",
            "action_primary_text",
            "action_primary_border",
            "action_secondary_bg",
            "action_secondary_text",
            "action_secondary_border",
        ],
    },
    {
        title: "Wi-Fi Buttons",
        fields: [
            "wifi_btn_bg",
            "wifi_btn_border",
            "wifi_btn_text",
            "wifi_btn_connected_bg",
            "wifi_btn_connected_border",
            "wifi_btn_connected_text",
            "wifi_btn_known_bg",
            "wifi_btn_known_border",
            "wifi_btn_known_text",
            "wifi_btn_selected_bg",
            "wifi_btn_selected_border",
            "wifi_btn_selected_text",
        ],
    },
];

const colorFields = colorGroups.flatMap((group) => group.fields);
const allFields = [...metadataFields, ...colorFields];
const colorFieldSet = new Set(colorFields);
const boolFieldSet = new Set(["is_user_selectable", "has_night_variant", "dialog_has_border"]);

const themeEnums = [
    "GUI_VIEW_THEME_LIGHT",
    "GUI_VIEW_THEME_DARK",
    "GUI_VIEW_THEME_HELLO_KITTY",
    "GUI_VIEW_THEME_TERMINAL",
    "GUI_VIEW_THEME_HELLO_KITTY_NIGHT",
    "GUI_VIEW_THEME_DEATH_NOTE",
    "GUI_VIEW_THEME_SPONGEBOB",
    "GUI_VIEW_THEME_BONZI_BUDDY",
];

const themeSources = [
    {
        id: "GUI_VIEW_THEME_LIGHT",
        source: `
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
        `,
    },
    {
        id: "GUI_VIEW_THEME_DARK",
        source: `
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
        `,
    },
    {
        id: "GUI_VIEW_THEME_HELLO_KITTY",
        source: `
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
        `,
    },
    {
        id: "GUI_VIEW_THEME_TERMINAL",
        source: `
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
        `,
    },
    {
        id: "GUI_VIEW_THEME_HELLO_KITTY_NIGHT",
        source: `
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
        `,
    },
    {
        id: "GUI_VIEW_THEME_DEATH_NOTE",
        source: `
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
        `,
    },
    {
        id: "GUI_VIEW_THEME_SPONGEBOB",
        source: `
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
        `,
    },
    {
        id: "GUI_VIEW_THEME_BONZI_BUDDY",
        source: `
        .display_name        = "BonziBuddy",
        .is_user_selectable  = true,
        .has_night_variant   = false,
        .night_variant       = GUI_VIEW_THEME_BONZI_BUDDY,
        .dialog_has_border   = true,
        .body_font           = &bonzibuddy18,
        .emphasis_font       = &bonzibuddy24,
        .background_image    = &bonzibuddy_bg,
        .screen_bg           = 0x3A6EA5,
        .screen_grad         = 0x7FB5FF,
        .sidebar_bg          = 0x245EDC,
        .sidebar_grad        = 0x003399,
        .sidebar_shadow      = 0x003399,
        .brand_text          = 0xFFFFFF,
        .content_bg          = 0xECE9D8,
        .content_shadow      = 0x316AC5,
        .title_text          = 0x003399,
        .subtitle_text       = 0x4B5E7A,
        .muted_text          = 0xE0E0E0,
        .value_text          = 0x1F1F1F,
        .panel_bg            = 0xF5F4EA,
        .panel_border        = 0xD4D0C8,
        .card_bg             = 0xFFFFFF,
        .card_border         = 0xACA899,
        .item_bg             = 0xECE9D8,
        .item_border         = 0xD4D0C8,
        .accent_color        = 0x3C8D2F,
        .accent_soft_color   = 0x73B64A,
        .keyboard_bg         = 0xD4D0C8,
        .keyboard_border     = 0xACA899,
        .keyboard_key_bg     = 0xF5F4EA,
        .keyboard_key_text   = 0x1F1F1F,
        .keyboard_special_bg     = 0x316AC5,
        .keyboard_special_text   = 0xFFFFFF,
        .keyboard_special_border = 0x003399,
        .slider_bg           = 0xD4D0C8,
        .slider_knob_bg      = 0xFFFFFF,
        .dropdown_bg         = 0xF5F4EA,
        .dropdown_border     = 0xACA899,
        .dropdown_selected_bg   = 0x316AC5,
        .dropdown_selected_text = 0xFFFFFF,
        .energy_chart_bg     = 0xFFFFFF,
        .energy_chart_grid   = 0xD4D0C8,
        .energy_chart_tick   = 0x4B5E7A,
        .energy_buy_color    = 0x245EDC,
        .energy_solar_color  = 0xFFB000,
        .energy_charge_color = 0x3C8D2F,
        .energy_sell_color   = 0xCC3333,
        .wifi_connected_color = 0x3C8D2F,
        .wifi_idle_color      = 0x5F5F5F,
        .nav_active_bg       = 0x316AC5,
        .nav_active_text     = 0xFFFFFF,
        .nav_active_border   = 0x7FB5FF,
        .nav_inactive_bg     = 0x245EDC,
        .nav_inactive_text   = 0xFFFFFF,
        .nav_inactive_border = 0x003399,
        .action_primary_bg     = 0x3C8D2F,
        .action_primary_text   = 0xFFFFFF,
        .action_primary_border = 0x2E6F24,
        .action_secondary_bg     = 0xF5F4EA,
        .action_secondary_text   = 0x003399,
        .action_secondary_border = 0xACA899,
        .wifi_btn_bg             = 0xFFFFFF,
        .wifi_btn_border         = 0xACA899,
        .wifi_btn_text           = 0x1F1F1F,
        .wifi_btn_connected_bg   = 0xF5F4EA,
        .wifi_btn_connected_border = 0x3C8D2F,
        .wifi_btn_connected_text   = 0x2E6F24,
        .wifi_btn_known_bg       = 0xECE9D8,
        .wifi_btn_known_border   = 0x316AC5,
        .wifi_btn_known_text     = 0x003399,
        .wifi_btn_selected_bg     = 0x7FB5FF,
        .wifi_btn_selected_border = 0x245EDC,
        .wifi_btn_selected_text   = 0x003399,
        `,
    },
];

const defaultsById = new Map(
    themeSources.map((theme) => [theme.id, parseInitializer(theme.source)])
);

const elements = {
    themeSelect: document.getElementById("themeSelect"),
    activeThemeLabel: document.getElementById("activeThemeLabel"),
    resetThemeButton: document.getElementById("resetThemeButton"),
    metadataControls: document.getElementById("metadataControls"),
    colorControls: document.getElementById("colorControls"),
    importText: document.getElementById("importText"),
    importButton: document.getElementById("importButton"),
    importStatus: document.getElementById("importStatus"),
    exportText: document.getElementById("exportText"),
    copyExportButton: document.getElementById("copyExportButton"),
    copyStatus: document.getElementById("copyStatus"),
    previewDisplayName: document.querySelector("[data-preview-display-name]"),
};

let activeThemeId = themeSources[0].id;
let activeTheme = loadTheme(activeThemeId);

init();

function init() {
    renderThemeSelect();
    renderMetadataControls();
    renderColorControls();
    bindStaticEvents();
    applyTheme();
}

function bindStaticEvents() {
    elements.themeSelect.addEventListener("change", () => {
        activeThemeId = elements.themeSelect.value;
        activeTheme = loadTheme(activeThemeId);
        renderMetadataControls();
        renderColorControls();
        applyTheme();
    });

    elements.resetThemeButton.addEventListener("click", () => {
        activeTheme = cloneTheme(defaultsById.get(activeThemeId));
        removeStoredTheme(activeThemeId);
        renderMetadataControls();
        renderColorControls();
        applyTheme();
        setStatus(elements.importStatus, "Reset");
    });

    elements.importButton.addEventListener("click", () => {
        const imported = parseInitializer(elements.importText.value);
        const importFields = Object.keys(imported);

        if (importFields.length === 0) {
            setStatus(elements.importStatus, "No fields found");
            return;
        }

        for (const field of importFields) {
            if (allFields.includes(field)) {
                activeTheme[field] = imported[field];
            }
        }

        saveTheme();
        renderThemeSelect();
        renderMetadataControls();
        renderColorControls();
        applyTheme();
        setStatus(elements.importStatus, `Imported ${importFields.length} fields`);
    });

    elements.copyExportButton.addEventListener("click", async () => {
        const text = elements.exportText.value;

        try {
            await navigator.clipboard.writeText(text);
            setStatus(elements.copyStatus, "Copied");
        } catch (error) {
            elements.exportText.focus();
            elements.exportText.select();
            document.execCommand("copy");
            setStatus(elements.copyStatus, "Copied");
        }
    });
}

function renderThemeSelect() {
    const selected = activeThemeId;
    elements.themeSelect.innerHTML = "";

    for (const theme of themeSources) {
        const fields = loadTheme(theme.id);
        const option = document.createElement("option");
        option.value = theme.id;
        option.textContent = `${fields.display_name || theme.id} (${theme.id})`;
        elements.themeSelect.appendChild(option);
    }

    elements.themeSelect.value = selected;
}

function renderMetadataControls() {
    elements.metadataControls.innerHTML = "";

    for (const field of metadataFields) {
        const wrapper = document.createElement("div");
        wrapper.className = "metadata-control";

        if (boolFieldSet.has(field)) {
            wrapper.className += " checkbox-line";

            const input = document.createElement("input");
            input.type = "checkbox";
            input.id = `meta-${field}`;
            input.checked = Boolean(activeTheme[field]);
            input.addEventListener("change", () => {
                activeTheme[field] = input.checked;
                saveTheme();
                applyTheme();
            });

            const label = document.createElement("label");
            label.htmlFor = input.id;
            label.textContent = field;

            wrapper.append(input, label);
        } else if (field === "night_variant") {
            const label = document.createElement("label");
            label.htmlFor = `meta-${field}`;
            label.textContent = field;

            const select = document.createElement("select");
            select.id = `meta-${field}`;
            for (const enumValue of themeEnums) {
                const option = document.createElement("option");
                option.value = enumValue;
                option.textContent = enumValue;
                select.appendChild(option);
            }
            select.value = activeTheme[field] || activeThemeId;
            select.addEventListener("change", () => {
                activeTheme[field] = select.value;
                saveTheme();
                applyTheme();
            });

            wrapper.append(label, select);
        } else {
            const label = document.createElement("label");
            label.htmlFor = `meta-${field}`;
            label.textContent = field;

            const input = document.createElement("input");
            input.type = "text";
            input.id = `meta-${field}`;
            input.value = activeTheme[field] ?? "";
            input.addEventListener("input", () => {
                activeTheme[field] = input.value.trim();
                saveTheme();
                if (field === "display_name") {
                    renderThemeSelect();
                    elements.themeSelect.value = activeThemeId;
                }
                applyTheme();
            });

            wrapper.append(label, input);
        }

        elements.metadataControls.appendChild(wrapper);
    }
}

function renderColorControls() {
    elements.colorControls.innerHTML = "";

    colorGroups.forEach((group, groupIndex) => {
        const details = document.createElement("details");
        details.className = "color-group";
        details.open = groupIndex < 4;

        const summary = document.createElement("summary");
        summary.textContent = group.title;

        const body = document.createElement("div");
        body.className = "color-group-body";

        for (const field of group.fields) {
            body.appendChild(createColorRow(field));
        }

        details.append(summary, body);
        elements.colorControls.appendChild(details);
    });
}

function createColorRow(field) {
    const row = document.createElement("div");
    row.className = "color-row";

    const label = document.createElement("label");
    label.htmlFor = `color-text-${field}`;
    label.textContent = field;

    const picker = document.createElement("input");
    picker.type = "color";
    picker.id = `color-picker-${field}`;
    picker.value = activeTheme[field];

    const text = document.createElement("input");
    text.type = "text";
    text.id = `color-text-${field}`;
    text.value = toCColor(activeTheme[field]);
    text.autocomplete = "off";
    text.spellcheck = false;

    picker.addEventListener("input", () => {
        updateColorField(field, picker.value, { picker, text });
    });

    text.addEventListener("input", () => {
        const normalized = normalizeColor(text.value);
        if (normalized == null) {
            text.classList.add("is-invalid");
            return;
        }

        updateColorField(field, normalized, { picker, text });
    });

    row.append(label, picker, text);
    return row;
}

function updateColorField(field, value, controls) {
    const normalized = normalizeColor(value);
    if (normalized == null) {
        return;
    }

    activeTheme[field] = normalized;
    controls.picker.value = normalized;
    controls.text.value = toCColor(normalized);
    controls.text.classList.remove("is-invalid");
    saveTheme();
    applyTheme();
}

function applyTheme() {
    const root = document.documentElement;

    for (const field of colorFields) {
        root.style.setProperty(cssVariableName(field), activeTheme[field]);
    }

    elements.activeThemeLabel.textContent = activeTheme.display_name || activeThemeId;
    elements.previewDisplayName.textContent = "Redmole";
    updateExport();
}

function updateExport() {
    elements.exportText.value = exportInitializer(activeTheme);
}

function exportInitializer(theme) {
    const maxFieldLength = allFields.reduce((max, field) => Math.max(max, field.length), 0);
    const lines = ["{"];

    for (const field of allFields) {
        const name = `.${field}`.padEnd(maxFieldLength + 2, " ");
        lines.push(`        ${name} = ${formatValueForC(field, theme[field])},`);
    }

    lines.push("    },");
    return lines.join("\n");
}

function formatValueForC(field, value) {
    if (colorFieldSet.has(field)) {
        return toCColor(value);
    }

    if (field === "display_name") {
        return `"${escapeCString(String(value ?? ""))}"`;
    }

    if (boolFieldSet.has(field)) {
        return value ? "true" : "false";
    }

    const raw = String(value ?? "").trim();
    return raw === "" ? "NULL" : raw;
}

function parseInitializer(text) {
    const fields = {};
    const lines = String(text).replace(/[{}]/g, "\n").split(/\r?\n/);

    for (const rawLine of lines) {
        let line = rawLine.replace(/\/\/.*$/, "").trim();
        if (line === "") {
            continue;
        }

        const match = line.match(/^\.(\w+)\s*=\s*(.+?)\s*,?$/);
        if (match == null) {
            continue;
        }

        const field = match[1];
        let value = match[2].trim();

        if (colorFieldSet.has(field)) {
            const color = normalizeColor(value);
            if (color != null) {
                fields[field] = color;
            }
            continue;
        }

        if (field === "display_name") {
            fields[field] = parseCString(value);
            continue;
        }

        if (boolFieldSet.has(field)) {
            fields[field] = value === "true";
            continue;
        }

        fields[field] = value;
    }

    return fields;
}

function parseCString(value) {
    const trimmed = value.trim();
    if (!trimmed.startsWith('"')) {
        return trimmed;
    }

    const body = trimmed.replace(/^"/, "").replace(/",?$/, "");
    return body
        .replace(/\\n/g, "\n")
        .replace(/\\"/g, '"')
        .replace(/\\\\/g, "\\");
}

function escapeCString(value) {
    return value
        .replace(/\\/g, "\\\\")
        .replace(/"/g, '\\"')
        .replace(/\n/g, "\\n");
}

function normalizeColor(value) {
    const text = String(value ?? "").trim();
    const match = text.match(/^(?:#|0x)?([0-9a-fA-F]{6})$/);

    if (match == null) {
        return null;
    }

    return `#${match[1].toUpperCase()}`;
}

function toCColor(value) {
    const normalized = normalizeColor(value) || "#000000";
    return `0x${normalized.slice(1).toUpperCase()}`;
}

function cssVariableName(field) {
    return `--${field.replaceAll("_", "-")}`;
}

function loadTheme(id) {
    const defaults = cloneTheme(defaultsById.get(id));
    const stored = readStoredTheme(id);

    if (stored == null) {
        return defaults;
    }

    return { ...defaults, ...stored };
}

function saveTheme() {
    try {
        localStorage.setItem(`${STORAGE_PREFIX}${activeThemeId}`, JSON.stringify(activeTheme));
    } catch (error) {
        setStatus(elements.importStatus, "Storage unavailable");
    }
}

function readStoredTheme(id) {
    try {
        const raw = localStorage.getItem(`${STORAGE_PREFIX}${id}`);
        return raw == null ? null : JSON.parse(raw);
    } catch (error) {
        return null;
    }
}

function removeStoredTheme(id) {
    try {
        localStorage.removeItem(`${STORAGE_PREFIX}${id}`);
    } catch (error) {
        /* localStorage can be unavailable for some file:// browser policies. */
    }
}

function cloneTheme(theme) {
    return JSON.parse(JSON.stringify(theme));
}

function setStatus(element, text) {
    element.textContent = text;
    window.clearTimeout(element._clearStatusTimer);
    element._clearStatusTimer = window.setTimeout(() => {
        element.textContent = "";
    }, 2200);
}
