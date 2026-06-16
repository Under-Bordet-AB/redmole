#ifndef GUI_TYPES_H
#define GUI_TYPES_H

/**
 * @file gui_types.h
 * @brief Shared GUI data types and configuration limits.
 *
 * These types form the copy-by-value model passed between the application,
 * GUI state container, and LVGL view layer.
 */

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Identifiers for the top-level panels shown in the GUI.
 */
typedef enum {
    GUI_PANEL_BME280 = 0,    /*!< Local BME280 environmental measurements panel. */
    GUI_PANEL_ENERGY_PLAN,   /*!< Hourly energy plan chart panel. */
    GUI_PANEL_FORECAST,      /*!< Weather forecast panel. */
    GUI_PANEL_SETTINGS,      /*!< User settings and connectivity panel. */
} gui_panel_id_t;

/**
 * @brief View mode for the energy panel.
 */
typedef enum {
    GUI_ENERGY_PANEL_MODE_LEOP = 0,      /*!< Show the LEOP action plan chart. */
    GUI_ENERGY_PANEL_MODE_SPOT_PRICE,    /*!< Show the spot-price chart. */
} gui_energy_panel_mode_t;

/**
 * @brief Swedish electricity price areas supported by the spot-price feed.
 */
typedef enum {
    GUI_SPOT_PRICE_AREA_SE1 = 0, /*!< SE1 / Luleå / Norra Sverige. */
    GUI_SPOT_PRICE_AREA_SE2,     /*!< SE2 / Sundsvall / Norra Mellansverige. */
    GUI_SPOT_PRICE_AREA_SE3,     /*!< SE3 / Stockholm / Södra Mellansverige. */
    GUI_SPOT_PRICE_AREA_SE4,     /*!< SE4 / Malmö / Södra Sverige. */
} gui_spot_price_area_t;

/**
 * @brief Available visual themes for the GUI.
 */
typedef enum {
    GUI_VIEW_THEME_LIGHT = 0,        /*!< Default light theme. */
    GUI_VIEW_THEME_DARK,             /*!< Default dark theme. */
    GUI_VIEW_THEME_HELLO_KITTY,      /*!< Hello Kitty themed appearance. */
    GUI_VIEW_THEME_TERMINAL,         /*!< Terminal themed appearance. */
    GUI_VIEW_THEME_HELLO_KITTY_NIGHT, /*!< Internal Hello Kitty night variant. */
    GUI_VIEW_THEME_DEATH_NOTE,       /*!< Death Note themed appearance. */
    GUI_VIEW_THEME_SPONGEBOB,        /*!< SpongeBob themed appearance. */
    GUI_VIEW_THEME_BONZI_BUDDY,      /*!< Bonzi Buddy themed appearance. */
} gui_view_theme_t;

/**
 * @brief Weather icon identifiers rendered by the forecast panel.
 */
typedef enum {
    GUI_WEATHER_ICON_CLEAR = 0,       /*!< Clear sky or sunny conditions. */
    GUI_WEATHER_ICON_PARTLY_CLOUDY,   /*!< Partly cloudy conditions. */
    GUI_WEATHER_ICON_CLOUDY,          /*!< Cloudy or unknown fallback conditions. */
    GUI_WEATHER_ICON_FOG,             /*!< Fog or low-visibility conditions. */
    GUI_WEATHER_ICON_DRIZZLE,         /*!< Drizzle or freezing drizzle conditions. */
    GUI_WEATHER_ICON_RAIN,            /*!< Rain or rain shower conditions. */
    GUI_WEATHER_ICON_SNOW,            /*!< Snow or snow shower conditions. */
    GUI_WEATHER_ICON_THUNDERSTORM,    /*!< Thunderstorm conditions. */
    GUI_WEATHER_ICON_COUNT,           /*!< Number of weather icon identifiers. */
} gui_weather_icon_t;

/** Hourly points stored in an energy plan day profile. */
#define GUI_ENERGY_PLAN_POINT_COUNT 24
/** Hourly points stored in a forward-looking spot-price profile. */
#define GUI_SPOT_PRICE_POINT_COUNT 24
/** Hour labels shown under the energy plan chart. */
#define GUI_ENERGY_PLAN_TIME_LABEL_COUNT 5
/** Number of daily forecast entries rendered in the forecast panel. */
#define GUI_FORECAST_DAY_COUNT 5
/** Number of detail lines shown in the forecast details card. */
#define GUI_FORECAST_DETAIL_COUNT 4
/** Maximum number of scanned Wi-Fi networks shown in the selection dialog. */
#define GUI_WIFI_NETWORK_COUNT 4
/** Maximum number of saved Wi-Fi networks surfaced as known networks. */
#define GUI_WIFI_KNOWN_NETWORK_COUNT 3
/** Buffer size for 32-byte SSIDs plus the trailing null byte. */
#define GUI_WIFI_SSID_MAX_LEN 33
/** Maximum Wi-Fi password length accepted from the password dialog. */
#define GUI_WIFI_PASSWORD_MAX_LEN 64
/** Maximum length of the user-visible Wi-Fi status message buffer. */
#define GUI_WIFI_STATUS_TEXT_MAX_LEN 96
/** Maximum length of a user-entered location coordinate string. */
#define GUI_LOCATION_TEXT_MAX_LEN 24
/** Maximum length of the forecast title label. */
#define GUI_FORECAST_TITLE_MAX_LEN 24
/** Maximum length of a forecast condition string. */
#define GUI_FORECAST_CONDITION_MAX_LEN 32
/** Maximum length of a forecast temperature string. */
#define GUI_FORECAST_TEMP_TEXT_MAX_LEN 20
/** Maximum length of the forecast feels-like temperature string. */
#define GUI_FORECAST_FEELS_LIKE_TEXT_MAX_LEN 24
/** Maximum length of the forecast range string. */
#define GUI_FORECAST_RANGE_TEXT_MAX_LEN 32
/** Maximum length of the forecast summary string. */
#define GUI_FORECAST_SUMMARY_TEXT_MAX_LEN 96
/** Maximum length of a forecast detail line. */
#define GUI_FORECAST_DETAIL_TEXT_MAX_LEN 32
/** Maximum length of a compact forecast day label. */
#define GUI_FORECAST_DAY_LABEL_MAX_LEN 12
/** Maximum length of a daily forecast date string. */
#define GUI_FORECAST_DAY_DATE_TEXT_MAX_LEN 16
/** Maximum length of a daily forecast range string. */
#define GUI_FORECAST_DAY_RANGE_TEXT_MAX_LEN 20
/** Maximum length of a last-updated timestamp label. */
#define GUI_LAST_UPDATED_TEXT_MAX_LEN 32
/** Maximum length of the current spot-price value text. */
#define GUI_SPOT_PRICE_VALUE_TEXT_MAX_LEN 24
/** Maximum length of the spot-price summary text. */
#define GUI_SPOT_PRICE_SUMMARY_TEXT_MAX_LEN 64

/**
 * @brief Latest sensor readings displayed by the GUI.
 */
typedef struct {
    int32_t temperature_deci_c; /*!< Temperature in deci-degrees Celsius. */
    int32_t humidity_deci_pct; /*!< Relative humidity in deci-percent. */
    int32_t pressure_deci_hpa; /*!< Pressure in deci-hectopascals. */
    bool is_fresh;             /*!< True when the readings reflect a recent sensor update. */
    uint32_t update_count;     /*!< Monotonic counter of applied sensor updates. */
    char last_updated[GUI_LAST_UPDATED_TEXT_MAX_LEN]; /*!< Null-terminated last successful update timestamp. */
} gui_sensor_state_t;

/**
 * @brief Hour-by-hour energy plan values rendered by the energy chart.
 */
typedef struct {
    uint16_t buy_electricity[GUI_ENERGY_PLAN_POINT_COUNT]; /*!< Grid purchase values per hour, scaled by 1000. */
    uint16_t use_solar_directly[GUI_ENERGY_PLAN_POINT_COUNT]; /*!< Direct solar consumption per hour, scaled by 1000. */
    uint16_t charge_battery[GUI_ENERGY_PLAN_POINT_COUNT]; /*!< Battery charging values per hour, scaled by 1000. */
    uint16_t sell_excess[GUI_ENERGY_PLAN_POINT_COUNT]; /*!< Excess energy sold back per hour, scaled by 1000. */
    uint8_t start_hour; /*!< Hour represented by the first chart point, 0-23. */
    char last_updated[GUI_LAST_UPDATED_TEXT_MAX_LEN]; /*!< Null-terminated last successful update timestamp. */
} gui_energy_plan_t;

/**
 * @brief Spot-price values rendered by the energy panel.
 */
typedef struct {
    int16_t price_milli_kr[GUI_SPOT_PRICE_POINT_COUNT]; /*!< Hourly kr/kWh values, scaled by 1000. */
    bool valid_points[GUI_SPOT_PRICE_POINT_COUNT]; /*!< True when the paired price point is known. */
    int16_t current_price_milli_kr; /*!< Current quarter-hour kr/kWh value, scaled by 1000. */
    bool has_current_price; /*!< True when current_price_milli_kr contains a fetched value. */
    uint8_t start_hour; /*!< Hour represented by the first chart point, 0-23. */
    gui_spot_price_area_t area; /*!< Selected Swedish price area used for the feed. */
    char current_price_text[GUI_SPOT_PRICE_VALUE_TEXT_MAX_LEN]; /*!< Formatted current price value. */
    char summary[GUI_SPOT_PRICE_SUMMARY_TEXT_MAX_LEN]; /*!< Compact min/max or availability summary. */
    char last_updated[GUI_LAST_UPDATED_TEXT_MAX_LEN]; /*!< Null-terminated last successful update timestamp. */
} gui_spot_price_state_t;

/**
 * @brief Forecast details shown in the side card.
 */
typedef struct {
    char rain_chance[GUI_FORECAST_DETAIL_TEXT_MAX_LEN]; /*!< Daily rain chance summary. */
    char wind[GUI_FORECAST_DETAIL_TEXT_MAX_LEN]; /*!< Wind summary. */
    char humidity[GUI_FORECAST_DETAIL_TEXT_MAX_LEN]; /*!< Current humidity summary. */
    char uv_index[GUI_FORECAST_DETAIL_TEXT_MAX_LEN]; /*!< UV summary. */
} gui_forecast_details_t;

/**
 * @brief Daily forecast entry shown in the forecast footer row.
 */
typedef struct {
    char label[GUI_FORECAST_DAY_LABEL_MAX_LEN]; /*!< Day label such as Mon or Tue. */
    char date_text[GUI_FORECAST_DAY_DATE_TEXT_MAX_LEN]; /*!< Calendar date such as May 26. */
    gui_weather_icon_t icon; /*!< Icon matching the daily weather condition. */
    char range_text[GUI_FORECAST_DAY_RANGE_TEXT_MAX_LEN]; /*!< Daily high/low temperature text. */
} gui_forecast_day_t;

/**
 * @brief Forecast state rendered by the forecast panel.
 */
typedef struct {
    bool has_data; /*!< True when the values came from a parsed forecast response. */
    char title[GUI_FORECAST_TITLE_MAX_LEN]; /*!< Primary forecast heading. */
    char condition[GUI_FORECAST_CONDITION_MAX_LEN]; /*!< Current condition text. */
    gui_weather_icon_t current_icon; /*!< Icon matching the current weather condition. */
    char current_temperature[GUI_FORECAST_TEMP_TEXT_MAX_LEN]; /*!< Current temperature text. */
    char feels_like_temperature[GUI_FORECAST_FEELS_LIKE_TEXT_MAX_LEN]; /*!< Apparent temperature text. */
    char range_text[GUI_FORECAST_RANGE_TEXT_MAX_LEN]; /*!< High/low summary text. */
    char summary[GUI_FORECAST_SUMMARY_TEXT_MAX_LEN]; /*!< One-line forecast summary. */
    gui_forecast_details_t details; /*!< Detail lines for the side card. */
    gui_forecast_day_t days[GUI_FORECAST_DAY_COUNT]; /*!< Daily forecast entries shown below the summary cards. */
    char last_updated[GUI_LAST_UPDATED_TEXT_MAX_LEN]; /*!< Last successful update timestamp. */
} gui_forecast_state_t;

/**
 * @brief Wi-Fi connection states surfaced by the GUI.
 */
typedef enum {
    GUI_WIFI_STATE_IDLE = 0,  /*!< No active scan or connection attempt is visible. */
    GUI_WIFI_STATE_SCANNED,   /*!< Scan results are available for user selection. */
    GUI_WIFI_STATE_CONNECTING, /*!< A connection attempt is in progress. */
    GUI_WIFI_STATE_CONNECTED, /*!< Wi-Fi is connected. */
    GUI_WIFI_STATE_FAILED,    /*!< Last visible Wi-Fi operation failed. */
} gui_wifi_state_t;

/**
 * @brief Bluetooth availability states shown in the sidebar and settings.
 */
typedef enum {
    GUI_BLUETOOTH_STATE_IDLE = 0,   /*!< Bluetooth is idle or not configured. */
    GUI_BLUETOOTH_STATE_CONNECTING, /*!< Bluetooth connection is in progress. */
    GUI_BLUETOOTH_STATE_CONNECTED,  /*!< Bluetooth is connected. */
    GUI_BLUETOOTH_STATE_UNAVAILABLE, /*!< Bluetooth is unavailable on this device. */
} gui_bluetooth_state_t;

/**
 * @brief SD card availability states shown in the sidebar.
 */
typedef enum {
    GUI_SD_CARD_STATE_IDLE = 0,    /*!< SD card state has not been reported yet. */
    GUI_SD_CARD_STATE_CONNECTED,   /*!< SD card storage is initialized. */
    GUI_SD_CARD_STATE_UNAVAILABLE, /*!< SD card storage is not initialized. */
} gui_sd_card_state_t;

/**
 * @brief Summary of a Wi-Fi network shown in the GUI.
 */
typedef struct {
    char ssid[GUI_WIFI_SSID_MAX_LEN]; /*!< Null-terminated SSID string in a fixed GUI buffer. */
    uint8_t signal_strength_pct;      /*!< Signal quality expressed as a percentage. */
    bool secured;                     /*!< True when the network requires authentication. */
} gui_wifi_network_t;

/**
 * @brief Complete Wi-Fi dialog and connection state consumed by the GUI.
 */
typedef struct {
    gui_wifi_network_t networks[GUI_WIFI_NETWORK_COUNT]; /*!< Scanned networks available for selection. */
    uint8_t network_count; /*!< Number of valid entries in networks, 0-GUI_WIFI_NETWORK_COUNT. */
    gui_wifi_network_t known_networks[GUI_WIFI_KNOWN_NETWORK_COUNT]; /*!< Saved networks offered as quick-connect targets. */
    uint8_t known_network_count; /*!< Number of valid entries in known_networks, 0-GUI_WIFI_KNOWN_NETWORK_COUNT. */
    int8_t selected_network_index; /*!< Selected scanned network index, or -1 when none is selected. */
    int8_t selected_known_network_index; /*!< Selected known network index, or -1 when not applicable. */
    char selected_ssid[GUI_WIFI_SSID_MAX_LEN]; /*!< SSID currently targeted by the password dialog. */
    char password[GUI_WIFI_PASSWORD_MAX_LEN + 1]; /*!< Editable password buffer including the trailing null byte. */
    char status_text[GUI_WIFI_STATUS_TEXT_MAX_LEN]; /*!< Status text shown under the Wi-Fi section header. */
    gui_wifi_state_t state; /*!< Current Wi-Fi connection workflow state. */
    bool connect_requested; /*!< True after the user has requested a connection attempt. */
    bool can_disconnect; /*!< True when the active connection can be disconnected from the GUI. */
} gui_wifi_settings_t;

/**
 * @brief User-selectable appearance settings applied to the GUI.
 */
typedef struct {
    gui_view_theme_t theme;       /*!< Base theme selection. */
    bool show_background_image;   /*!< True when the theme background image should be shown. */
    bool night_variant_enabled;   /*!< True when a theme-specific night variant should be used. */
} gui_appearance_settings_t;

/**
 * @brief User-editable location settings shown in the System settings page.
 */
typedef struct {
    char latitude[GUI_LOCATION_TEXT_MAX_LEN + 1];  /*!< Latitude in decimal degrees. */
    char longitude[GUI_LOCATION_TEXT_MAX_LEN + 1]; /*!< Longitude in decimal degrees. */
} gui_location_settings_t;

/**
 * @brief Aggregated model rendered by the GUI view layer.
 */
typedef struct {
    gui_panel_id_t active_panel;                /*!< Panel that should be visible in the content area. */
    gui_energy_panel_mode_t energy_panel_mode;  /*!< Subview shown inside the energy panel. */
    gui_sensor_state_t sensor;                  /*!< Latest environmental sensor state. */
    gui_energy_plan_t energy_plan;              /*!< Energy plan series rendered on the chart panel. */
    gui_spot_price_state_t spot_price;          /*!< Spot-price series rendered on the energy panel. */
    gui_forecast_state_t forecast;              /*!< Forecast data rendered on the forecast panel. */
    gui_wifi_settings_t wifi;                   /*!< Wi-Fi dialog and connection settings. */
    gui_wifi_state_t wifi_state;                /*!< Sidebar Wi-Fi status indicator state. */
    gui_bluetooth_state_t bluetooth_state;      /*!< Sidebar Bluetooth status indicator state. */
    gui_sd_card_state_t sd_card_state;          /*!< Sidebar SD card status indicator state. */
    gui_appearance_settings_t appearance;       /*!< Theme and background presentation settings. */
    gui_location_settings_t location;           /*!< Editable location settings shown in System. */
} gui_view_model_t;

#endif
