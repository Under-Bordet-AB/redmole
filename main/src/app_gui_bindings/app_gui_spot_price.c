/**
 * @file app_gui_spot_price.c
 * @brief Fetch, parse, and publish Swedish spot-price data to the GUI.
 */

#include "app_gui_bindings_internal.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "cJSON.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "http_client.h"
#include "nac.h"

#define SPOT_PRICE_REFRESH_DELAY_MS 60000U
#define SPOT_PRICE_INITIAL_DELAY_MS 12000U
#define SPOT_PRICE_TIME_RETRY_DELAY_MS 2000U
#define SPOT_PRICE_RESPONSE_BUF_LEN 24576U
#define SPOT_PRICE_QUARTERS_PER_HOUR 4U
#define SPOT_PRICE_VALUE_SCALE 1000.0

typedef struct {
    bool valid;
    int16_t price_milli_kr;
} spot_price_quarter_t;

static char *s_spot_price_response_buf;

static esp_err_t spot_price_ensure_response_buf(void)
{
    if (s_spot_price_response_buf != NULL) {
        return ESP_OK;
    }

    s_spot_price_response_buf = heap_caps_malloc(SPOT_PRICE_RESPONSE_BUF_LEN + 1U,
                                                 MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_spot_price_response_buf == NULL) {
        ESP_LOGE(APP_GUI_BINDINGS_TAG, "Spot-price response buffer allocation failed.");
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

static const char *spot_price_area_code(gui_spot_price_area_t area)
{
    switch (area) {
        case GUI_SPOT_PRICE_AREA_SE1:
            return "SE1";
        case GUI_SPOT_PRICE_AREA_SE2:
            return "SE2";
        case GUI_SPOT_PRICE_AREA_SE3:
            return "SE3";
        case GUI_SPOT_PRICE_AREA_SE4:
            return "SE4";
        default:
            return "SE3";
    }
}

static int16_t spot_price_scale_value(double value)
{
    double scaled_value = (value * SPOT_PRICE_VALUE_SCALE);

    if (scaled_value >= 0.0) {
        scaled_value += 0.5;
    } else {
        scaled_value -= 0.5;
    }

    if (scaled_value > (double)(INT16_MAX - 1)) {
        return INT16_MAX - 1;
    }

    if (scaled_value < (double)INT16_MIN) {
        return INT16_MIN;
    }

    return (int16_t)scaled_value;
}

static int16_t spot_price_average_milli(int32_t sum, uint8_t count)
{
    if (count == 0U) {
        return 0;
    }

    if (sum >= 0) {
        return (int16_t)((sum + ((int32_t)count / 2)) / (int32_t)count);
    }

    return (int16_t)((sum - ((int32_t)count / 2)) / (int32_t)count);
}

static void spot_price_format_value(int16_t value_milli, char *text, size_t text_len)
{
    int32_t abs_milli;
    int32_t cents;
    int32_t whole;
    int32_t fraction;

    if ((text == NULL) || (text_len == 0U)) {
        return;
    }

    abs_milli = (value_milli < 0) ? -(int32_t)value_milli : (int32_t)value_milli;
    cents = (abs_milli + 5) / 10;
    whole = cents / 100;
    fraction = cents % 100;

    snprintf(text, text_len, "%s%ld.%02ld kr/kWh", (value_milli < 0) ? "-" : "",
             (long)whole, (long)fraction);
}

static bool spot_price_parse_time_start(const char *time_text, uint8_t *hour,
                                        uint8_t *quarter_index)
{
    int parsed_hour;
    int parsed_minute;

    if ((time_text == NULL) || (hour == NULL) || (quarter_index == NULL)) {
        return false;
    }

    if ((strlen(time_text) < 16U) || (time_text[10] != 'T') ||
        (sscanf(&time_text[11], "%2d:%2d", &parsed_hour, &parsed_minute) != 2) ||
        (parsed_hour < 0) || (parsed_hour > 23) ||
        (parsed_minute < 0) || (parsed_minute > 59) ||
        ((parsed_minute % 15) != 0)) {
        return false;
    }

    *hour = (uint8_t)parsed_hour;
    *quarter_index = (uint8_t)(parsed_minute / 15);
    return *quarter_index < SPOT_PRICE_QUARTERS_PER_HOUR;
}

static bool spot_price_parse_day_response(
    const char *response,
    uint8_t day_offset,
    spot_price_quarter_t quarters[2][24][SPOT_PRICE_QUARTERS_PER_HOUR])
{
    cJSON *root;
    int item_count;
    bool hourly_format;
    bool parsed_any = false;
    int index;

    if ((response == NULL) || (quarters == NULL) || (day_offset > 1U)) {
        return false;
    }

    root = cJSON_Parse(response);
    if (!cJSON_IsArray(root)) {
        cJSON_Delete(root);
        return false;
    }

    item_count = cJSON_GetArraySize(root);
    hourly_format = item_count <= 24;

    for (index = 0; index < item_count; index++) {
        cJSON *item = cJSON_GetArrayItem(root, index);
        cJSON *price = cJSON_GetObjectItem(item, "SEK_per_kWh");
        cJSON *time_start = cJSON_GetObjectItem(item, "time_start");
        uint8_t hour;
        uint8_t quarter_index;
        int16_t scaled_price;

        if (!cJSON_IsObject(item) || !cJSON_IsNumber(price) ||
            !cJSON_IsString(time_start) ||
            !spot_price_parse_time_start(time_start->valuestring, &hour, &quarter_index)) {
            cJSON_Delete(root);
            return false;
        }

        scaled_price = spot_price_scale_value(price->valuedouble);
        if (hourly_format) {
            uint8_t fill_index;

            for (fill_index = 0; fill_index < SPOT_PRICE_QUARTERS_PER_HOUR; fill_index++) {
                quarters[day_offset][hour][fill_index].price_milli_kr = scaled_price;
                quarters[day_offset][hour][fill_index].valid = true;
            }
        } else {
            quarters[day_offset][hour][quarter_index].price_milli_kr = scaled_price;
            quarters[day_offset][hour][quarter_index].valid = true;
        }
        parsed_any = true;
    }

    cJSON_Delete(root);
    return parsed_any;
}

static bool spot_price_build_url(char *url, size_t url_len, const struct tm *date,
                                 gui_spot_price_area_t area)
{
    if ((url == NULL) || (url_len == 0U) || (date == NULL)) {
        return false;
    }

    return snprintf(url, url_len,
                    "https://www.elprisetjustnu.se/api/v1/prices/%04d/%02d-%02d_%s.json",
                    date->tm_year + 1900, date->tm_mon + 1, date->tm_mday,
                    spot_price_area_code(area)) < (int)url_len;
}

static void spot_price_build_state(
    const spot_price_quarter_t quarters[2][24][SPOT_PRICE_QUARTERS_PER_HOUR],
    const struct tm *now_time,
    gui_spot_price_area_t area,
    gui_spot_price_state_t *spot_price)
{
    bool has_min_max = false;
    int16_t min_price = 0;
    int16_t max_price = 0;
    uint8_t current_hour;
    uint8_t current_quarter;
    uint8_t point_index;

    if ((quarters == NULL) || (now_time == NULL) || (spot_price == NULL)) {
        return;
    }

    memset(spot_price, 0, sizeof(*spot_price));
    spot_price->area = area;
    spot_price->start_hour = (uint8_t)now_time->tm_hour;
    snprintf(spot_price->current_price_text, sizeof(spot_price->current_price_text), "%s",
             "-- kr/kWh");
    snprintf(spot_price->summary, sizeof(spot_price->summary), "%s",
             "Spot price excluding VAT and taxes.");
    app_gui_time_format_unknown_last_updated(spot_price->last_updated,
                                             sizeof(spot_price->last_updated));

    current_hour = (uint8_t)now_time->tm_hour;
    current_quarter = (uint8_t)(now_time->tm_min / 15);
    if (current_quarter >= SPOT_PRICE_QUARTERS_PER_HOUR) {
        current_quarter = SPOT_PRICE_QUARTERS_PER_HOUR - 1U;
    }

    if (quarters[0][current_hour][current_quarter].valid) {
        spot_price->current_price_milli_kr =
            quarters[0][current_hour][current_quarter].price_milli_kr;
        spot_price->has_current_price = true;
        spot_price_format_value(spot_price->current_price_milli_kr,
                                spot_price->current_price_text,
                                sizeof(spot_price->current_price_text));
    }

    for (point_index = 0; point_index < GUI_SPOT_PRICE_POINT_COUNT; point_index++) {
        uint8_t absolute_hour = (uint8_t)(current_hour + point_index);
        uint8_t day_offset = (uint8_t)(absolute_hour / 24U);
        uint8_t hour = (uint8_t)(absolute_hour % 24U);
        uint8_t quarter_index;
        int32_t sum = 0;
        uint8_t count = 0;
        int16_t average;

        if (day_offset > 1U) {
            continue;
        }

        for (quarter_index = 0; quarter_index < SPOT_PRICE_QUARTERS_PER_HOUR;
             quarter_index++) {
            if (quarters[day_offset][hour][quarter_index].valid) {
                sum += quarters[day_offset][hour][quarter_index].price_milli_kr;
                count++;
            }
        }

        if (count == 0U) {
            continue;
        }

        average = spot_price_average_milli(sum, count);
        spot_price->price_milli_kr[point_index] = average;
        spot_price->valid_points[point_index] = true;

        if (!has_min_max) {
            min_price = average;
            max_price = average;
            has_min_max = true;
        } else {
            if (average < min_price) {
                min_price = average;
            }
            if (average > max_price) {
                max_price = average;
            }
        }
    }

    if (has_min_max) {
        char min_text[GUI_SPOT_PRICE_VALUE_TEXT_MAX_LEN];
        char max_text[GUI_SPOT_PRICE_VALUE_TEXT_MAX_LEN];

        spot_price_format_value(min_price, min_text, sizeof(min_text));
        spot_price_format_value(max_price, max_text, sizeof(max_text));
        snprintf(spot_price->summary, sizeof(spot_price->summary), "min %s\nmax %s",
                 min_text, max_text);
    }
}

static task_status_t spot_price_work(task_node_t *node)
{
    app_gui_bindings_ctx_t *ctx;
    gui_spot_price_state_t spot_price;
    spot_price_quarter_t quarters[2][24][SPOT_PRICE_QUARTERS_PER_HOUR] = { 0 };
    char url[160];
    time_t now;
    time_t tomorrow;
    struct tm today_time;
    struct tm tomorrow_time;
    esp_err_t http_rc;

    if (node == NULL) {
        return TASK_ERROR;
    }

    if (nac_get_wifi_status() != NAC_WIFI_CONNECTED) {
        node->run_at_tick = xTaskGetTickCount() + pdMS_TO_TICKS(SPOT_PRICE_REFRESH_DELAY_MS);
        ESP_LOGI(APP_GUI_BINDINGS_TAG, "Waiting for Wi-Fi before fetching spot prices...");
        return TASK_RUN_AGAIN;
    }

    ctx = container_of(node, app_gui_bindings_ctx_t, spot_price_task);
    node->run_at_tick = xTaskGetTickCount() + pdMS_TO_TICKS(SPOT_PRICE_REFRESH_DELAY_MS);

    if ((ctx == NULL) || (ctx->gui == NULL)) {
        ESP_LOGW(APP_GUI_BINDINGS_TAG, "Skipping spot-price refresh without GUI context.");
        return TASK_RUN_AGAIN;
    }

    if (!gui_get_spot_price_state(ctx->gui, &spot_price)) {
        ESP_LOGW(APP_GUI_BINDINGS_TAG, "Failed to load current spot-price GUI state.");
        return TASK_RUN_AGAIN;
    }

    now = time(NULL);
    if ((now < 1700000000) || (localtime_r(&now, &today_time) == NULL)) {
        node->run_at_tick =
            xTaskGetTickCount() + pdMS_TO_TICKS(SPOT_PRICE_TIME_RETRY_DELAY_MS);
        ESP_LOGI(APP_GUI_BINDINGS_TAG, "Waiting for local time before fetching spot prices...");
        return TASK_RUN_AGAIN;
    }

    tomorrow = now + (24 * 60 * 60);
    if (localtime_r(&tomorrow, &tomorrow_time) == NULL) {
        ESP_LOGW(APP_GUI_BINDINGS_TAG, "Failed to build tomorrow date for spot prices.");
        return TASK_RUN_AGAIN;
    }

    ESP_LOGI(APP_GUI_BINDINGS_TAG, "Fetching spot prices for %s...",
             spot_price_area_code(spot_price.area));

    if (!spot_price_build_url(url, sizeof(url), &today_time, spot_price.area)) {
        ESP_LOGW(APP_GUI_BINDINGS_TAG, "Failed to build spot-price URL.");
        return TASK_RUN_AGAIN;
    }

    http_rc = spot_price_ensure_response_buf();
    if (http_rc != ESP_OK) {
        ESP_LOGW(APP_GUI_BINDINGS_TAG, "Spot-price request failed: %s",
                 esp_err_to_name(http_rc));
        return TASK_RUN_AGAIN;
    }

    s_spot_price_response_buf[0] = '\0';
    http_rc = http_client_get(url, s_spot_price_response_buf,
                              SPOT_PRICE_RESPONSE_BUF_LEN + 1U);
    if (http_rc != ESP_OK) {
        ESP_LOGW(APP_GUI_BINDINGS_TAG, "Spot-price request failed: %s",
                 esp_err_to_name(http_rc));
        return TASK_RUN_AGAIN;
    }

    if (!spot_price_parse_day_response(s_spot_price_response_buf, 0U, quarters)) {
        ESP_LOGW(APP_GUI_BINDINGS_TAG, "Failed to parse today's spot-price response.");
        return TASK_RUN_AGAIN;
    }

    if (spot_price_build_url(url, sizeof(url), &tomorrow_time, spot_price.area)) {
        s_spot_price_response_buf[0] = '\0';
        http_rc = http_client_get(url, s_spot_price_response_buf,
                                  SPOT_PRICE_RESPONSE_BUF_LEN + 1U);
        if (http_rc == ESP_OK) {
            if (!spot_price_parse_day_response(s_spot_price_response_buf, 1U, quarters)) {
                ESP_LOGW(APP_GUI_BINDINGS_TAG,
                         "Failed to parse tomorrow's spot-price response.");
            }
        } else {
            ESP_LOGW(APP_GUI_BINDINGS_TAG, "Tomorrow's spot prices are not available yet.");
        }
    }

    spot_price_build_state(quarters, &today_time, spot_price.area, &spot_price);
    app_gui_time_format_last_updated_now(spot_price.last_updated,
                                         sizeof(spot_price.last_updated));
    gui_set_spot_price_state(ctx->gui, &spot_price);

    return TASK_RUN_AGAIN;
}

void app_gui_spot_price_schedule_now(app_gui_bindings_ctx_t *ctx)
{
    if ((ctx == NULL) || !ctx->spot_price_task.active) {
        return;
    }

    ctx->spot_price_task.run_at_tick = xTaskGetTickCount();
}

void app_gui_spot_price_register_task(app_gui_bindings_ctx_t *ctx)
{
    int rc;

    if (ctx == NULL) {
        return;
    }

    ctx->spot_price_task.work = spot_price_work;
    rc = task_scheduler_add(&ctx->spot_price_task, SPOT_PRICE_INITIAL_DELAY_MS);
    if (rc < 0) {
        ESP_LOGE(APP_GUI_BINDINGS_TAG, "Failed to add spot-price task to scheduler.");
    }
}
