#include "app_gui_bindings_internal.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "http_client.h"

#define FORECAST_REFRESH_DELAY_MS 60000U
#define FORECAST_INITIAL_DELAY_MS 5000U
#define FORECAST_RESPONSE_BUF_LEN 16384U

static char s_forecast_response_buf[FORECAST_RESPONSE_BUF_LEN + 1U];

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

static gui_weather_icon_t forecast_weather_code_to_icon(int weather_code)
{
    switch (weather_code) {
        case 0:
        case 1:
            return GUI_WEATHER_ICON_CLEAR;
        case 2:
            return GUI_WEATHER_ICON_PARTLY_CLOUDY;
        case 3:
            return GUI_WEATHER_ICON_CLOUDY;
        case 45:
        case 48:
            return GUI_WEATHER_ICON_FOG;
        case 51:
        case 53:
        case 55:
        case 56:
        case 57:
            return GUI_WEATHER_ICON_DRIZZLE;
        case 61:
        case 63:
        case 65:
        case 66:
        case 67:
        case 80:
        case 81:
        case 82:
            return GUI_WEATHER_ICON_RAIN;
        case 71:
        case 73:
        case 75:
        case 77:
        case 85:
        case 86:
            return GUI_WEATHER_ICON_SNOW;
        case 95:
        case 96:
        case 99:
            return GUI_WEATHER_ICON_THUNDERSTORM;
        default:
            return GUI_WEATHER_ICON_CLOUDY;
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

    snprintf(summary, summary_len, "%s", "Continues all day.");

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

static bool forecast_parse_date_parts(const char *date_text, int *year, int *month, int *day)
{
    static const int days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int parsed_year;
    int parsed_month;
    int parsed_day;
    int chars_read = 0;
    int max_day;

    if ((date_text == NULL) || (month == NULL) || (day == NULL) ||
        (strlen(date_text) != 10U) || (date_text[4] != '-') || (date_text[7] != '-') ||
        (sscanf(date_text, "%4d-%2d-%2d%n", &parsed_year, &parsed_month, &parsed_day,
                &chars_read) != 3) ||
        (chars_read != 10) || (parsed_year < 1) || (parsed_month < 1) ||
        (parsed_month > 12)) {
        return false;
    }

    max_day = days_in_month[parsed_month - 1];
    if ((parsed_month == 2) &&
        (((parsed_year % 4) == 0) &&
         (((parsed_year % 100) != 0) || ((parsed_year % 400) == 0)))) {
        max_day = 29;
    }

    if ((parsed_day < 1) || (parsed_day > max_day)) {
        return false;
    }

    if (year != NULL) {
        *year = parsed_year;
    }
    *month = parsed_month;
    *day = parsed_day;
    return true;
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

    if (!forecast_parse_date_parts(date_text, &year, &month, &day)) {
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

static void forecast_format_day_date(const char *date_text, char *label, size_t label_len)
{
    static const char *month_labels[] = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };
    int month;
    int day;

    if ((label == NULL) || (label_len == 0U)) {
        return;
    }

    if (!forecast_parse_date_parts(date_text, NULL, &month, &day)) {
        snprintf(label, label_len, "%s", "--");
        return;
    }

    snprintf(label, label_len, "%s %d", month_labels[month - 1], day);
}

static bool forecast_parse_response(const char *response, gui_forecast_state_t *forecast)
{
    cJSON *root;
    cJSON *current;
    cJSON *daily;
    cJSON *hourly;
    cJSON *current_time;
    cJSON *current_temperature;
    cJSON *current_apparent_temperature;
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

#define FORECAST_PARSE_FAIL(fmt, ...)                               \
    do {                                                            \
        ESP_LOGW(APP_GUI_BINDINGS_TAG, "Forecast parse error: " fmt, ##__VA_ARGS__); \
        cJSON_Delete(root);                                         \
        return false;                                               \
    } while (0)

    if ((response == NULL) || (forecast == NULL)) {
        return false;
    }

    root = cJSON_Parse(response);
    if (root == NULL) {
        ESP_LOGW(APP_GUI_BINDINGS_TAG, "Forecast parse error: invalid JSON payload");
        return false;
    }

    current = cJSON_GetObjectItemCaseSensitive(root, "current");
    daily = cJSON_GetObjectItemCaseSensitive(root, "daily");
    hourly = cJSON_GetObjectItemCaseSensitive(root, "hourly");
    current_time = cJSON_GetObjectItemCaseSensitive(current, "time");
    current_temperature = cJSON_GetObjectItemCaseSensitive(current, "temperature_2m");
    current_apparent_temperature = cJSON_GetObjectItemCaseSensitive(current,
                                                                    "apparent_temperature");
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
    snprintf(forecast->title, sizeof(forecast->title), "%s", "Right now");
    snprintf(forecast->condition, sizeof(forecast->condition), "%s",
             forecast_weather_code_to_condition(current_weather_code->valueint));
    forecast->current_icon = forecast_weather_code_to_icon(current_weather_code->valueint);
    snprintf(forecast->current_temperature, sizeof(forecast->current_temperature), "%.0f C",
             current_temperature->valuedouble);
    if (cJSON_IsNumber(current_apparent_temperature)) {
        snprintf(forecast->feels_like_temperature, sizeof(forecast->feels_like_temperature),
                 "Feels like %.0f C", current_apparent_temperature->valuedouble);
    } else {
        snprintf(forecast->feels_like_temperature, sizeof(forecast->feels_like_temperature),
                 "%s", "Feels like N/A");
    }
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
        snprintf(forecast->details.wind, sizeof(forecast->details.wind), "Wind: %.0f m/s",
                 today_wind_speed_max->valuedouble);
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
        forecast_format_day_date(time_item->valuestring, forecast->days[day_index].date_text,
                                 sizeof(forecast->days[day_index].date_text));
        forecast->days[day_index].icon =
            forecast_weather_code_to_icon(weather_code_item->valueint);
        snprintf(forecast->days[day_index].range_text,
                 sizeof(forecast->days[day_index].range_text), "%.0f/%.0f C",
                 temp_min_item->valuedouble, temp_max_item->valuedouble);
    }

    cJSON_Delete(root);
    return true;

#undef FORECAST_PARSE_FAIL
}

static task_status_t forecast_work(task_node_t *node)
{
    app_gui_bindings_ctx_t *ctx;
    char url[1024];
    double latitude;
    double longitude;
    gui_forecast_state_t forecast;
    esp_err_t http_rc;

    ESP_LOGI(APP_GUI_BINDINGS_TAG, "Fetching forecast...");

    if (node == NULL) {
        return TASK_ERROR;
    }

    ctx = container_of(node, app_gui_bindings_ctx_t, forecast_task);
    node->run_at_tick = xTaskGetTickCount() + pdMS_TO_TICKS(FORECAST_REFRESH_DELAY_MS);

    if (ctx->gui == NULL) {
        ESP_LOGW(APP_GUI_BINDINGS_TAG, "Skipping forecast refresh without GUI context.");
        return TASK_RUN_AGAIN;
    }

    if (!app_gui_settings_load_location_for_forecast(ctx, &latitude, &longitude)) {
        ESP_LOGW(APP_GUI_BINDINGS_TAG, "Skipping forecast refresh until a valid location is available.");
        return TASK_RUN_AGAIN;
    }

    snprintf(url, sizeof(url),
             "https://api.open-meteo.com/v1/forecast?latitude=%.2f&longitude=%.2f"
             "&timezone=auto"
             "&current=temperature_2m,apparent_temperature,relative_humidity_2m,"
             "weather_code,wind_speed_10m,wind_direction_10m"
             "&hourly=temperature_2m,weather_code"
             "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max,uv_index_max,wind_speed_10m_max,wind_direction_10m_dominant"
             "&forecast_days=5",
             latitude, longitude);

    s_forecast_response_buf[0] = '\0';
    http_rc = http_client_get(url, s_forecast_response_buf,
                              sizeof(s_forecast_response_buf));
    if (http_rc != ESP_OK) {
        ESP_LOGW(APP_GUI_BINDINGS_TAG, "Forecast request failed: %s", esp_err_to_name(http_rc));
        return TASK_RUN_AGAIN;
    }

    if (!gui_get_forecast_state(ctx->gui, &forecast)) {
        ESP_LOGW(APP_GUI_BINDINGS_TAG, "Failed to load current forecast GUI state.");
        return TASK_RUN_AGAIN;
    }

    if (!forecast_parse_response(s_forecast_response_buf, &forecast)) {
        ESP_LOGW(APP_GUI_BINDINGS_TAG, "Failed to parse forecast response.");
        return TASK_RUN_AGAIN;
    }

    app_gui_time_format_last_updated_now(forecast.last_updated, sizeof(forecast.last_updated));
    gui_set_forecast_state(ctx->gui, &forecast);
    return TASK_RUN_AGAIN;
}

void app_gui_forecast_register_task(app_gui_bindings_ctx_t *ctx)
{
    int rc;

    if (ctx == NULL) {
        return;
    }

    ctx->forecast_task.work = forecast_work;
    rc = task_scheduler_add(&ctx->forecast_task, FORECAST_INITIAL_DELAY_MS);
    if (rc < 0) {
        ESP_LOGE(APP_GUI_BINDINGS_TAG, "Failed to add forecast fetching task to scheduler.");
    }
}
