/**
 * @file app_gui_sync.c
 * @brief Synchronize runtime application state into the GUI model.
 *
 * Updates sensor values, Wi-Fi status, SD card state, and the UART GUI-online
 * event bit, and registers the periodic sensor scheduler node.
 */

#include "app_gui_bindings_internal.h"

#include "esp_log.h"
#include "environment_measurements.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "sdcard.h"
#include "uart_mole.h"

#define SENSOR_REFRESH_DELAY_MS 60000U
#define SENSOR_INITIAL_DELAY_MS 2500U

static void sync_sensor(gui_ctx_t *gui)
{
    environment_measurement_sample_t sample = { 0 };
    gui_sensor_state_t sensor = { 0 };
    gui_sensor_state_t current_sensor = { 0 };
    bool has_current_sensor;
    uint32_t update_count;

    if (gui == NULL) {
        return;
    }

    has_current_sensor = gui_get_sensor_state(gui, &current_sensor);
    if (has_current_sensor) {
        sensor = current_sensor;
    }

    if (sensor.last_updated[0] == '\0') {
        app_gui_time_format_unknown_last_updated(sensor.last_updated, sizeof(sensor.last_updated));
    }

    if (environment_measurements_get_latest(&sample) && sample.valid) {
        update_count = environment_measurements_get_update_count();
        sensor.temperature_deci_c = sample.temperature_deci_c;
        sensor.humidity_deci_pct = sample.humidity_deci_pct;
        sensor.pressure_deci_hpa = sample.pressure_deci_hpa;
        sensor.is_fresh = environment_measurements_is_fresh(3000U);
        sensor.update_count = update_count;
        if (!has_current_sensor || (current_sensor.update_count != update_count)) {
            app_gui_time_format_last_updated_now(sensor.last_updated, sizeof(sensor.last_updated));
        }
    } else {
        sensor.is_fresh = false;
    }

    gui_set_sensor_state(gui, &sensor);
}

static gui_sd_card_state_t get_sd_card_state(void)
{
    return sdcard_is_initialized() ? GUI_SD_CARD_STATE_CONNECTED
                                   : GUI_SD_CARD_STATE_UNAVAILABLE;
}

static void sync_gui_online_bit(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui)
{
    if ((ctx == NULL) || (ctx->event_group == NULL) || (*ctx->event_group == NULL)) {
        return;
    }

    if (gui_is_active(gui)) {
        xEventGroupSetBits(*ctx->event_group, UART_MOLE_GUI_ONLINE_BIT);
    } else {
        xEventGroupClearBits(*ctx->event_group, UART_MOLE_GUI_ONLINE_BIT);
    }
}

static bool sync_sd_card_state(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui)
{
    gui_sd_card_state_t sd_card_state;

    if ((ctx == NULL) || (gui == NULL)) {
        return false;
    }

    sd_card_state = get_sd_card_state();
    if (ctx->has_last_sd_card_state &&
        (ctx->last_sd_card_state == sd_card_state)) {
        return false;
    }

    gui_set_sd_card_state(gui, sd_card_state);
    ctx->last_sd_card_state = sd_card_state;
    ctx->has_last_sd_card_state = true;
    return true;
}

void app_gui_sync_runtime(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui)
{
    bool wifi_state_changed;

    sync_gui_online_bit(ctx, gui);

    if ((ctx == NULL) || (gui == NULL)) {
        return;
    }

    wifi_state_changed = app_gui_wifi_sync_state(ctx, gui);
    if (wifi_state_changed || ctx->wifi_scan_requested ||
        ctx->wifi_connect_requested || ctx->wifi_disconnect_requested) {
        app_gui_wifi_sync(ctx, gui);
    }

    if (wifi_state_changed && (ctx->last_wifi_state == GUI_WIFI_STATE_CONNECTED)) {
        app_gui_forecast_schedule_now(ctx);
        app_gui_leop_schedule_now(ctx);
        app_gui_spot_price_schedule_now(ctx);
    }

    if (ctx->location_changed) {
        app_gui_forecast_schedule_now(ctx);
        ctx->location_changed = false;
    }

    if (ctx->spot_price_area_changed) {
        app_gui_spot_price_schedule_now(ctx);
        ctx->spot_price_area_changed = false;
    }

    sync_sensor(gui);

    (void)sync_sd_card_state(ctx, gui);
}

static task_status_t sensor_work(task_node_t *node)
{
    ESP_LOGI(APP_GUI_BINDINGS_TAG, "Fetching sensor...");

    if (node == NULL) {
        return TASK_ERROR;
    }

    node->run_at_tick = xTaskGetTickCount() + pdMS_TO_TICKS(SENSOR_REFRESH_DELAY_MS);
    return TASK_RUN_AGAIN;
}

void app_gui_sync_register_sensor_task(app_gui_bindings_ctx_t *ctx)
{
    int rc;

    if (ctx == NULL) {
        return;
    }

    ctx->sensor_task.work = sensor_work;
    rc = task_scheduler_add(&ctx->sensor_task, SENSOR_INITIAL_DELAY_MS);
    if (rc < 0) {
        ESP_LOGE(APP_GUI_BINDINGS_TAG, "Failed to add sensor fetching task to scheduler.");
    }
}
