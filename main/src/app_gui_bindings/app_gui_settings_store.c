#include "app_gui_bindings_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "rm_nvs.h"

#define GUI_NVS_KEY_THEME       "gui_theme"
#define GUI_NVS_KEY_BG          "gui_bg"
#define GUI_NVS_KEY_NIGHT       "gui_night"
#define GUI_NVS_KEY_BRIGHT      "gui_bright"
#define GUI_NVS_KEY_LAT         "gui_lat"
#define GUI_NVS_KEY_LON         "gui_lon"

static int32_t clamp_saved_brightness(int32_t value)
{
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

bool app_gui_settings_load_saved_appearance(gui_init_config_t *config)
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

void app_gui_settings_cache_current_appearance(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui)
{
    gui_appearance_settings_t appearance;
    int32_t brightness = 0;

    if ((ctx == NULL) || (gui == NULL) ||
        !gui_get_appearance_settings(gui, &appearance) ||
        !gui_get_brightness(gui, &brightness)) {
        return;
    }

    ctx->last_appearance = appearance;
    ctx->has_last_appearance = true;
    ctx->last_brightness = clamp_saved_brightness(brightness);
    ctx->has_last_brightness = true;
}

void app_gui_settings_cache_current_location(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui)
{
    gui_location_settings_t location;

    if ((ctx == NULL) || (gui == NULL) || !gui_get_location_settings(gui, &location)) {
        return;
    }

    ctx->last_location = location;
    ctx->has_last_location = true;
}

bool app_gui_settings_load_location_for_forecast(app_gui_bindings_ctx_t *ctx,
                                                double *latitude,
                                                double *longitude)
{
    gui_location_settings_t location = { 0 };
    char *end_lat = NULL;
    char *end_lon = NULL;

    if ((ctx == NULL) || (latitude == NULL) || (longitude == NULL)) {
        return false;
    }

    if (((ctx->gui == NULL) || !gui_get_location_settings(ctx->gui, &location)) &&
        ctx->has_last_location) {
        location = ctx->last_location;
    }

    if (!parse_coordinate_in_range(location.latitude, -90.0, 90.0) ||
        !parse_coordinate_in_range(location.longitude, -180.0, 180.0) ||
        (location.latitude[0] == '\0') || (location.longitude[0] == '\0')) {
        return false;
    }

    *latitude = strtod(location.latitude, &end_lat);
    *longitude = strtod(location.longitude, &end_lon);
    if ((end_lat == location.latitude) || (end_lon == location.longitude) ||
        (end_lat == NULL) || (end_lon == NULL) || (*end_lat != '\0') ||
        (*end_lon != '\0')) {
        return false;
    }

    return true;
}

bool app_gui_settings_load_saved_location(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui)
{
    gui_location_settings_t location = { 0 };
    size_t latitude_len = sizeof(location.latitude);
    size_t longitude_len = sizeof(location.longitude);
    bool loaded_any = false;

    if ((ctx == NULL) || (gui == NULL)) {
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
    app_gui_settings_cache_current_location(ctx, gui);
    return true;
}

bool app_gui_settings_save_appearance_if_changed(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui)
{
    gui_appearance_settings_t appearance;
    int32_t brightness = 0;
    bool appearance_changed;
    bool brightness_changed;
    esp_err_t err;

    if ((ctx == NULL) || (gui == NULL) ||
        !gui_get_appearance_settings(gui, &appearance) ||
        !gui_get_brightness(gui, &brightness)) {
        return false;
    }

    brightness = clamp_saved_brightness(brightness);

    appearance_changed =
        !ctx->has_last_appearance ||
        (appearance.theme != ctx->last_appearance.theme) ||
        (appearance.show_background_image != ctx->last_appearance.show_background_image) ||
        (appearance.night_variant_enabled != ctx->last_appearance.night_variant_enabled);

    brightness_changed = !ctx->has_last_brightness || (brightness != ctx->last_brightness);

    if (!appearance_changed && !brightness_changed) {
        return false;
    }

    if (appearance_changed) {
        err = rm_nvs_set_u8(GUI_NVS_KEY_THEME, (uint8_t)appearance.theme);
        if (err != ESP_OK) {
            ESP_LOGE(APP_GUI_BINDINGS_TAG, "rm_nvs_set_u8(%s) failed: %s",
                     GUI_NVS_KEY_THEME, esp_err_to_name(err));
            return false;
        }

        err = rm_nvs_set_u8(GUI_NVS_KEY_BG, appearance.show_background_image ? 1U : 0U);
        if (err != ESP_OK) {
            ESP_LOGE(APP_GUI_BINDINGS_TAG, "rm_nvs_set_u8(%s) failed: %s",
                     GUI_NVS_KEY_BG, esp_err_to_name(err));
            return false;
        }

        err = rm_nvs_set_u8(GUI_NVS_KEY_NIGHT, appearance.night_variant_enabled ? 1U : 0U);
        if (err != ESP_OK) {
            ESP_LOGE(APP_GUI_BINDINGS_TAG, "rm_nvs_set_u8(%s) failed: %s",
                     GUI_NVS_KEY_NIGHT, esp_err_to_name(err));
            return false;
        }

        ctx->last_appearance = appearance;
        ctx->has_last_appearance = true;
    }

    if (brightness_changed) {
        err = rm_nvs_set_u8(GUI_NVS_KEY_BRIGHT, (uint8_t)brightness);
        if (err != ESP_OK) {
            ESP_LOGE(APP_GUI_BINDINGS_TAG, "rm_nvs_set_u8(%s) failed: %s",
                     GUI_NVS_KEY_BRIGHT, esp_err_to_name(err));
            return false;
        }

        ctx->last_brightness = brightness;
        ctx->has_last_brightness = true;
    }

    return true;
}

bool app_gui_settings_save_location_if_changed(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui)
{
    gui_location_settings_t location;
    gui_location_settings_t fallback_location = { 0 };
    bool latitude_valid;
    bool longitude_valid;
    esp_err_t err;

    if ((ctx == NULL) || (gui == NULL) || !gui_get_location_settings(gui, &location)) {
        return false;
    }

    if (ctx->has_last_location) {
        fallback_location = ctx->last_location;
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

    if (!latitude_valid && !longitude_valid && !ctx->has_last_location) {
        return false;
    }

    if (ctx->has_last_location &&
        (strcmp(location.latitude, ctx->last_location.latitude) == 0) &&
        (strcmp(location.longitude, ctx->last_location.longitude) == 0)) {
        return false;
    }

    err = rm_nvs_set_str(GUI_NVS_KEY_LAT, location.latitude);
    if (err != ESP_OK) {
        ESP_LOGE(APP_GUI_BINDINGS_TAG, "rm_nvs_set_str(%s) failed: %s",
                 GUI_NVS_KEY_LAT, esp_err_to_name(err));
        return false;
    }

    err = rm_nvs_set_str(GUI_NVS_KEY_LON, location.longitude);
    if (err != ESP_OK) {
        ESP_LOGE(APP_GUI_BINDINGS_TAG, "rm_nvs_set_str(%s) failed: %s",
                 GUI_NVS_KEY_LON, esp_err_to_name(err));
        return false;
    }

    ctx->last_location = location;
    ctx->has_last_location = true;
    return true;
}
