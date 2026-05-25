/**
 * @file app_gui_bindings.c
 * @brief Synchronize application services, persisted settings, and GUI events.
 *
 * This module loads persisted GUI startup preferences, mirrors runtime
 * application state into the GUI model, persists appearance changes, and
 * forwards GUI-originated Wi-Fi actions to the network control layer.
 */

#include "app_gui_bindings.h"

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cJSON.h"
#include "esp_log.h"
#include "http_client.h"
#include "nac.h"
#include "rm_nvs.h"
#include "sensor_data.h"
#include "sdkconfig.h"

static const char *TAG = "APP_GUI_BINDINGS";

// Defines for NVS keys
#define GUI_NVS_KEY_THEME       "gui_theme"
#define GUI_NVS_KEY_BG          "gui_bg"
#define GUI_NVS_KEY_NIGHT       "gui_night"
#define GUI_NVS_KEY_BRIGHT      "gui_bright"
#define GUI_NVS_KEY_LAT         "gui_lat"
#define GUI_NVS_KEY_LON         "gui_lon"
#define GUI_NVS_KEY_WIFI_SSID   "wifi_ssid"
#define FORECAST_REFRESH_DELAY_MS 60000U
#define LEOP_REFRESH_URL CONFIG_REDMOLE_LEOP_REFRESH_URL
#define LEOP_RAW_POINT_COUNT 96
#define LEOP_POINTS_PER_HOUR 4
#define LEOP_REFRESH_DELAY_MS 60000U
#define GUI_VALID_TIME_THRESHOLD 1700000000
/*
 * Scale LEOP hourly totals into integer chart values using milli-units so the
 * 15-minute inputs retain visible precision after aggregation.
 */
#define LEOP_VALUE_SCALE 1000.0

typedef struct {
    gui_ctx_t *gui;

    bool wifi_connect_requested;
    bool wifi_scan_requested;
    bool wifi_disconnect_requested;

    bool has_last_wifi_state;
    gui_wifi_state_t last_wifi_state;

    bool has_last_sd_card_state;
    gui_sd_card_state_t last_sd_card_state;

    bool has_last_appearance;
    gui_appearance_settings_t last_appearance;
    bool has_last_location;
    gui_location_settings_t last_location;

    bool has_last_brightness;
    int32_t last_brightness;

    bool boot_autoconnect_queued;
    bool startup_clock_refresh_pending;
    char requested_ssid[GUI_WIFI_SSID_MAX_LEN];
} app_gui_bindings_ctx_t;

static app_gui_bindings_ctx_t s_bindings;

// Forward declarations
//////////////////////////

// Helpers
static uint8_t signal_strength_pct(int8_t rssi);
static gui_wifi_state_t map_wifi_state(nac_wifi_status_t status);
static void set_wifi_status(gui_ctx_t *gui, const char *status_text, gui_wifi_state_t state);
static void copy_scan_results(gui_wifi_settings_t *wifi);
static gui_sd_card_state_t get_sd_card_state(void);
static int32_t clamp_saved_brightness(int32_t value);
static bool parse_coordinate_in_range(const char *text, double min_value, double max_value);
static const char *forecast_weather_code_to_condition(int weather_code);
static int forecast_weather_code_to_sky_rank(int weather_code);
static bool forecast_parse_hour(const char *time_text, char *date, size_t date_len, int *hour);
static void forecast_format_later_today_summary(const cJSON *hourly_time,
                                                const cJSON *hourly_temperature,
                                                const cJSON *hourly_weather_code,
                                                const char *today_date,
                                                const char *current_time_text,
                                                double current_temperature,
                                                int current_weather_code,
                                                char *summary,
                                                size_t summary_len);
static const char *forecast_wind_direction_to_compass(double direction_degrees);
static void forecast_format_day_label(const char *date_text, char *label, size_t label_len);
static bool forecast_load_location(double *latitude, double *longitude);
static bool forecast_parse_response(const char *response, gui_forecast_state_t *forecast);
static bool leop_parse_response(const char *response, gui_energy_plan_t *energy_plan);
static bool leop_parse_series(const cJSON *series_array, uint16_t *values, const char *series_name);
static uint16_t leop_scale_hourly_total(double hourly_total);

// UI sync
static void sync_sensor(gui_ctx_t *gui);
static bool sync_wifi_state(gui_ctx_t *gui);
static void sync_wifi(gui_ctx_t *gui);
static void sync_startup_sidebar_clock(gui_ctx_t *gui, nac_wifi_status_t wifi_status);
static bool sync_sd_card_state(gui_ctx_t *gui);

// UI caching
bool app_gui_bindings_load_saved_appearance(gui_init_config_t *config);
static void cache_current_appearance(gui_ctx_t *gui);
static bool save_appearance_if_changed(gui_ctx_t *gui);
static void cache_current_location(gui_ctx_t *gui);
static bool load_saved_location(gui_ctx_t *gui);
static bool save_location_if_changed(gui_ctx_t *gui);

// Wi-Fi metadata
static bool load_saved_wifi_metadata(gui_ctx_t *gui);
static bool refresh_saved_wifi_metadata(gui_ctx_t *gui);
static bool queue_saved_wifi_autoconnect(gui_ctx_t *gui);

// UI events
static void on_panel_changed(gui_ctx_t *gui, gui_panel_id_t panel, void *user_data);
static bool on_wifi_scan_requested(gui_ctx_t *gui, void *user_data);
static void on_wifi_network_selected(gui_ctx_t *gui, const gui_wifi_network_t *network, void *user_data);
static bool on_wifi_known_network_requested(gui_ctx_t *gui, const gui_wifi_network_t *network, void *user_data);
static bool on_wifi_connect_requested(gui_ctx_t *gui, const char *ssid, const char *password, void *user_data);
static bool on_wifi_disconnect_requested(gui_ctx_t *gui, void *user_data);

// Basics
esp_err_t app_gui_bindings_init(gui_ctx_t *gui);
void app_gui_bindings_sync(gui_ctx_t *gui);

// Function definitions
//////////////////////////

static uint8_t signal_strength_pct(int8_t rssi)
{
    if (rssi <= -100) {
        return 0;
    }

    if (rssi >= -50) {
        return 100;
    }
    
    return (uint8_t)((rssi + 100) * 2);
}

static gui_wifi_state_t map_wifi_state(nac_wifi_status_t status)
{
    switch (status) {
        case NAC_WIFI_CONNECTED:
            return GUI_WIFI_STATE_CONNECTED;
        case NAC_WIFI_ERROR:
            return GUI_WIFI_STATE_FAILED;
        case NAC_WIFI_SCANNING:
            return GUI_WIFI_STATE_SCANNED;
        case NAC_WIFI_CONNECTING:
            return GUI_WIFI_STATE_CONNECTING;
        case NAC_WIFI_DISCONNECTED:
        default:
            return GUI_WIFI_STATE_IDLE;
    }
}

static void set_wifi_status(gui_ctx_t *gui, const char *status_text, gui_wifi_state_t state)
{
    gui_wifi_settings_t wifi = { 0 };

    if ((gui == NULL) || (status_text == NULL) || !gui_get_wifi_settings(gui, &wifi)) {
        return;
    }

    wifi.state = state;
    snprintf(wifi.status_text, sizeof(wifi.status_text), "%s", status_text);
    gui_set_wifi_settings(gui, &wifi);
}

static void copy_scan_results(gui_wifi_settings_t *wifi)
{
    const wifi_ap_record_t *records;
    uint16_t record_count;
    uint8_t count;

    if (wifi == NULL) {
        return;
    }

    record_count = 0;
    records = nac_get_scan_results(&record_count);

    memset(wifi->networks, 0, sizeof(wifi->networks));
    wifi->network_count = 0;
    if (records == NULL) {
        return;
    }

    count = (record_count > GUI_WIFI_NETWORK_COUNT) ? GUI_WIFI_NETWORK_COUNT : (uint8_t)record_count;
    for (uint8_t index = 0; index < count; index++) {
        size_t ssid_len = strnlen((const char *)records[index].ssid, sizeof(wifi->networks[index].ssid) - 1U);

        memcpy(wifi->networks[index].ssid, records[index].ssid, ssid_len);
        wifi->networks[index].ssid[ssid_len] = '\0';
        wifi->networks[index].signal_strength_pct = signal_strength_pct(records[index].rssi);
        wifi->networks[index].secured = records[index].authmode != WIFI_AUTH_OPEN;
    }
    wifi->network_count = count;
}

static void sync_sensor(gui_ctx_t *gui)
{
    sensor_data_sample sample = { 0 };
    gui_sensor_state_t sensor = { 0 };

    if (gui == NULL) {
        return;
    }

    if (sensor_data_get_latest_local(&sample) && sample.valid) {
        sensor.temperature_deci_c = sample.temperature_deci_c;
        sensor.humidity_deci_pct = sample.humidity_deci_pct;
        sensor.pressure_deci_hpa = sample.pressure_deci_hpa;
        sensor.is_fresh = sensor_data_is_local_fresh(3000U);
        sensor.update_count = sensor_data_get_local_update_count();
    }

    gui_set_sensor_state(gui, &sensor);
}

static gui_sd_card_state_t get_sd_card_state(void)
{
    return GUI_SD_CARD_STATE_IDLE;
}

static bool sync_wifi_state(gui_ctx_t *gui)
{
    gui_wifi_state_t wifi_state;

    if (gui == NULL) {
        return false;
    }

    wifi_state = map_wifi_state(nac_get_wifi_status());
    if (s_bindings.has_last_wifi_state && (s_bindings.last_wifi_state == wifi_state)) {
        return false;
    }

    gui_set_wifi_state(gui, wifi_state);
    s_bindings.last_wifi_state = wifi_state;
    s_bindings.has_last_wifi_state = true;
    return true;
}

static bool sync_sd_card_state(gui_ctx_t *gui)
{
    gui_sd_card_state_t sd_card_state;

    if (gui == NULL) {
        return false;
    }

    sd_card_state = get_sd_card_state();
    if (s_bindings.has_last_sd_card_state &&
        (s_bindings.last_sd_card_state == sd_card_state)) {
        return false;
    }

    gui_set_sd_card_state(gui, sd_card_state);
    s_bindings.last_sd_card_state = sd_card_state;
    s_bindings.has_last_sd_card_state = true;
    return true;
}

static void sync_wifi(gui_ctx_t *gui)
{
    gui_wifi_settings_t wifi = { 0 };
    nac_wifi_status_t nac_status;

    if ((gui == NULL) || !gui_get_wifi_settings(gui, &wifi)) {
        return;
    }

    nac_status = nac_get_wifi_status();
    wifi.state = map_wifi_state(nac_status);
    wifi.connect_requested = s_bindings.wifi_connect_requested;
    wifi.can_disconnect = nac_status == NAC_WIFI_CONNECTED;

    if ((wifi.selected_ssid[0] == '\0') && (s_bindings.requested_ssid[0] != '\0')) {
        snprintf(wifi.selected_ssid, sizeof(wifi.selected_ssid), "%s",
                 s_bindings.requested_ssid);
    }

    if (nac_status == NAC_WIFI_SCANNING) {
        snprintf(wifi.status_text, sizeof(wifi.status_text), "%s", "Scanning for Wi-Fi networks...");
    } else if (s_bindings.wifi_scan_requested && nac_scan_is_complete()) {
        s_bindings.wifi_scan_requested = false;
        copy_scan_results(&wifi);
        if (wifi.network_count > 0U) {
            wifi.state = GUI_WIFI_STATE_SCANNED;
            snprintf(wifi.status_text, sizeof(wifi.status_text), "%s", "Scan complete. Select a network.");
        } else {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "%s", "Scan complete. No Wi-Fi networks found.");
        }
    } else if (nac_status == NAC_WIFI_CONNECTING) {
        if (wifi.selected_ssid[0] != '\0') {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "Connecting to %s...", wifi.selected_ssid);
        } else {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "%s", "Connecting to Wi-Fi...");
        }
    } else if (nac_status == NAC_WIFI_CONNECTED) {
        s_bindings.wifi_connect_requested = false;
        s_bindings.wifi_disconnect_requested = false;
        wifi.connect_requested = false;
        wifi.can_disconnect = true;
        if (wifi.selected_ssid[0] != '\0') {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "Connected to %s.", wifi.selected_ssid);
        } else {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "%s", "Wi-Fi connected.");
        }
        gui_set_wifi_settings(gui, &wifi);
        (void)refresh_saved_wifi_metadata(gui);
        gui_hide_wifi_dialogs(gui);
        return;
    } else if (nac_status == NAC_WIFI_ERROR) {
        s_bindings.wifi_connect_requested = false;
        s_bindings.wifi_disconnect_requested = false;
        wifi.connect_requested = false;
        wifi.can_disconnect = false;
        if (wifi.selected_ssid[0] != '\0') {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "Failed to connect to %s.", wifi.selected_ssid);
        } else {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "%s", "Wi-Fi connection failed.");
        }
    } else if (s_bindings.wifi_connect_requested &&
               nac_status == NAC_WIFI_DISCONNECTED) {
        s_bindings.wifi_connect_requested = false;
        wifi.connect_requested = false;
        wifi.can_disconnect = false;
        wifi.state = GUI_WIFI_STATE_IDLE;
        snprintf(wifi.status_text, sizeof(wifi.status_text), "%s", "Wi-Fi disconnected.");
    } else if (s_bindings.wifi_connect_requested) {
        wifi.state = GUI_WIFI_STATE_CONNECTING;
        wifi.connect_requested = true;
        if (wifi.selected_ssid[0] != '\0') {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "Connecting to %s...", wifi.selected_ssid);
        } else {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "%s", "Connecting to Wi-Fi...");
        }
    } else if (s_bindings.wifi_disconnect_requested) {
        s_bindings.wifi_disconnect_requested = false;
        wifi.connect_requested = false;
        wifi.can_disconnect = false;
        wifi.state = GUI_WIFI_STATE_IDLE;
        snprintf(wifi.status_text, sizeof(wifi.status_text), "%s", "Disconnected.");
    } else {
        wifi.connect_requested = false;
        wifi.can_disconnect = false;
        if (wifi.status_text[0] == '\0') {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "%s", "Press Scan to search for Wi-Fi networks.");
        }
    }

    gui_set_wifi_settings(gui, &wifi);
}

static void sync_startup_sidebar_clock(gui_ctx_t *gui, nac_wifi_status_t wifi_status)
{
    time_t now;

    if ((gui == NULL) || !s_bindings.startup_clock_refresh_pending ||
        (wifi_status != NAC_WIFI_CONNECTED)) {
        return;
    }

    now = time(NULL);
    if (now < GUI_VALID_TIME_THRESHOLD) {
        return;
    }

    gui_refresh_sidebar_clock(gui);
    s_bindings.startup_clock_refresh_pending = false;
}

// Loading & saving

static int32_t clamp_saved_brightness(int32_t value) {
    if (value < 5) {
        return 5;
    }

    if (value > 100) {
        return 100;
    }

    return value;
}

static bool parse_coordinate_in_range(const char *text, double min_value, double max_value)
{
    char *end = NULL;
    double value;

    if (text == NULL) {
        return false;
    }

    if (text[0] == '\0') {
        return true;
    }

    value = strtod(text, &end);
    if ((end == text) || (end == NULL) || (*end != '\0')) {
        return false;
    }

    return (value >= min_value) && (value <= max_value);
}

static const char *forecast_weather_code_to_condition(int weather_code)
{
    switch (weather_code) {
        case 0:
            return "Clear sky";
        case 1:
            return "Mainly clear";
        case 2:
            return "Partly cloudy";
        case 3:
            return "Overcast";
        case 45:
        case 48:
            return "Fog";
        case 51:
        case 53:
        case 55:
            return "Drizzle";
        case 56:
        case 57:
            return "Freezing drizzle";
        case 61:
        case 63:
        case 65:
            return "Rain";
        case 66:
        case 67:
            return "Freezing rain";
        case 71:
        case 73:
        case 75:
        case 77:
            return "Snow";
        case 80:
        case 81:
        case 82:
            return "Rain showers";
        case 85:
        case 86:
            return "Snow showers";
        case 95:
            return "Thunderstorm";
        case 96:
        case 99:
            return "Storm with hail";
        default:
            return "Unknown";
    }
}

static int forecast_weather_code_to_sky_rank(int weather_code)
{
    switch (weather_code) {
        case 0:
        case 1:
            return 0;
        case 2:
            return 1;
        case 3:
        case 45:
        case 48:
            return 2;
        case 51:
        case 53:
        case 55:
        case 56:
        case 57:
        case 61:
        case 63:
        case 65:
        case 66:
        case 67:
        case 71:
        case 73:
        case 75:
        case 77:
        case 80:
        case 81:
        case 82:
        case 85:
        case 86:
        case 95:
        case 96:
        case 99:
            return 3;
        default:
            return 1;
    }
}

static bool forecast_parse_hour(const char *time_text, char *date, size_t date_len, int *hour)
{
    int parsed_hour;

    if ((time_text == NULL) || (date == NULL) || (date_len < 11U) || (hour == NULL)) {
        return false;
    }

    if ((strlen(time_text) < 13U) || (time_text[10] != 'T') ||
        (sscanf(&time_text[11], "%2d", &parsed_hour) != 1) ||
        (parsed_hour < 0) || (parsed_hour > 23)) {
        return false;
    }

    memcpy(date, time_text, 10U);
    date[10] = '\0';
    *hour = parsed_hour;
    return true;
}

static void forecast_format_later_today_summary(const cJSON *hourly_time,
                                                const cJSON *hourly_temperature,
                                                const cJSON *hourly_weather_code,
                                                const char *today_date,
                                                const char *current_time_text,
                                                double current_temperature,
                                                int current_weather_code,
                                                char *summary,
                                                size_t summary_len)
{
    enum { FORECAST_TARGET_EVENING_HOUR = 18 };
    char current_date[11];
    char point_date[11];
    int current_hour;
    int point_hour;
    int selected_index = -1;
    int selected_distance = INT_MAX;
    int hourly_count;
    int index;
    cJSON *selected_temperature;
    cJSON *selected_weather_code;
    double later_temperature;
    int temperature_delta;
    int current_sky_rank;
    int later_sky_rank;

    if ((summary == NULL) || (summary_len == 0U)) {
        return;
    }

    snprintf(summary, summary_len, "%s", "Forecast continues through the day.");

    if (!cJSON_IsArray(hourly_time) || !cJSON_IsArray(hourly_temperature) ||
        !cJSON_IsArray(hourly_weather_code) || (today_date == NULL) ||
        !forecast_parse_hour(current_time_text, current_date, sizeof(current_date),
                             &current_hour) ||
        (strcmp(current_date, today_date) != 0)) {
        return;
    }

    hourly_count = cJSON_GetArraySize(hourly_time);
    for (index = 0; index < hourly_count; index++) {
        cJSON *time_item = cJSON_GetArrayItem(hourly_time, index);
        int distance;

        if (!cJSON_IsString(time_item) ||
            !forecast_parse_hour(time_item->valuestring, point_date, sizeof(point_date),
                                 &point_hour) ||
            (strcmp(point_date, today_date) != 0) || (point_hour <= current_hour)) {
            continue;
        }

        if (!cJSON_IsNumber(cJSON_GetArrayItem(hourly_temperature, index)) ||
            !cJSON_IsNumber(cJSON_GetArrayItem(hourly_weather_code, index))) {
            continue;
        }

        distance = abs(point_hour - FORECAST_TARGET_EVENING_HOUR);
        if ((selected_index < 0) || (distance < selected_distance)) {
            selected_index = index;
            selected_distance = distance;
        }
    }

    if (selected_index < 0) {
        return;
    }

    selected_temperature = cJSON_GetArrayItem(hourly_temperature, selected_index);
    selected_weather_code = cJSON_GetArrayItem(hourly_weather_code, selected_index);
    later_temperature = selected_temperature->valuedouble;
    temperature_delta = (int)(later_temperature - current_temperature);
    current_sky_rank = forecast_weather_code_to_sky_rank(current_weather_code);
    later_sky_rank = forecast_weather_code_to_sky_rank(selected_weather_code->valueint);

    if (later_sky_rank >= (current_sky_rank + 2)) {
        snprintf(summary, summary_len, "%s", "Cloudier by evening.");
    } else if (current_sky_rank >= (later_sky_rank + 2)) {
        snprintf(summary, summary_len, "%s", "Clearer by evening.");
    } else if (temperature_delta >= 2) {
        snprintf(summary, summary_len, "%s", "Warmer later today.");
    } else if (temperature_delta <= -2) {
        snprintf(summary, summary_len, "%s", "Cooler this evening.");
    } else {
        snprintf(summary, summary_len, "%s", "Stays mild later today.");
    }
}

static const char *forecast_wind_direction_to_compass(double direction_degrees)
{
    static const char *labels[] = {
        "N", "NE", "E", "SE", "S", "SW", "W", "NW"
    };
    int index;

    while (direction_degrees < 0.0) {
        direction_degrees += 360.0;
    }

    while (direction_degrees >= 360.0) {
        direction_degrees -= 360.0;
    }

    index = (int)((direction_degrees + 22.5) / 45.0) % 8;
    return labels[index];
}

static void forecast_format_day_label(const char *date_text, char *label, size_t label_len)
{
    static const char *weekday_labels[] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    static const int month_offsets[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    int year;
    int month;
    int day;
    int weekday;

    if ((label == NULL) || (label_len == 0U)) {
        return;
    }

    if ((date_text == NULL) ||
        (sscanf(date_text, "%4d-%2d-%2d", &year, &month, &day) != 3) ||
        (month < 1) || (month > 12) || (day < 1) || (day > 31)) {
        snprintf(label, label_len, "%s", "--");
        return;
    }

    if (month < 3) {
        year--;
    }

    weekday = (year + (year / 4) - (year / 100) + (year / 400) +
               month_offsets[month - 1] + day) % 7;
    snprintf(label, label_len, "%s", weekday_labels[weekday]);
}

static bool forecast_load_location(double *latitude, double *longitude)
{
    gui_location_settings_t location = { 0 };
    char *end_lat = NULL;
    char *end_lon = NULL;

    if ((latitude == NULL) || (longitude == NULL)) {
        return false;
    }

    if (((s_bindings.gui == NULL) || !gui_get_location_settings(s_bindings.gui, &location)) &&
        s_bindings.has_last_location) {
        location = s_bindings.last_location;
    }

    if (!parse_coordinate_in_range(location.latitude, -90.0, 90.0) ||
        !parse_coordinate_in_range(location.longitude, -180.0, 180.0) ||
        (location.latitude[0] == '\0') || (location.longitude[0] == '\0')) {
        return false;
    }

    *latitude = strtod(location.latitude, &end_lat);
    *longitude = strtod(location.longitude, &end_lon);
    if ((end_lat == location.latitude) || (end_lon == location.longitude) ||
        (end_lat == NULL) || (end_lon == NULL) || (*end_lat != '\0') || (*end_lon != '\0')) {
        return false;
    }

    return true;
}

static bool forecast_parse_response(const char *response, gui_forecast_state_t *forecast)
{
    cJSON *root;
    cJSON *current;
    cJSON *daily;
    cJSON *hourly;
    cJSON *current_time;
    cJSON *current_temperature;
    cJSON *current_humidity;
    cJSON *current_weather_code;
    cJSON *hourly_time;
    cJSON *hourly_temperature;
    cJSON *hourly_weather_code;
    cJSON *daily_time;
    cJSON *daily_weather_code;
    cJSON *daily_temperature_max;
    cJSON *daily_temperature_min;
    cJSON *daily_precipitation_probability_max;
    cJSON *daily_uv_index_max;
    cJSON *daily_wind_speed_max;
    cJSON *daily_wind_direction_dominant;
    cJSON *today_temp_max;
    cJSON *today_temp_min;
    cJSON *today_precip_probability_max;
    cJSON *today_uv_index_max;
    cJSON *today_wind_speed_max;
    cJSON *today_wind_direction_dominant;
    cJSON *today_date;
    int day_count;
    int day_index;

#define FORECAST_PARSE_FAIL(fmt, ...)                           \
    do {                                                        \
        ESP_LOGW(TAG, "Forecast parse error: " fmt, ##__VA_ARGS__); \
        cJSON_Delete(root);                                     \
        return false;                                           \
    } while (0)

    if ((response == NULL) || (forecast == NULL)) {
        return false;
    }

    root = cJSON_Parse(response);
    if (root == NULL) {
        ESP_LOGW(TAG, "Forecast parse error: invalid JSON payload");
        return false;
    }

    current = cJSON_GetObjectItemCaseSensitive(root, "current");
    daily = cJSON_GetObjectItemCaseSensitive(root, "daily");
    hourly = cJSON_GetObjectItemCaseSensitive(root, "hourly");
    current_time = cJSON_GetObjectItemCaseSensitive(current, "time");
    current_temperature = cJSON_GetObjectItemCaseSensitive(current, "temperature_2m");
    current_humidity = cJSON_GetObjectItemCaseSensitive(current, "relative_humidity_2m");
    current_weather_code = cJSON_GetObjectItemCaseSensitive(current, "weather_code");
    hourly_time = cJSON_GetObjectItemCaseSensitive(hourly, "time");
    hourly_temperature = cJSON_GetObjectItemCaseSensitive(hourly, "temperature_2m");
    hourly_weather_code = cJSON_GetObjectItemCaseSensitive(hourly, "weather_code");
    daily_time = cJSON_GetObjectItemCaseSensitive(daily, "time");
    daily_weather_code = cJSON_GetObjectItemCaseSensitive(daily, "weather_code");
    daily_temperature_max = cJSON_GetObjectItemCaseSensitive(daily, "temperature_2m_max");
    daily_temperature_min = cJSON_GetObjectItemCaseSensitive(daily, "temperature_2m_min");
    daily_precipitation_probability_max =
        cJSON_GetObjectItemCaseSensitive(daily, "precipitation_probability_max");
    daily_uv_index_max = cJSON_GetObjectItemCaseSensitive(daily, "uv_index_max");
    daily_wind_speed_max = cJSON_GetObjectItemCaseSensitive(daily, "wind_speed_10m_max");
    daily_wind_direction_dominant =
        cJSON_GetObjectItemCaseSensitive(daily, "wind_direction_10m_dominant");

    if (!cJSON_IsObject(current)) {
        FORECAST_PARSE_FAIL("missing current object");
    }
    if (!cJSON_IsObject(daily)) {
        FORECAST_PARSE_FAIL("missing daily object");
    }
    if (!cJSON_IsNumber(current_temperature)) {
        FORECAST_PARSE_FAIL("missing current.temperature_2m");
    }
    if (!cJSON_IsNumber(current_weather_code)) {
        FORECAST_PARSE_FAIL("missing current.weather_code");
    }
    if (!cJSON_IsArray(daily_time)) {
        FORECAST_PARSE_FAIL("missing daily.time");
    }
    if (!cJSON_IsArray(daily_weather_code)) {
        FORECAST_PARSE_FAIL("missing daily.weather_code");
    }
    if (!cJSON_IsArray(daily_temperature_max)) {
        FORECAST_PARSE_FAIL("missing daily.temperature_2m_max");
    }
    if (!cJSON_IsArray(daily_temperature_min)) {
        FORECAST_PARSE_FAIL("missing daily.temperature_2m_min");
    }

    day_count = cJSON_GetArraySize(daily_time);
    if ((day_count < GUI_FORECAST_DAY_COUNT) ||
        (cJSON_GetArraySize(daily_weather_code) < day_count) ||
        (cJSON_GetArraySize(daily_temperature_max) < day_count) ||
        (cJSON_GetArraySize(daily_temperature_min) < day_count)) {
        FORECAST_PARSE_FAIL("daily arrays shorter than expected: day_count=%d", day_count);
    }

    today_date = cJSON_GetArrayItem(daily_time, 0);
    today_temp_max = cJSON_GetArrayItem(daily_temperature_max, 0);
    today_temp_min = cJSON_GetArrayItem(daily_temperature_min, 0);
    today_precip_probability_max = cJSON_IsArray(daily_precipitation_probability_max) ?
        cJSON_GetArrayItem(daily_precipitation_probability_max, 0) : NULL;
    today_uv_index_max = cJSON_IsArray(daily_uv_index_max) ?
        cJSON_GetArrayItem(daily_uv_index_max, 0) : NULL;
    today_wind_speed_max = cJSON_IsArray(daily_wind_speed_max) ?
        cJSON_GetArrayItem(daily_wind_speed_max, 0) : NULL;
    today_wind_direction_dominant = cJSON_IsArray(daily_wind_direction_dominant) ?
        cJSON_GetArrayItem(daily_wind_direction_dominant, 0) : NULL;
    if (!cJSON_IsString(today_date) ||
        !cJSON_IsNumber(today_temp_max) || !cJSON_IsNumber(today_temp_min)) {
        FORECAST_PARSE_FAIL("today core fields missing");
    }

    forecast->has_data = true;
    snprintf(forecast->title, sizeof(forecast->title), "%s", "Today");
    snprintf(forecast->condition, sizeof(forecast->condition), "%s",
             forecast_weather_code_to_condition(current_weather_code->valueint));
    snprintf(forecast->current_temperature, sizeof(forecast->current_temperature), "%.0f C",
             current_temperature->valuedouble);
    snprintf(forecast->range_text, sizeof(forecast->range_text),
             "High %.0f C  |  Low %.0f C",
             today_temp_max->valuedouble, today_temp_min->valuedouble);
    forecast_format_later_today_summary(hourly_time, hourly_temperature, hourly_weather_code,
                                        today_date->valuestring,
                                        cJSON_IsString(current_time) ?
                                            current_time->valuestring : NULL,
                                        current_temperature->valuedouble,
                                        current_weather_code->valueint,
                                        forecast->summary, sizeof(forecast->summary));

    if (cJSON_IsNumber(today_precip_probability_max)) {
        snprintf(forecast->details.rain_chance, sizeof(forecast->details.rain_chance),
                 "Rain chance: %.0f%%", today_precip_probability_max->valuedouble);
    } else {
        snprintf(forecast->details.rain_chance, sizeof(forecast->details.rain_chance), "%s",
                 "Rain chance: N/A");
    }

    if (cJSON_IsNumber(today_wind_speed_max) && cJSON_IsNumber(today_wind_direction_dominant)) {
        snprintf(forecast->details.wind, sizeof(forecast->details.wind), "Wind: %.0f m/s %s",
                 today_wind_speed_max->valuedouble,
                 forecast_wind_direction_to_compass(today_wind_direction_dominant->valuedouble));
    } else {
        snprintf(forecast->details.wind, sizeof(forecast->details.wind), "%s", "Wind: N/A");
    }

    if (cJSON_IsNumber(current_humidity)) {
        snprintf(forecast->details.humidity, sizeof(forecast->details.humidity),
                 "Humidity: %.0f%%", current_humidity->valuedouble);
    } else {
        snprintf(forecast->details.humidity, sizeof(forecast->details.humidity), "%s",
                 "Humidity: N/A");
    }

    if (cJSON_IsNumber(today_uv_index_max)) {
        snprintf(forecast->details.uv_index, sizeof(forecast->details.uv_index),
                 "UV index: %.0f", today_uv_index_max->valuedouble);
    } else {
        snprintf(forecast->details.uv_index, sizeof(forecast->details.uv_index), "%s",
                 "UV index: N/A");
    }

    for (day_index = 0; day_index < GUI_FORECAST_DAY_COUNT; day_index++) {
        cJSON *time_item = cJSON_GetArrayItem(daily_time, day_index);
        cJSON *weather_code_item = cJSON_GetArrayItem(daily_weather_code, day_index);
        cJSON *temp_max_item = cJSON_GetArrayItem(daily_temperature_max, day_index);
        cJSON *temp_min_item = cJSON_GetArrayItem(daily_temperature_min, day_index);

        if (!cJSON_IsString(time_item) || !cJSON_IsNumber(weather_code_item) ||
            !cJSON_IsNumber(temp_max_item) || !cJSON_IsNumber(temp_min_item)) {
            FORECAST_PARSE_FAIL("day %d core fields missing", day_index);
        }

        forecast_format_day_label(time_item->valuestring, forecast->days[day_index].label,
                                  sizeof(forecast->days[day_index].label));
        snprintf(forecast->days[day_index].condition,
                 sizeof(forecast->days[day_index].condition), "%s",
                 forecast_weather_code_to_condition(weather_code_item->valueint));
        snprintf(forecast->days[day_index].range_text,
                 sizeof(forecast->days[day_index].range_text), "%.0f / %.0f C",
                 temp_max_item->valuedouble, temp_min_item->valuedouble);
    }

    cJSON_Delete(root);
    return true;

#undef FORECAST_PARSE_FAIL
}

static uint16_t leop_scale_hourly_total(double hourly_total)
{
    double scaled_value;

    if (hourly_total <= 0.0) {
        return 0U;
    }

    scaled_value = (hourly_total * LEOP_VALUE_SCALE) + 0.5;
    if (scaled_value >= (double)UINT16_MAX) {
        return UINT16_MAX;
    }

    return (uint16_t)scaled_value;
}

static bool leop_parse_series(const cJSON *series_array, uint16_t *values, const char *series_name)
{
    int hour_index;

    if ((series_array == NULL) || (values == NULL) || (series_name == NULL)) {
        return false;
    }

    if (!cJSON_IsArray(series_array)) {
        ESP_LOGW(TAG, "LEOP parse error: %s is not an array", series_name);
        return false;
    }

    if (cJSON_GetArraySize(series_array) < LEOP_RAW_POINT_COUNT) {
        ESP_LOGW(TAG, "LEOP parse error: %s shorter than %d points", series_name,
                 LEOP_RAW_POINT_COUNT);
        return false;
    }

    for (hour_index = 0; hour_index < GUI_ENERGY_PLAN_POINT_COUNT; hour_index++) {
        double hourly_total = 0.0;
        int quarter_index;

        for (quarter_index = 0; quarter_index < LEOP_POINTS_PER_HOUR; quarter_index++) {
            const cJSON *sample = cJSON_GetArrayItem(series_array,
                                                     (hour_index * LEOP_POINTS_PER_HOUR) +
                                                     quarter_index);

            if (!cJSON_IsNumber(sample)) {
                ESP_LOGW(TAG, "LEOP parse error: %s[%d] is not numeric", series_name,
                         (hour_index * LEOP_POINTS_PER_HOUR) + quarter_index);
                return false;
            }

            hourly_total += sample->valuedouble;
        }

        values[hour_index] = leop_scale_hourly_total(hourly_total);
    }

    return true;
}

static bool leop_parse_response(const char *response, gui_energy_plan_t *energy_plan)
{
    cJSON *root;
    cJSON *result;
    cJSON *buy_electricity;
    cJSON *direct_use;
    cJSON *charge_battery;
    cJSON *sell_excess;
    cJSON *timestamp;

#define LEOP_PARSE_FAIL(fmt, ...)                                  \
    do {                                                           \
        ESP_LOGW(TAG, "LEOP parse error: " fmt, ##__VA_ARGS__);    \
        cJSON_Delete(root);                                        \
        return false;                                              \
    } while (0)

    if ((response == NULL) || (energy_plan == NULL)) {
        return false;
    }

    root = cJSON_Parse(response);
    if (root == NULL) {
        ESP_LOGW(TAG, "LEOP parse error: invalid JSON payload");
        return false;
    }

    result = cJSON_GetObjectItemCaseSensitive(root, "result");
    if (!cJSON_IsObject(result)) {
        LEOP_PARSE_FAIL("missing result object");
    }

    buy_electricity = cJSON_GetObjectItemCaseSensitive(result, "buy_electricity");
    direct_use = cJSON_GetObjectItemCaseSensitive(result, "direct_use");
    charge_battery = cJSON_GetObjectItemCaseSensitive(result, "charge_battery");
    sell_excess = cJSON_GetObjectItemCaseSensitive(result, "sell_excess");
    timestamp = cJSON_GetObjectItemCaseSensitive(result, "timestamp");

    if (!cJSON_IsArray(timestamp) || (cJSON_GetArraySize(timestamp) < LEOP_RAW_POINT_COUNT)) {
        LEOP_PARSE_FAIL("timestamp array shorter than %d points", LEOP_RAW_POINT_COUNT);
    }

    memset(energy_plan, 0, sizeof(*energy_plan));

    if (!leop_parse_series(buy_electricity, energy_plan->buy_electricity, "buy_electricity") ||
        !leop_parse_series(direct_use, energy_plan->use_solar_directly, "direct_use") ||
        !leop_parse_series(charge_battery, energy_plan->charge_battery, "charge_battery") ||
        !leop_parse_series(sell_excess, energy_plan->sell_excess, "sell_excess")) {
        cJSON_Delete(root);
        return false;
    }

    cJSON_Delete(root);
    return true;

#undef LEOP_PARSE_FAIL
}

bool app_gui_bindings_load_saved_appearance(gui_init_config_t *config)
{
    uint8_t value = 0;
    bool loaded_any = false;

    if (config == NULL) {
        return false;
    }

    memset(config, 0, sizeof(*config));

    if (rm_nvs_get_u8(GUI_NVS_KEY_THEME, &value) == ESP_OK) {
        config->has_theme = true;
        config->theme = (gui_view_theme_t)value;
        loaded_any = true;
    }

    if (rm_nvs_get_u8(GUI_NVS_KEY_BG, &value) == ESP_OK) {
        config->has_background_image = true;
        config->show_background_image = (value != 0U);
        loaded_any = true;
    }

    if (rm_nvs_get_u8(GUI_NVS_KEY_NIGHT, &value) == ESP_OK) {
        config->has_night_variant = true;
        config->night_variant_enabled = (value != 0U);
        loaded_any = true;
    }

    if (rm_nvs_get_u8(GUI_NVS_KEY_BRIGHT, &value) == ESP_OK) {
        config->has_brightness = true;
        config->brightness_percent = clamp_saved_brightness((int32_t)value);
        loaded_any = true;
    }

    return loaded_any;
}

static void cache_current_appearance(gui_ctx_t *gui)
{
    gui_appearance_settings_t appearance;
    int32_t brightness = 0;

    if ((gui == NULL) || !gui_get_appearance_settings(gui, &appearance) ||
        !gui_get_brightness(gui, &brightness)) {
        return;
    }

    s_bindings.last_appearance = appearance;
    s_bindings.has_last_appearance = true;
    s_bindings.last_brightness = clamp_saved_brightness(brightness);
    s_bindings.has_last_brightness = true;
}

static void cache_current_location(gui_ctx_t *gui)
{
    gui_location_settings_t location;

    if ((gui == NULL) || !gui_get_location_settings(gui, &location)) {
        return;
    }

    s_bindings.last_location = location;
    s_bindings.has_last_location = true;
}

static bool save_appearance_if_changed(gui_ctx_t *gui)
{
    gui_appearance_settings_t appearance;
    int32_t brightness = 0;
    bool appearance_changed;
    bool brightness_changed;
    esp_err_t err;

    if ((gui == NULL) || !gui_get_appearance_settings(gui, &appearance) ||
        !gui_get_brightness(gui, &brightness)) {
        return false;
    }

    brightness = clamp_saved_brightness(brightness);

    appearance_changed = !s_bindings.has_last_appearance || (appearance.theme != s_bindings.last_appearance.theme) || (appearance.show_background_image != s_bindings.last_appearance.show_background_image) || (appearance.night_variant_enabled != s_bindings.last_appearance.night_variant_enabled);

    brightness_changed = !s_bindings.has_last_brightness || (brightness != s_bindings.last_brightness);

    if (!appearance_changed && !brightness_changed) {
        return false;
    }

    if (appearance_changed) {
        err = rm_nvs_set_u8(GUI_NVS_KEY_THEME, (uint8_t)appearance.theme);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "rm_nvs_set_u8(%s) failed: %s", GUI_NVS_KEY_THEME, esp_err_to_name(err));
            return false;
        }

        err = rm_nvs_set_u8(GUI_NVS_KEY_BG, appearance.show_background_image ? 1U : 0U);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "rm_nvs_set_u8(%s) failed: %s", GUI_NVS_KEY_BG, esp_err_to_name(err));
            return false;
        }

        err = rm_nvs_set_u8(GUI_NVS_KEY_NIGHT, appearance.night_variant_enabled ? 1U : 0U);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "rm_nvs_set_u8(%s) failed: %s", GUI_NVS_KEY_NIGHT, esp_err_to_name(err));
            return false;
        }

        s_bindings.last_appearance = appearance;
        s_bindings.has_last_appearance = true;
    }

    if (brightness_changed) {
        err = rm_nvs_set_u8(GUI_NVS_KEY_BRIGHT, (uint8_t)brightness);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "rm_nvs_set_u8(%s) failed: %s", GUI_NVS_KEY_BRIGHT, esp_err_to_name(err));
            return false;
        }

        s_bindings.last_brightness = brightness;
        s_bindings.has_last_brightness = true;
    }

    return true;
}

static bool load_saved_location(gui_ctx_t *gui)
{
    gui_location_settings_t location = { 0 };
    size_t latitude_len = sizeof(location.latitude);
    size_t longitude_len = sizeof(location.longitude);
    bool loaded_any = false;

    if (gui == NULL) {
        return false;
    }

    if (rm_nvs_get_str(GUI_NVS_KEY_LAT, location.latitude, &latitude_len) == ESP_OK) {
        loaded_any = true;
    } else {
        location.latitude[0] = '\0';
    }

    if (rm_nvs_get_str(GUI_NVS_KEY_LON, location.longitude, &longitude_len) == ESP_OK) {
        loaded_any = true;
    } else {
        location.longitude[0] = '\0';
    }

    if (!loaded_any) {
        return false;
    }

    if (!parse_coordinate_in_range(location.latitude, -90.0, 90.0)) {
        location.latitude[0] = '\0';
    }

    if (!parse_coordinate_in_range(location.longitude, -180.0, 180.0)) {
        location.longitude[0] = '\0';
    }

    gui_set_location_settings(gui, &location);
    cache_current_location(gui);
    return true;
}

static bool save_location_if_changed(gui_ctx_t *gui)
{
    gui_location_settings_t location;
    gui_location_settings_t fallback_location = { 0 };
    bool latitude_valid;
    bool longitude_valid;
    esp_err_t err;

    if ((gui == NULL) || !gui_get_location_settings(gui, &location)) {
        return false;
    }

    if (s_bindings.has_last_location) {
        fallback_location = s_bindings.last_location;
    }

    latitude_valid = parse_coordinate_in_range(location.latitude, -90.0, 90.0);
    longitude_valid = parse_coordinate_in_range(location.longitude, -180.0, 180.0);

    if (!latitude_valid) {
        snprintf(location.latitude, sizeof(location.latitude), "%s",
                 fallback_location.latitude);
    }

    if (!longitude_valid) {
        snprintf(location.longitude, sizeof(location.longitude), "%s",
                 fallback_location.longitude);
    }

    if (!latitude_valid || !longitude_valid) {
        gui_set_location_settings(gui, &location);
    }

    if (!latitude_valid && !longitude_valid && !s_bindings.has_last_location) {
        return false;
    }

    if (s_bindings.has_last_location &&
        (strcmp(location.latitude, s_bindings.last_location.latitude) == 0) &&
        (strcmp(location.longitude, s_bindings.last_location.longitude) == 0)) {
        return false;
    }

    err = rm_nvs_set_str(GUI_NVS_KEY_LAT, location.latitude);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "rm_nvs_set_str(%s) failed: %s", GUI_NVS_KEY_LAT, esp_err_to_name(err));
        return false;
    }

    err = rm_nvs_set_str(GUI_NVS_KEY_LON, location.longitude);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "rm_nvs_set_str(%s) failed: %s", GUI_NVS_KEY_LON, esp_err_to_name(err));
        return false;
    }

    s_bindings.last_location = location;
    s_bindings.has_last_location = true;
    return true;
}

static bool load_saved_wifi_metadata(gui_ctx_t *gui)
{
    gui_wifi_settings_t wifi = { 0 };
    char saved_ssid[GUI_WIFI_SSID_MAX_LEN] = { 0 };
    size_t saved_ssid_len = sizeof(saved_ssid);

    if ((gui == NULL) || !gui_get_wifi_settings(gui, &wifi)) {
        return false;
    }

    if (rm_nvs_get_str(GUI_NVS_KEY_WIFI_SSID, saved_ssid, &saved_ssid_len) != ESP_OK) {
        return false;
    }

    memset(wifi.known_networks, 0, sizeof(wifi.known_networks));
    snprintf(wifi.known_networks[0].ssid, sizeof(wifi.known_networks[0].ssid), "%s", saved_ssid);
    wifi.known_networks[0].secured = true;
    wifi.known_networks[0].signal_strength_pct = 0;
    wifi.known_network_count = 1;

    if (wifi.selected_ssid[0] == '\0') {
        snprintf(wifi.selected_ssid, sizeof(wifi.selected_ssid), "%s", saved_ssid);
    }

    snprintf(s_bindings.requested_ssid, sizeof(s_bindings.requested_ssid), "%s", saved_ssid);
    gui_set_wifi_settings(gui, &wifi);
    return true;
}

static bool refresh_saved_wifi_metadata(gui_ctx_t *gui)
{
    return load_saved_wifi_metadata(gui);
}

static bool queue_saved_wifi_autoconnect(gui_ctx_t *gui)
{
    char status_text[GUI_WIFI_STATUS_TEXT_MAX_LEN];
    esp_err_t result;

    if ((gui == NULL) || s_bindings.boot_autoconnect_queued) {
        return false;
    }

    if (!load_saved_wifi_metadata(gui)) {
        return false;
    }

    s_bindings.wifi_connect_requested = true;
    s_bindings.wifi_disconnect_requested = false;

    if (s_bindings.requested_ssid[0] != '\0') {
        snprintf(status_text, sizeof(status_text), "Connecting to %s...", s_bindings.requested_ssid);
        set_wifi_status(gui, status_text, GUI_WIFI_STATE_CONNECTING);
    } else {
        set_wifi_status(gui, "Connecting to Wi-Fi...", GUI_WIFI_STATE_CONNECTING);
    }

    result = nac_request_wifi_connect(NULL, NULL);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "nac_request_wifi_connect(NULL, NULL) failed: %s", esp_err_to_name(result));
        s_bindings.wifi_connect_requested = false;
        set_wifi_status(gui, "Failed to request Wi-Fi connection.", GUI_WIFI_STATE_FAILED);
        return false;
    }

    s_bindings.boot_autoconnect_queued = true;
    return true;
}

// UI Events
//////////////////////////

static void on_panel_changed(gui_ctx_t *gui, gui_panel_id_t panel, void *user_data)
{
    (void)gui;
    (void)user_data;
    ESP_LOGI(TAG, "GUI panel changed to %d", (int)panel);
}

static bool on_wifi_scan_requested(gui_ctx_t *gui, void *user_data)
{
    esp_err_t result;
    gui_wifi_settings_t wifi = { 0 };

    (void)user_data;

    s_bindings.wifi_connect_requested = false;
    s_bindings.wifi_disconnect_requested = false;
    s_bindings.requested_ssid[0] = '\0';

    if ((gui != NULL) && gui_get_wifi_settings(gui, &wifi)) {
        wifi.connect_requested = false;
        wifi.can_disconnect = false;
        wifi.selected_ssid[0] = '\0';
        gui_set_wifi_settings(gui, &wifi);
    }

    result = nac_request_wifi_scan();
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "nac_request_wifi_scan failed: %s", esp_err_to_name(result));
        set_wifi_status(gui, "Failed to start Wi-Fi scan.", GUI_WIFI_STATE_FAILED);
        return true;
    }

    s_bindings.wifi_scan_requested = true;
    set_wifi_status(gui, "Scanning for Wi-Fi networks...", GUI_WIFI_STATE_SCANNED);
    return true;
}

static void on_wifi_network_selected(gui_ctx_t *gui, const gui_wifi_network_t *network, void *user_data)
{
    (void)gui;
    (void)user_data;

    if (network == NULL) {
        return;
    }

    ESP_LOGI(TAG, "GUI selected Wi-Fi network: %s", network->ssid);
}

static bool on_wifi_known_network_requested(gui_ctx_t *gui, const gui_wifi_network_t *network, void *user_data)
{
    const char *ssid = NULL;

    if (network != NULL) {
        ssid = network->ssid;
    }

    return on_wifi_connect_requested(gui, ssid, NULL, user_data);
}

static bool on_wifi_connect_requested(gui_ctx_t *gui, const char *ssid, const char *password, void *user_data)
{
    esp_err_t result;

    (void)user_data;

    result = nac_request_wifi_connect(ssid, password);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "nac_request_wifi_connect failed: %s", esp_err_to_name(result));
        set_wifi_status(gui, "Failed to request Wi-Fi connection.", GUI_WIFI_STATE_FAILED);
        return true;
    }

    s_bindings.wifi_connect_requested = true;
    s_bindings.wifi_disconnect_requested = false;
    if (ssid != NULL) {
        snprintf(s_bindings.requested_ssid, sizeof(s_bindings.requested_ssid), "%s", ssid);
    } else {
        s_bindings.requested_ssid[0] = '\0';
    }
    ESP_LOGI(TAG, "GUI requested Wi-Fi connection for %s", (ssid != NULL) ? ssid : "<none>");
    if ((ssid != NULL) && (ssid[0] != '\0')) {
        char status_text[GUI_WIFI_STATUS_TEXT_MAX_LEN];

        snprintf(status_text, sizeof(status_text), "Connecting to %s...", ssid);
        set_wifi_status(gui, status_text, GUI_WIFI_STATE_CONNECTING);
    } else {
        set_wifi_status(gui, "Connecting to Wi-Fi...", GUI_WIFI_STATE_CONNECTING);
    }
    return true;
}

static bool on_wifi_disconnect_requested(gui_ctx_t *gui, void *user_data)
{
    esp_err_t result;

    (void)user_data;

    result = nac_request_wifi_disconnect();
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "nac_request_wifi_disconnect failed: %s", esp_err_to_name(result));
        set_wifi_status(gui, "Failed to disconnect Wi-Fi.", GUI_WIFI_STATE_FAILED);
        return true;
    }

    s_bindings.wifi_connect_requested = false;
    s_bindings.wifi_disconnect_requested = true;
    ESP_LOGI(TAG, "GUI requested Wi-Fi disconnect");
    set_wifi_status(gui, "Disconnected.", GUI_WIFI_STATE_IDLE);
    return true;
}

static task_node_t forecast_task = {0};
static task_node_t leop_task = {0};
static task_node_t sensor_task = {0};

task_status_t forecast_work(task_node_t *node) {
    ESP_LOGI(TAG, "Fetching forecast...");
    char url[1024];
    double latitude;
    double longitude;
    enum { RESPONSE_BUF_LEN = 16384 };
    char *buf;
    gui_forecast_state_t forecast;
    esp_err_t http_rc;

    if (node == NULL) {
        return TASK_ERROR;
    }

    node->run_at_tick = xTaskGetTickCount() + pdMS_TO_TICKS(FORECAST_REFRESH_DELAY_MS);

    if (s_bindings.gui == NULL) {
        ESP_LOGW(TAG, "Skipping forecast refresh without GUI context.");
        return TASK_RUN_AGAIN;
    }

    if (!forecast_load_location(&latitude, &longitude)) {
        ESP_LOGW(TAG, "Skipping forecast refresh until a valid location is available.");
        return TASK_RUN_AGAIN;
    }

    snprintf(url, sizeof(url),
             "https://api.open-meteo.com/v1/forecast?latitude=%.2f&longitude=%.2f"
             "&timezone=auto"
             "&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m,wind_direction_10m"
             "&hourly=temperature_2m,weather_code"
             "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max,uv_index_max,wind_speed_10m_max,wind_direction_10m_dominant"
             "&forecast_days=5",
             latitude, longitude);

    buf = malloc(RESPONSE_BUF_LEN + 1U);

    if (buf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate forecast response buffer");
        return TASK_RUN_AGAIN;
    }

    buf[0] = '\0';
    http_rc = http_client_get(url, buf, RESPONSE_BUF_LEN + 1U);
    if (http_rc != ESP_OK) {
        ESP_LOGW(TAG, "Forecast request failed: %s", esp_err_to_name(http_rc));
        free(buf);
        return TASK_RUN_AGAIN;
    }

    if (!gui_get_forecast_state(s_bindings.gui, &forecast)) {
        ESP_LOGW(TAG, "Failed to load current forecast GUI state.");
        free(buf);
        return TASK_RUN_AGAIN;
    }

    if (!forecast_parse_response(buf, &forecast)) {
        ESP_LOGW(TAG, "Failed to parse forecast response.");
        free(buf);
        return TASK_RUN_AGAIN;
    }

    gui_set_forecast_state(s_bindings.gui, &forecast);
    free(buf);
    return TASK_RUN_AGAIN;
}

task_status_t leop_work(task_node_t *node) {
    ESP_LOGI(TAG, "Fetching LEOP...");
    enum { RESPONSE_BUF_LEN = 8192 };
    char *buf;
    gui_energy_plan_t energy_plan;
    esp_err_t http_rc;

    if (node == NULL) {
        return TASK_ERROR;
    }

    node->run_at_tick = xTaskGetTickCount() + pdMS_TO_TICKS(LEOP_REFRESH_DELAY_MS);

    if (s_bindings.gui == NULL) {
        ESP_LOGW(TAG, "Skipping LEOP refresh without GUI context.");
        return TASK_RUN_AGAIN;
    }

    buf = malloc(RESPONSE_BUF_LEN + 1U);

    if (buf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate LEOP response buffer");
        return TASK_RUN_AGAIN;
    }

    buf[0] = '\0';
    http_rc = http_client_get(LEOP_REFRESH_URL, buf, RESPONSE_BUF_LEN + 1U);
    if (http_rc != ESP_OK) {
        ESP_LOGW(TAG, "LEOP request failed: %s", esp_err_to_name(http_rc));
        free(buf);
        return TASK_RUN_AGAIN;
    }

    if (!gui_get_energy_plan_state(s_bindings.gui, &energy_plan)) {
        ESP_LOGW(TAG, "Failed to load current LEOP GUI state.");
        free(buf);
        return TASK_RUN_AGAIN;
    }

    if (!leop_parse_response(buf, &energy_plan)) {
        ESP_LOGW(TAG, "Failed to parse LEOP response.");
        free(buf);
        return TASK_RUN_AGAIN;
    }

    gui_set_energy_plan_state(s_bindings.gui, &energy_plan);
    free(buf);
    return TASK_RUN_AGAIN;
}

task_status_t sensor_work(task_node_t *node) {
    ESP_LOGI(TAG, "Fetching sensor...");

    node->run_at_tick = xTaskGetTickCount() + pdMS_TO_TICKS(FORECAST_REFRESH_DELAY_MS);
    return TASK_RUN_AGAIN;
}

// Basics
//////////////////////////

esp_err_t app_gui_bindings_init(gui_ctx_t *gui)
{
    gui_module_bindings_t bindings = { 0 };

    if (gui == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(&s_bindings, 0, sizeof(s_bindings));
    s_bindings.gui = gui;
    s_bindings.startup_clock_refresh_pending = true;

    bindings.user_data = &s_bindings;
    bindings.on_panel_changed = on_panel_changed;
    bindings.on_wifi_scan_requested = on_wifi_scan_requested;
    bindings.on_wifi_network_selected = on_wifi_network_selected;
    bindings.on_wifi_known_network_requested = on_wifi_known_network_requested;
    bindings.on_wifi_connect_requested = on_wifi_connect_requested;
    bindings.on_wifi_disconnect_requested = on_wifi_disconnect_requested;
    gui_set_bindings(gui, &bindings);

    cache_current_appearance(gui);
    cache_current_location(gui);
    (void)load_saved_location(gui);
    (void)load_saved_wifi_metadata(gui);
    app_gui_bindings_sync(gui);
    (void)queue_saved_wifi_autoconnect(gui);

    // Add tasks to task-scheduler for fetching data from sensor, LEOP and forecast
    int rc = 0;

    forecast_task.work = forecast_work;
    rc = task_scheduler_add(&forecast_task, 5000U);
    if (rc < 0) {
        ESP_LOGE(TAG, "Failed to add forecast fetching task to scheduler.");
    }

    leop_task.work = leop_work;
    rc = task_scheduler_add(&leop_task, 5000U);
    if (rc < 0) {
        ESP_LOGE(TAG, "Failed to add LEOP fetching task to scheduler.");
    }

    sensor_task.work = sensor_work;
    rc = task_scheduler_add(&sensor_task, 5000U);
    if (rc < 0) {
        ESP_LOGE(TAG, "Failed to add sensor fetching task to scheduler.");
    }

    return ESP_OK;
}

void app_gui_bindings_sync(gui_ctx_t *gui)
{
    nac_wifi_status_t wifi_status;

    if (gui == NULL) {
        return;
    }

    wifi_status = nac_get_wifi_status();
    sync_startup_sidebar_clock(gui, wifi_status);

    bool wifi_state_changed = sync_wifi_state(gui);
    if (wifi_state_changed || s_bindings.wifi_scan_requested || s_bindings.wifi_connect_requested || s_bindings.wifi_disconnect_requested) {
        sync_wifi(gui);
    }

    sync_sensor(gui);
    
    if (sync_sd_card_state(gui)) {
        ESP_LOGE(TAG, "Syncing SD-card state failed.");
    }

    (void)save_appearance_if_changed(gui);
    (void)save_location_if_changed(gui);
}
