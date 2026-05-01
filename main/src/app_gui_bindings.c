/**
 * @file app_gui_bindings.c
 * @brief Synchronize application services, persisted settings, and GUI events.
 *
 * This module loads persisted GUI startup preferences, mirrors runtime
 * application state into the GUI model, persists appearance changes, and
 * forwards GUI-originated Wi-Fi actions to the network control layer.
 */

#include "app_gui_bindings.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "nac.h"
#include "rm_nvs.h"
#include "sensor_data.h"

static const char *TAG = "APP_GUI_BINDINGS";

#define GUI_NVS_KEY_THEME       "gui_theme"
#define GUI_NVS_KEY_BG          "gui_bg"
#define GUI_NVS_KEY_NIGHT       "gui_night"
#define GUI_NVS_KEY_BRIGHT      "gui_bright"
#define GUI_NVS_KEY_WIFI_SSID   "wifi_ssid"

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

    bool has_last_brightness;
    int32_t last_brightness;

    bool boot_autoconnect_queued;
    char requested_ssid[GUI_WIFI_SSID_MAX_LEN];
} app_gui_bindings_ctx_t;

static app_gui_bindings_ctx_t s_bindings;

// Forward declarations
//////////////////////////

/**
 * @brief Copy the latest local sensor sample into the GUI state.
 *
 * When a fresh local sample is available, this updates the GUI sensor model
 * with the latest readings and freshness metadata.
 *
 * @param gui Initialized GUI context to update.
 */
static void sync_sensor(gui_ctx_t *gui);

/**
 * @brief Refresh the cached GUI Wi-Fi state indicator when it changes.
 *
 * @param gui Initialized GUI context to update.
 * @return True when the GUI state changed, otherwise false.
 */
static bool sync_wifi_state(gui_ctx_t *gui);

/**
 * @brief Refresh the sidebar SD card indicator when its state changes.
 *
 * @param gui Initialized GUI context to update.
 * @return True when the GUI state changed, otherwise false.
 */
static bool sync_sd_card_state(gui_ctx_t *gui);

/**
 * @brief Refresh the GUI Wi-Fi model from the current NAC state.
 *
 * This applies scan results, connection progress, disconnect events, and
 * remembered SSID metadata so the GUI stays aligned with network activity.
 *
 * @param gui Initialized GUI context to update.
 */
static void sync_wifi(gui_ctx_t *gui);

/**
 * @brief Clamp a persisted brightness value into the supported GUI range.
 *
 * @param value Stored brightness percentage.
 * @return Brightness clamped to the supported range.
 */
static int32_t clamp_saved_brightness(int32_t value);

/**
 * @brief Load persisted appearance settings from NVS for GUI startup.
 *
 * This loads any saved theme, background, night-mode, and brightness values
 * into a GUI init config so the first render uses the persisted settings.
 *
 * @param config Startup configuration to populate.
 * @return True when at least one saved value was loaded, otherwise false.
 */
bool app_gui_bindings_load_saved_appearance(gui_init_config_t *config);

/**
 * @brief Seed the last-saved appearance cache from the current GUI state.
 *
 * @param gui Initialized GUI context to inspect.
 */
static void cache_current_appearance(gui_ctx_t *gui);

/**
 * @brief Persist GUI appearance changes when they differ from the last save.
 *
 * @param gui Initialized GUI context to inspect.
 * @return True when a changed value was written to NVS, otherwise false.
 */
static bool save_appearance_if_changed(gui_ctx_t *gui);

/**
 * @brief Restore remembered Wi-Fi metadata into the GUI model.
 *
 * Currently this restores the last saved SSID as a known network and selected
 * target network when one is not already selected.
 *
 * @param gui Initialized GUI context to update.
 * @return True when saved Wi-Fi metadata was loaded, otherwise false.
 */
static bool load_saved_wifi_metadata(gui_ctx_t *gui);

/**
 * @brief Reload remembered Wi-Fi metadata after a connection-state update.
 *
 * @param gui Initialized GUI context to update.
 * @return True when saved Wi-Fi metadata was loaded, otherwise false.
 */
static bool refresh_saved_wifi_metadata(gui_ctx_t *gui);

/**
 * @brief Queue a boot-time Wi-Fi connection using the remembered network.
 *
 * The request is issued at most once per boot after remembered Wi-Fi metadata
 * has been restored into the GUI state.
 *
 * @param gui Initialized GUI context to update.
 * @return True when an auto-connect request was queued, otherwise false.
 */
static bool queue_saved_wifi_autoconnect(gui_ctx_t *gui);

/**
 * @brief Handle top-level panel change notifications from the GUI.
 *
 * @param gui Initialized GUI context that emitted the event.
 * @param panel Newly active panel.
 * @param user_data Opaque binding context supplied during registration.
 */
static void on_panel_changed(gui_ctx_t *gui, gui_panel_id_t panel, void *user_data);

/**
 * @brief Start a Wi-Fi scan requested by the GUI.
 *
 * This clears any pending connect state, resets the selected network in the
 * GUI model, and forwards the scan request to the network control layer.
 *
 * @param gui Initialized GUI context that emitted the event.
 * @param user_data Opaque binding context supplied during registration.
 * @return True after the request is handled, including failure reporting.
 */
static bool on_wifi_scan_requested(gui_ctx_t *gui, void *user_data);

/**
 * @brief Observe the currently selected scanned Wi-Fi network.
 *
 * The current implementation logs the selection and leaves the actual connect
 * flow to the explicit connect request callback.
 *
 * @param gui Initialized GUI context that emitted the event.
 * @param network Selected network, or NULL when no selection is available.
 * @param user_data Opaque binding context supplied during registration.
 */
static void on_wifi_network_selected(gui_ctx_t *gui, const gui_wifi_network_t *network, void *user_data);

/**
 * @brief Start connection flow for a remembered Wi-Fi network.
 *
 * @param gui Initialized GUI context that emitted the event.
 * @param network Remembered network chosen by the user.
 * @param user_data Opaque binding context supplied during registration.
 * @return True after the request is handled, including failure reporting.
 */
static bool on_wifi_known_network_requested(gui_ctx_t *gui, const gui_wifi_network_t *network, void *user_data);

/**
 * @brief Request a Wi-Fi connection using explicit credentials.
 *
 * @param gui Initialized GUI context that emitted the event.
 * @param ssid Target SSID, or NULL when reconnecting with saved credentials.
 * @param password Password associated with @p ssid, or NULL when unused.
 * @param user_data Opaque binding context supplied during registration.
 * @return True after the request is handled, including failure reporting.
 */
static bool on_wifi_connect_requested(gui_ctx_t *gui, const char *ssid, const char *password, void *user_data);

/**
 * @brief Request disconnection from the current Wi-Fi network.
 *
 * @param gui Initialized GUI context that emitted the event.
 * @param user_data Opaque binding context supplied during registration.
 * @return True after the request is handled, including failure reporting.
 */
static bool on_wifi_disconnect_requested(gui_ctx_t *gui, void *user_data);

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
        snprintf(wifi.status_text, sizeof(wifi.status_text), "%s",
                 "Scanning for Wi-Fi networks...");
    } else if (s_bindings.wifi_scan_requested && nac_scan_is_complete()) {
        s_bindings.wifi_scan_requested = false;
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
        if (wifi.selected_ssid[0] != '\0') {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "Connecting to %s...",
                     wifi.selected_ssid);
        } else {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "%s",
                     "Connecting to Wi-Fi...");
        }
    } else if (nac_status == NAC_WIFI_CONNECTED) {
        s_bindings.wifi_connect_requested = false;
        s_bindings.wifi_disconnect_requested = false;
        wifi.connect_requested = false;
        wifi.can_disconnect = true;
        if (wifi.selected_ssid[0] != '\0') {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "Connected to %s.",
                     wifi.selected_ssid);
        } else {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "%s",
                     "Wi-Fi connected.");
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
            snprintf(wifi.status_text, sizeof(wifi.status_text),
                     "Failed to connect to %s.", wifi.selected_ssid);
        } else {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "%s",
                     "Wi-Fi connection failed.");
        }
    } else if (s_bindings.wifi_connect_requested &&
               nac_status == NAC_WIFI_DISCONNECTED) {
        s_bindings.wifi_connect_requested = false;
        wifi.connect_requested = false;
        wifi.can_disconnect = false;
        wifi.state = GUI_WIFI_STATE_IDLE;
        snprintf(wifi.status_text, sizeof(wifi.status_text), "%s",
                 "Wi-Fi disconnected.");
    } else if (s_bindings.wifi_connect_requested) {
        wifi.state = GUI_WIFI_STATE_CONNECTING;
        wifi.connect_requested = true;
        if (wifi.selected_ssid[0] != '\0') {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "Connecting to %s...",
                     wifi.selected_ssid);
        } else {
            snprintf(wifi.status_text, sizeof(wifi.status_text), "%s",
                     "Connecting to Wi-Fi...");
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
            snprintf(wifi.status_text, sizeof(wifi.status_text), "%s",
                     "Press Scan to search for Wi-Fi networks.");
        }
    }

    gui_set_wifi_settings(gui, &wifi);
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

    appearance_changed =
        !s_bindings.has_last_appearance ||
        (appearance.theme != s_bindings.last_appearance.theme) ||
        (appearance.show_background_image !=
         s_bindings.last_appearance.show_background_image) ||
        (appearance.night_variant_enabled !=
         s_bindings.last_appearance.night_variant_enabled);

    brightness_changed =
        !s_bindings.has_last_brightness ||
        (brightness != s_bindings.last_brightness);

    if (!appearance_changed && !brightness_changed) {
        return false;
    }

    if (appearance_changed) {
        err = rm_nvs_set_u8(GUI_NVS_KEY_THEME, (uint8_t)appearance.theme);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "rm_nvs_set_u8(%s) failed: %s",
                     GUI_NVS_KEY_THEME, esp_err_to_name(err));
            return false;
        }

        err = rm_nvs_set_u8(GUI_NVS_KEY_BG,
                            appearance.show_background_image ? 1U : 0U);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "rm_nvs_set_u8(%s) failed: %s",
                     GUI_NVS_KEY_BG, esp_err_to_name(err));
            return false;
        }

        err = rm_nvs_set_u8(GUI_NVS_KEY_NIGHT,
                            appearance.night_variant_enabled ? 1U : 0U);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "rm_nvs_set_u8(%s) failed: %s",
                     GUI_NVS_KEY_NIGHT, esp_err_to_name(err));
            return false;
        }

        s_bindings.last_appearance = appearance;
        s_bindings.has_last_appearance = true;
    }

    if (brightness_changed) {
        err = rm_nvs_set_u8(GUI_NVS_KEY_BRIGHT, (uint8_t)brightness);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "rm_nvs_set_u8(%s) failed: %s",
                     GUI_NVS_KEY_BRIGHT, esp_err_to_name(err));
            return false;
        }

        s_bindings.last_brightness = brightness;
        s_bindings.has_last_brightness = true;
    }

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
    snprintf(wifi.known_networks[0].ssid, sizeof(wifi.known_networks[0].ssid), "%s",
             saved_ssid);
    wifi.known_networks[0].secured = true;
    wifi.known_networks[0].signal_strength_pct = 0;
    wifi.known_network_count = 1;

    if (wifi.selected_ssid[0] == '\0') {
        snprintf(wifi.selected_ssid, sizeof(wifi.selected_ssid), "%s", saved_ssid);
    }

    snprintf(s_bindings.requested_ssid, sizeof(s_bindings.requested_ssid), "%s",
             saved_ssid);
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
        snprintf(status_text, sizeof(status_text), "Connecting to %s...",
                 s_bindings.requested_ssid);
        set_wifi_status(gui, status_text, GUI_WIFI_STATE_CONNECTING);
    } else {
        set_wifi_status(gui, "Connecting to Wi-Fi...", GUI_WIFI_STATE_CONNECTING);
    }

    result = nac_request_wifi_connect(NULL, NULL);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "nac_request_wifi_connect(NULL, NULL) failed: %s",
                 esp_err_to_name(result));
        s_bindings.wifi_connect_requested = false;
        set_wifi_status(gui, "Failed to request Wi-Fi connection.",
                        GUI_WIFI_STATE_FAILED);
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

    bindings.user_data = &s_bindings;
    bindings.on_panel_changed = on_panel_changed;
    bindings.on_wifi_scan_requested = on_wifi_scan_requested;
    bindings.on_wifi_network_selected = on_wifi_network_selected;
    bindings.on_wifi_known_network_requested = on_wifi_known_network_requested;
    bindings.on_wifi_connect_requested = on_wifi_connect_requested;
    bindings.on_wifi_disconnect_requested = on_wifi_disconnect_requested;
    gui_set_bindings(gui, &bindings);

    cache_current_appearance(gui);
    (void)load_saved_wifi_metadata(gui);
    app_gui_bindings_sync(gui);
    (void)queue_saved_wifi_autoconnect(gui);
    return ESP_OK;
}

void app_gui_bindings_sync(gui_ctx_t *gui)
{
    bool wifi_state_changed;

    if (gui == NULL) {
        return;
    }

    sync_sensor(gui);
    wifi_state_changed = sync_wifi_state(gui);
    sync_sd_card_state(gui);
    if (wifi_state_changed || s_bindings.wifi_scan_requested ||
        s_bindings.wifi_connect_requested || s_bindings.wifi_disconnect_requested) {
        sync_wifi(gui);
    }
    (void)save_appearance_if_changed(gui);
}