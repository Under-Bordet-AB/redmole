#include "app_gui_bindings_internal.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "nac.h"
#include "rm_nvs.h"

#define GUI_NVS_KEY_WIFI_SSID "wifi_ssid"

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
        size_t ssid_len = strnlen((const char *)records[index].ssid,
                                  sizeof(wifi->networks[index].ssid) - 1U);

        memcpy(wifi->networks[index].ssid, records[index].ssid, ssid_len);
        wifi->networks[index].ssid[ssid_len] = '\0';
        wifi->networks[index].signal_strength_pct = signal_strength_pct(records[index].rssi);
        wifi->networks[index].secured = records[index].authmode != WIFI_AUTH_OPEN;
    }
    wifi->network_count = count;
}

bool app_gui_wifi_sync_state(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui)
{
    gui_wifi_state_t wifi_state;

    if ((ctx == NULL) || (gui == NULL)) {
        return false;
    }

    wifi_state = map_wifi_state(nac_get_wifi_status());
    if (ctx->has_last_wifi_state && (ctx->last_wifi_state == wifi_state)) {
        return false;
    }

    gui_set_wifi_state(gui, wifi_state);
    ctx->last_wifi_state = wifi_state;
    ctx->has_last_wifi_state = true;
    return true;
}

void app_gui_wifi_sync(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui)
{
    gui_wifi_settings_t wifi = { 0 };
    nac_wifi_status_t nac_status;

    if ((ctx == NULL) || (gui == NULL) || !gui_get_wifi_settings(gui, &wifi)) {
        return;
    }

    nac_status = nac_get_wifi_status();
    wifi.state = map_wifi_state(nac_status);
    wifi.connect_requested = ctx->wifi_connect_requested;
    wifi.can_disconnect = nac_status == NAC_WIFI_CONNECTED;

    if ((wifi.selected_ssid[0] == '\0') && (ctx->requested_ssid[0] != '\0')) {
        snprintf(wifi.selected_ssid, sizeof(wifi.selected_ssid), "%s",
                 ctx->requested_ssid);
    }

    if (nac_status == NAC_WIFI_SCANNING) {
        snprintf(wifi.status_text, sizeof(wifi.status_text), "%s", "Scanning for Wi-Fi networks...");
    } else if (ctx->wifi_scan_requested && nac_scan_is_complete()) {
        ctx->wifi_scan_requested = false;
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
        ctx->wifi_connect_requested = false;
        ctx->wifi_disconnect_requested = false;
        wifi.connect_requested = false;
        wifi.can_disconnect = true;
        if (wifi.selected_ssid[0] != '\0') {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "Connected to %s.", wifi.selected_ssid);
        } else {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "%s", "Wi-Fi connected.");
        }
        gui_set_wifi_settings(gui, &wifi);
        (void)app_gui_wifi_load_saved_metadata(ctx, gui);
        gui_hide_wifi_dialogs(gui);
        return;
    } else if (nac_status == NAC_WIFI_ERROR) {
        ctx->wifi_connect_requested = false;
        ctx->wifi_disconnect_requested = false;
        wifi.connect_requested = false;
        wifi.can_disconnect = false;
        if (wifi.selected_ssid[0] != '\0') {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "Failed to connect to %s.", wifi.selected_ssid);
        } else {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "%s", "Wi-Fi connection failed.");
        }
    } else if (ctx->wifi_connect_requested &&
               nac_status == NAC_WIFI_DISCONNECTED) {
        ctx->wifi_connect_requested = false;
        wifi.connect_requested = false;
        wifi.can_disconnect = false;
        wifi.state = GUI_WIFI_STATE_IDLE;
        snprintf(wifi.status_text, sizeof(wifi.status_text), "%s", "Wi-Fi disconnected.");
    } else if (ctx->wifi_connect_requested) {
        wifi.state = GUI_WIFI_STATE_CONNECTING;
        wifi.connect_requested = true;
        if (wifi.selected_ssid[0] != '\0') {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "Connecting to %s...", wifi.selected_ssid);
        } else {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "%s", "Connecting to Wi-Fi...");
        }
    } else if (ctx->wifi_disconnect_requested) {
        ctx->wifi_disconnect_requested = false;
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

bool app_gui_wifi_load_saved_metadata(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui)
{
    gui_wifi_settings_t wifi = { 0 };
    char saved_ssid[GUI_WIFI_SSID_MAX_LEN] = { 0 };
    size_t saved_ssid_len = sizeof(saved_ssid);

    if ((ctx == NULL) || (gui == NULL) || !gui_get_wifi_settings(gui, &wifi)) {
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

    snprintf(ctx->requested_ssid, sizeof(ctx->requested_ssid), "%s", saved_ssid);
    gui_set_wifi_settings(gui, &wifi);
    return true;
}

bool app_gui_wifi_queue_saved_autoconnect(app_gui_bindings_ctx_t *ctx, gui_ctx_t *gui)
{
    char status_text[GUI_WIFI_STATUS_TEXT_MAX_LEN];
    esp_err_t result;

    if ((ctx == NULL) || (gui == NULL) || ctx->boot_autoconnect_queued) {
        return false;
    }

    if (!app_gui_wifi_load_saved_metadata(ctx, gui)) {
        return false;
    }

    ctx->wifi_connect_requested = true;
    ctx->wifi_disconnect_requested = false;

    if (ctx->requested_ssid[0] != '\0') {
        snprintf(status_text, sizeof(status_text), "Connecting to %s...", ctx->requested_ssid);
        set_wifi_status(gui, status_text, GUI_WIFI_STATE_CONNECTING);
    } else {
        set_wifi_status(gui, "Connecting to Wi-Fi...", GUI_WIFI_STATE_CONNECTING);
    }

    result = nac_request_wifi_connect(NULL, NULL);
    if (result != ESP_OK) {
        ESP_LOGE(APP_GUI_BINDINGS_TAG, "nac_request_wifi_connect(NULL, NULL) failed: %s", esp_err_to_name(result));
        ctx->wifi_connect_requested = false;
        set_wifi_status(gui, "Failed to request Wi-Fi connection.", GUI_WIFI_STATE_FAILED);
        return false;
    }

    ctx->boot_autoconnect_queued = true;
    return true;
}

static void on_panel_changed(gui_ctx_t *gui, gui_panel_id_t panel, void *user_data)
{
    (void)gui;
    (void)user_data;
    ESP_LOGI(APP_GUI_BINDINGS_TAG, "GUI panel changed to %d", (int)panel);
}

static bool on_wifi_scan_requested(gui_ctx_t *gui, void *user_data)
{
    app_gui_bindings_ctx_t *ctx = (app_gui_bindings_ctx_t *)user_data;
    esp_err_t result;
    gui_wifi_settings_t wifi = { 0 };

    if (ctx == NULL) {
        return false;
    }

    ctx->wifi_connect_requested = false;
    ctx->wifi_disconnect_requested = false;
    ctx->requested_ssid[0] = '\0';

    if ((gui != NULL) && gui_get_wifi_settings(gui, &wifi)) {
        wifi.connect_requested = false;
        wifi.can_disconnect = false;
        wifi.selected_ssid[0] = '\0';
        gui_set_wifi_settings(gui, &wifi);
    }

    result = nac_request_wifi_scan();
    if (result != ESP_OK) {
        ESP_LOGE(APP_GUI_BINDINGS_TAG, "nac_request_wifi_scan failed: %s", esp_err_to_name(result));
        set_wifi_status(gui, "Failed to start Wi-Fi scan.", GUI_WIFI_STATE_FAILED);
        return true;
    }

    ctx->wifi_scan_requested = true;
    set_wifi_status(gui, "Scanning for Wi-Fi networks...", GUI_WIFI_STATE_SCANNED);
    return true;
}

static void on_wifi_network_selected(gui_ctx_t *gui,
                                     const gui_wifi_network_t *network,
                                     void *user_data)
{
    (void)gui;
    (void)user_data;

    if (network == NULL) {
        return;
    }

    ESP_LOGI(APP_GUI_BINDINGS_TAG, "GUI selected Wi-Fi network: %s", network->ssid);
}

static bool on_wifi_connect_requested(gui_ctx_t *gui,
                                      const char *ssid,
                                      const char *password,
                                      void *user_data);

static bool on_wifi_known_network_requested(gui_ctx_t *gui,
                                            const gui_wifi_network_t *network,
                                            void *user_data)
{
    const char *ssid = NULL;

    if (network != NULL) {
        ssid = network->ssid;
    }

    return on_wifi_connect_requested(gui, ssid, NULL, user_data);
}

static bool on_wifi_connect_requested(gui_ctx_t *gui,
                                      const char *ssid,
                                      const char *password,
                                      void *user_data)
{
    app_gui_bindings_ctx_t *ctx = (app_gui_bindings_ctx_t *)user_data;
    esp_err_t result;

    if (ctx == NULL) {
        return false;
    }

    result = nac_request_wifi_connect(ssid, password);
    if (result != ESP_OK) {
        ESP_LOGE(APP_GUI_BINDINGS_TAG, "nac_request_wifi_connect failed: %s", esp_err_to_name(result));
        set_wifi_status(gui, "Failed to request Wi-Fi connection.", GUI_WIFI_STATE_FAILED);
        return true;
    }

    ctx->wifi_connect_requested = true;
    ctx->wifi_disconnect_requested = false;
    if (ssid != NULL) {
        snprintf(ctx->requested_ssid, sizeof(ctx->requested_ssid), "%s", ssid);
    } else {
        ctx->requested_ssid[0] = '\0';
    }
    ESP_LOGI(APP_GUI_BINDINGS_TAG, "GUI requested Wi-Fi connection for %s", (ssid != NULL) ? ssid : "<none>");
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
    app_gui_bindings_ctx_t *ctx = (app_gui_bindings_ctx_t *)user_data;
    esp_err_t result;

    if (ctx == NULL) {
        return false;
    }

    result = nac_request_wifi_disconnect();
    if (result != ESP_OK) {
        ESP_LOGE(APP_GUI_BINDINGS_TAG, "nac_request_wifi_disconnect failed: %s", esp_err_to_name(result));
        set_wifi_status(gui, "Failed to disconnect Wi-Fi.", GUI_WIFI_STATE_FAILED);
        return true;
    }

    ctx->wifi_connect_requested = false;
    ctx->wifi_disconnect_requested = true;
    ESP_LOGI(APP_GUI_BINDINGS_TAG, "GUI requested Wi-Fi disconnect");
    set_wifi_status(gui, "Disconnected.", GUI_WIFI_STATE_IDLE);
    return true;
}

void app_gui_wifi_fill_bindings(gui_module_bindings_t *bindings,
                                app_gui_bindings_ctx_t *ctx)
{
    if (bindings == NULL) {
        return;
    }

    memset(bindings, 0, sizeof(*bindings));
    bindings->user_data = ctx;
    bindings->on_panel_changed = on_panel_changed;
    bindings->on_wifi_scan_requested = on_wifi_scan_requested;
    bindings->on_wifi_network_selected = on_wifi_network_selected;
    bindings->on_wifi_known_network_requested = on_wifi_known_network_requested;
    bindings->on_wifi_connect_requested = on_wifi_connect_requested;
    bindings->on_wifi_disconnect_requested = on_wifi_disconnect_requested;
}
