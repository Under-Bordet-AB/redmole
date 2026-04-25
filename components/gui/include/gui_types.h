#ifndef GUI_TYPES_H
#define GUI_TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    GUI_PANEL_BME280 = 0,
    GUI_PANEL_ENERGY_PLAN,
    GUI_PANEL_FORECAST,
    GUI_PANEL_SETTINGS,
} gui_panel_id_t;

typedef enum {
    GUI_VIEW_THEME_LIGHT = 0,
    GUI_VIEW_THEME_DARK,
    GUI_VIEW_THEME_HELLO_KITTY,
    GUI_VIEW_THEME_TERMINAL,
    GUI_VIEW_THEME_HELLO_KITTY_NIGHT,
    GUI_VIEW_THEME_DEATH_NOTE,
    GUI_VIEW_THEME_SPONGEBOB,
} gui_view_theme_t;

#define GUI_ENERGY_PLAN_POINT_COUNT 24
#define GUI_WIFI_NETWORK_COUNT 4
#define GUI_WIFI_KNOWN_NETWORK_COUNT 3
#define GUI_WIFI_SSID_MAX_LEN 32
#define GUI_WIFI_PASSWORD_MAX_LEN 64
#define GUI_WIFI_STATUS_TEXT_MAX_LEN 96

typedef struct {
    int32_t temperature_deci_c;
    int32_t humidity_deci_pct;
    int32_t pressure_deci_hpa;
    bool is_fresh;
    uint32_t update_count;
} gui_sensor_state_t;

typedef struct {
    uint16_t buy_electricity[GUI_ENERGY_PLAN_POINT_COUNT];
    uint16_t use_solar_directly[GUI_ENERGY_PLAN_POINT_COUNT];
    uint16_t charge_battery[GUI_ENERGY_PLAN_POINT_COUNT];
    uint16_t sell_excess[GUI_ENERGY_PLAN_POINT_COUNT];
} gui_energy_plan_t;

typedef enum {
    GUI_WIFI_STATE_IDLE = 0,
    GUI_WIFI_STATE_SCANNED,
    GUI_WIFI_STATE_CONNECTING,
    GUI_WIFI_STATE_CONNECTED,
    GUI_WIFI_STATE_FAILED,
} gui_wifi_state_t;

typedef struct {
    char ssid[GUI_WIFI_SSID_MAX_LEN];
    uint8_t signal_strength_pct;
    bool secured;
} gui_wifi_network_t;

typedef struct {
    gui_wifi_network_t networks[GUI_WIFI_NETWORK_COUNT];
    uint8_t network_count;
    gui_wifi_network_t known_networks[GUI_WIFI_KNOWN_NETWORK_COUNT];
    uint8_t known_network_count;
    int8_t selected_network_index;
    int8_t selected_known_network_index;
    char selected_ssid[GUI_WIFI_SSID_MAX_LEN];
    char password[GUI_WIFI_PASSWORD_MAX_LEN + 1];
    char status_text[GUI_WIFI_STATUS_TEXT_MAX_LEN];
    gui_wifi_state_t state;
    bool connect_requested;
    bool can_disconnect;
} gui_wifi_settings_t;

typedef struct {
    gui_view_theme_t theme;
    bool show_background_image;
    bool night_variant_enabled;
} gui_appearance_settings_t;

typedef struct {
    gui_panel_id_t active_panel;
    gui_sensor_state_t sensor;
    gui_energy_plan_t energy_plan;
    gui_wifi_settings_t wifi;
    gui_appearance_settings_t appearance;
} gui_view_model_t;

#endif
