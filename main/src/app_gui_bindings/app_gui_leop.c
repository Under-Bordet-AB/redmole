#include "app_gui_bindings_internal.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cJSON.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "http_client.h"
#include "sdkconfig.h"

#define LEOP_REFRESH_URL CONFIG_REDMOLE_LEOP_REFRESH_URL
#define LEOP_RAW_POINT_COUNT 96
#define LEOP_POINTS_PER_HOUR 4
#define LEOP_REFRESH_DELAY_MS 60000U
#define LEOP_INITIAL_DELAY_MS 10000U
/*
 * Scale LEOP normalized hourly averages into integer chart values using
 * milli-units so the chart can render one decimal place on its 0.0-1.0 axis.
 */
#define LEOP_VALUE_SCALE 1000.0

static uint16_t leop_scale_hourly_average(double hourly_average)
{
    double scaled_value;

    if (hourly_average <= 0.0) {
        return 0U;
    }

    scaled_value = (hourly_average * LEOP_VALUE_SCALE) + 0.5;
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
        ESP_LOGW(APP_GUI_BINDINGS_TAG, "LEOP parse error: %s is not an array", series_name);
        return false;
    }

    if (cJSON_GetArraySize(series_array) < LEOP_RAW_POINT_COUNT) {
        ESP_LOGW(APP_GUI_BINDINGS_TAG, "LEOP parse error: %s shorter than %d points", series_name,
                 LEOP_RAW_POINT_COUNT);
        return false;
    }

    for (hour_index = 0; hour_index < GUI_ENERGY_PLAN_POINT_COUNT; hour_index++) {
        double hourly_sum = 0.0;
        int quarter_index;

        for (quarter_index = 0; quarter_index < LEOP_POINTS_PER_HOUR; quarter_index++) {
            const cJSON *sample = cJSON_GetArrayItem(series_array,
                                                     (hour_index * LEOP_POINTS_PER_HOUR) +
                                                     quarter_index);

            if (cJSON_IsNull(sample)) {
                continue;
            }

            if (!cJSON_IsNumber(sample)) {
                ESP_LOGW(APP_GUI_BINDINGS_TAG, "LEOP parse error: %s[%d] is not numeric",
                         series_name, (hour_index * LEOP_POINTS_PER_HOUR) + quarter_index);
                return false;
            }

            hourly_sum += sample->valuedouble;
        }

        values[hour_index] =
            leop_scale_hourly_average(hourly_sum / (double)LEOP_POINTS_PER_HOUR);
    }

    return true;
}

static bool leop_parse_timestamp_hour(const cJSON *timestamp_item, uint8_t *hour)
{
    int parsed_hour;
    struct tm time_info;
    time_t timestamp_s;
    const char *time_text;

    if ((timestamp_item == NULL) || (hour == NULL)) {
        return false;
    }

    if (cJSON_IsNumber(timestamp_item)) {
        if (timestamp_item->valuedouble < 0.0) {
            return false;
        }

        timestamp_s = (time_t)timestamp_item->valuedouble;
        if (localtime_r(&timestamp_s, &time_info) == NULL) {
            return false;
        }

        *hour = (uint8_t)time_info.tm_hour;
        return true;
    }

    if (!cJSON_IsString(timestamp_item)) {
        return false;
    }

    time_text = timestamp_item->valuestring;
    if ((strlen(time_text) < 13U) || ((time_text[10] != 'T') && (time_text[10] != ' ')) ||
        (time_text[11] < '0') || (time_text[11] > '9') || (time_text[12] < '0') ||
        (time_text[12] > '9')) {
        return false;
    }

    parsed_hour = ((time_text[11] - '0') * 10) + (time_text[12] - '0');
    if (parsed_hour > 23) {
        return false;
    }

    *hour = (uint8_t)parsed_hour;
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
    cJSON *first_timestamp;
    uint8_t start_hour;

#define LEOP_PARSE_FAIL(fmt, ...)                                      \
    do {                                                               \
        ESP_LOGW(APP_GUI_BINDINGS_TAG, "LEOP parse error: " fmt, ##__VA_ARGS__); \
        cJSON_Delete(root);                                            \
        return false;                                                  \
    } while (0)

    if ((response == NULL) || (energy_plan == NULL)) {
        return false;
    }

    root = cJSON_Parse(response);
    if (root == NULL) {
        ESP_LOGW(APP_GUI_BINDINGS_TAG, "LEOP parse error: invalid JSON payload");
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

    first_timestamp = cJSON_GetArrayItem(timestamp, 0);
    if (!leop_parse_timestamp_hour(first_timestamp, &start_hour)) {
        LEOP_PARSE_FAIL("timestamp[0] does not contain a valid hour");
    }

    memset(energy_plan, 0, sizeof(*energy_plan));
    energy_plan->start_hour = start_hour;

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

static task_status_t leop_work(task_node_t *node)
{
    app_gui_bindings_ctx_t *ctx;
    enum { RESPONSE_BUF_LEN = 8192 };
    char *buf;
    gui_energy_plan_t energy_plan;
    esp_err_t http_rc;

    ESP_LOGI(APP_GUI_BINDINGS_TAG, "Fetching LEOP...");

    if (node == NULL) {
        return TASK_ERROR;
    }

    ctx = container_of(node, app_gui_bindings_ctx_t, leop_task);
    node->run_at_tick = xTaskGetTickCount() + pdMS_TO_TICKS(LEOP_REFRESH_DELAY_MS);

    if (ctx->gui == NULL) {
        ESP_LOGW(APP_GUI_BINDINGS_TAG, "Skipping LEOP refresh without GUI context.");
        return TASK_RUN_AGAIN;
    }

    buf = malloc(RESPONSE_BUF_LEN + 1U);

    if (buf == NULL) {
        ESP_LOGE(APP_GUI_BINDINGS_TAG, "Failed to allocate LEOP response buffer");
        return TASK_RUN_AGAIN;
    }

    buf[0] = '\0';
    http_rc = http_client_get(LEOP_REFRESH_URL, buf, RESPONSE_BUF_LEN + 1U);
    if (http_rc != ESP_OK) {
        ESP_LOGW(APP_GUI_BINDINGS_TAG, "LEOP request failed: %s", esp_err_to_name(http_rc));
        free(buf);
        return TASK_RUN_AGAIN;
    }

    if (!gui_get_energy_plan_state(ctx->gui, &energy_plan)) {
        ESP_LOGW(APP_GUI_BINDINGS_TAG, "Failed to load current LEOP GUI state.");
        free(buf);
        return TASK_RUN_AGAIN;
    }

    if (!leop_parse_response(buf, &energy_plan)) {
        ESP_LOGW(APP_GUI_BINDINGS_TAG, "Failed to parse LEOP response.");
        free(buf);
        return TASK_RUN_AGAIN;
    }

    app_gui_time_format_last_updated_now(energy_plan.last_updated, sizeof(energy_plan.last_updated));
    gui_set_energy_plan_state(ctx->gui, &energy_plan);
    free(buf);
    return TASK_RUN_AGAIN;
}

void app_gui_leop_register_task(app_gui_bindings_ctx_t *ctx)
{
    int rc;

    if (ctx == NULL) {
        return;
    }

    ctx->leop_task.work = leop_work;
    rc = task_scheduler_add(&ctx->leop_task, LEOP_INITIAL_DELAY_MS);
    if (rc < 0) {
        ESP_LOGE(APP_GUI_BINDINGS_TAG, "Failed to add LEOP fetching task to scheduler.");
    }
}
