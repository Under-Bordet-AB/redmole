#include "app_gui_bindings.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "nac.h"
#include "sensor_data.h"

static const char *TAG = "APP_GUI_BINDINGS";

typedef struct {
    gui_ctx_t *gui;
    bool wifi_connect_requested;
    bool wifi_scan_requested;
} app_gui_bindings_ctx_t;

static app_gui_bindings_ctx_t s_bindings;

static bool on_wifi_connect_requested(gui_ctx_t *gui, const char *ssid, const char *password,
                                      void *user_data);

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
        case NAC_WIFI_CONNECTING:
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
        size_t ssid_len = strnlen((const char *)records[index].ssid,
                                  sizeof(wifi->networks[index].ssid) - 1U);

        memcpy(wifi->networks[index].ssid, records[index].ssid, ssid_len);
        wifi->networks[index].ssid[ssid_len] = '\0';
        wifi->networks[index].signal_strength_pct =
            signal_strength_pct(records[index].rssi);
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

static void sync_wifi(gui_ctx_t *gui)
{
    gui_wifi_settings_t wifi = { 0 };
    nac_wifi_status_t nac_status;

    if ((gui == NULL) || !gui_get_wifi_settings(gui, &wifi)) {
        return;
    }

    nac_status = nac_get_wifi_status();
    wifi.state = map_wifi_state(nac_status);

    if (nac_status == NAC_WIFI_SCANNING) {
        snprintf(wifi.status_text, sizeof(wifi.status_text), "%s",
                 "Scanning for Wi-Fi networks...");
    } else if (s_bindings.wifi_scan_requested && nac_scan_is_complete()) {
        copy_scan_results(&wifi);
        if (wifi.network_count > 0U) {
            wifi.state = GUI_WIFI_STATE_SCANNED;
            snprintf(wifi.status_text, sizeof(wifi.status_text), "%s",
                     "Scan complete. Select a network.");
        } else {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "%s",
                     "Scan complete. No Wi-Fi networks found.");
        }
    } else if (nac_status == NAC_WIFI_CONNECTING) {
        snprintf(wifi.status_text, sizeof(wifi.status_text), "%s", "Connecting to Wi-Fi...");
    } else if (nac_status == NAC_WIFI_CONNECTED) {
        s_bindings.wifi_connect_requested = false;
        snprintf(wifi.status_text, sizeof(wifi.status_text), "%s", "Wi-Fi connected.");
    } else if (nac_status == NAC_WIFI_ERROR) {
        s_bindings.wifi_connect_requested = false;
        snprintf(wifi.status_text, sizeof(wifi.status_text), "%s", "Wi-Fi error.");
    } else if (s_bindings.wifi_connect_requested) {
        snprintf(wifi.status_text, sizeof(wifi.status_text), "%s",
                 "Connection requested. NAC uses configured Wi-Fi credentials.");
    }

    gui_set_wifi_settings(gui, &wifi);
}

static void on_panel_changed(gui_ctx_t *gui, gui_panel_id_t panel, void *user_data)
{
    (void)gui;
    (void)user_data;
    ESP_LOGI(TAG, "GUI panel changed to %d", (int)panel);
}

static bool on_wifi_scan_requested(gui_ctx_t *gui, void *user_data)
{
    esp_err_t result;

    (void)user_data;
    result = nac_request_wifi_scan();
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "nac_request_wifi_scan failed: %s", esp_err_to_name(result));
        set_wifi_status(gui, "Failed to start Wi-Fi scan.", GUI_WIFI_STATE_FAILED);
        return true;
    }

    s_bindings.wifi_scan_requested = true;
    set_wifi_status(gui, "Scanning for Wi-Fi networks...", GUI_WIFI_STATE_IDLE);
    return true;
}

static void on_wifi_network_selected(gui_ctx_t *gui, const gui_wifi_network_t *network,
                                     void *user_data)
{
    (void)gui;
    (void)user_data;

    if (network == NULL) {
        return;
    }

    ESP_LOGI(TAG, "GUI selected Wi-Fi network: %s", network->ssid);
}

static bool on_wifi_known_network_requested(gui_ctx_t *gui, const gui_wifi_network_t *network,
                                            void *user_data)
{
    const char *ssid = NULL;

    if (network != NULL) {
        ssid = network->ssid;
    }

    return on_wifi_connect_requested(gui, ssid, NULL, user_data);
}

static bool on_wifi_connect_requested(gui_ctx_t *gui, const char *ssid, const char *password,
                                      void *user_data)
{
    esp_err_t result;

    //(void)password;
    //(void)user_data;

    result = nac_request_wifi_connect(ssid, password);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "nac_request_wifi_connect failed: %s", esp_err_to_name(result));
        set_wifi_status(gui, "Failed to request Wi-Fi connection.", GUI_WIFI_STATE_FAILED);
        return true;
    }

    s_bindings.wifi_connect_requested = true;
    ESP_LOGI(TAG, "GUI requested Wi-Fi connection for %s", (ssid != NULL) ? ssid : "<none>");
    set_wifi_status(gui, "Connection requested. NAC uses configured Wi-Fi credentials.",
                    GUI_WIFI_STATE_IDLE);
    return true;
}

esp_err_t app_gui_bindings_init(gui_ctx_t *gui)
{
    gui_module_bindings_t bindings = { 0 };

    if (gui == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(&s_bindings, 0, sizeof(s_bindings));
    s_bindings.gui = gui;

    bindings.user_data = &s_bindings;
    bindings.on_panel_changed = on_panel_changed;
    bindings.on_wifi_scan_requested = on_wifi_scan_requested;
    bindings.on_wifi_network_selected = on_wifi_network_selected;
    bindings.on_wifi_known_network_requested = on_wifi_known_network_requested;
    bindings.on_wifi_connect_requested = on_wifi_connect_requested;
    gui_set_bindings(gui, &bindings);

    app_gui_bindings_sync(gui);
    return ESP_OK;
}

void app_gui_bindings_sync(gui_ctx_t *gui)
{
    if (gui == NULL) {
        return;
    }

    sync_sensor(gui);
    sync_wifi(gui);
}
