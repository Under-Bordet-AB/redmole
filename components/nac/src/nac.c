/* NAC - Network and communication
 * - This module handles wifi and bluetooth
 */

#include "esp_err.h"
#include "esp_log_level.h"
#include "esp_netif_types.h"
#include "esp_wifi_default.h"
#include "esp_wifi_types_generic.h"
#include "../include/nac.h"
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#define REDMOLE_WIFI_SSID      CONFIG_REDMOLE_WIFI_SSID
#define REDMOLE_WIFI_PASS      CONFIG_REDMOLE_WIFI_PASSWORD
#define REDMOLE_MAX_RETRY      CONFIG_REDMOLE_MAXIMUM_RETRY

#if CONFIG_REDMOLE_WPA3_SAE_PWE_HUNT_AND_PECK
    #define REDMOLE_WPA3_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
    #define REDMOLE_H2E_IDENTIFIER
#elif CONFIG_REDMOLE_WPA3_SAE_PWE_HASH_TO_ELEMENT
    #define REDMOLE_WPA3_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
    #define REDMOLE_H2E_IDENTIFIER CONFIG_REDMOLE_WIFI_PW_ID
#elif CONFIG_REDMOLE_WPA3_SAE_PWE_BOTH
    #define REDMOLE_WPA3_SAE_MODE WPA3_SAE_PWE_BOTH
    #define REDMOLE_H2E_IDENTIFIER CONFIG_REDMOLE_WIFI_PW_ID
#endif

#if CONFIG_REDMOLE_AUTH_OPEN
    #define REDMOLE_AUTH_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_REDMOLE_AUTH_WEP
    #define REDMOLE_AUTH_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_REDMOLE_AUTH_WPA_PSK
    #define REDMOLE_AUTH_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_REDMOLE_AUTH_WPA2_PSK
    #define REDMOLE_AUTH_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_REDMOLE_AUTH_WPA_WPA2_PSK
    #define REDMOLE_AUTH_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_REDMOLE_AUTH_WPA3_PSK
    #define REDMOLE_AUTH_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_REDMOLE_AUTH_WPA2_WPA3_PSK
    #define REDMOLE_AUTH_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_REDMOLE_AUTH_WAPI_PSK
    #define REDMOLE_AUTH_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

/* THESE ARE INTERNAL HELPER FUNCTIONS FOR WIFI MANAGMENT */
/* ------------------------------------------------------ */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);
static int8_t wifi_bring_hw_online(wifi_ctx_t *self);
static int8_t wifi_bring_hw_offline(wifi_ctx_t *self);
static void wifi_disconnect();
static void wifi_reconnect();
static void wifi_scan_done();
/* ------------------------------------------------------ */

/* @brief Initializes the NAC context.
 * @Return A pointer to the initialized NAC context, or NULL if initialization fails.
 * @Desc: Allocates memory for the NAC context and initializes its components.
 */
nac_ctx_t *nac_init()
{
    nac_ctx_t *self = (nac_ctx_t *)malloc(sizeof(nac_ctx_t));
    if (!self)
    {
        ESP_LOGE("NAC", "Failed to allocate memory for nac_ctx_t");
        return NULL;
    }

    memset(self, 0, sizeof(nac_ctx_t));

    if (wifi_init(&self->wifi) != 0)
    {
        free(self);
        return NULL;
    }
    if (bluetooth_init(&self->bluetooth) != 0)
    {
        free(self);
        return NULL;
    }

    return self;
}

/* @brief Initializes the WiFi context.
 * @param self Pointer to the WiFi context to initialize.
 * @return 0 on success, -1 on failure.
 * @Desc: sets up the WiFi context with default values, does not initialize the hardware.
 */
int8_t wifi_init(wifi_ctx_t *self)
{
    self->netif = esp_netif_create_default_wifi_sta();
    if (self->netif == NULL)
    {
        ESP_LOGE("WIFI", "Failed to create default WiFi station netif");
        return -1;
    }
    self->state         = WIFI_STATE_IDLE;
    self->tag           = "WIFI MODULE";
    self->saved_to_nvs  = 0;
    self->hw_online     = 0;

    // This gets moved in to wifi_connect and wifi_bring_hw_online
    /* Init WiFi driver */
    /*
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        self,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        self,
                                                        &instance_got_ip));

    wifi_config_t wifi_config =
        {
        .sta =
        {
            .ssid = REDMOLE_WIFI_SSID,
            .password = REDMOLE_WIFI_PASS,
            .threshold.authmode = REDMOLE_AUTH_THRESHOLD,
            .sae_pwe_h2e = REDMOLE_WPA3_SAE_MODE,
            .sae_h2e_identifier = REDMOLE_H2E_IDENTIFIER
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
    ESP_LOGI(self->tag, "WiFi initialized. Waiting for connection...");
    */

    return 0;
}

/* @brief Initializes the Bluetooth context.
 * @param self Pointer to the Bluetooth context to initialize.
 * @return 0 on success, -1 on failure.
 * @Desc: sets up the Bluetooth context with default values, does not initialize the hardware.
 */
int8_t bluetooth_init(bluetooth_ctx_t *self)
{
    self->state = BLUETOOTH_STATE_IDLE;
    self->bluetooth_event_group = xEventGroupCreate();
    if (self->bluetooth_event_group == NULL)
    {
        ESP_LOGE("BLUETOOTH", "Failed to create Bluetooth event group");
        return -1;
    }

    return 0;
}

/* @brief Brings the WiFi hardware online.
 * @param self Pointer to the WiFi context.
 * @return 0 on success, -1 on failure.
 * @Desc: initializes the WiFi hardware, sets the mode to STA, and registers event handlers.
 */
static int8_t wifi_bring_hw_online(wifi_ctx_t *self)
{
    if (self->hw_online)
    {
        return 0;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&cfg) != ESP_OK)
    {
        ESP_LOGE("WIFI", "Failed to initialize WiFi");
        return -1;
    }

    if (esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, self) != ESP_OK)
    {
        ESP_LOGE("WIFI", "Failed to register WiFi event handler");
        esp_wifi_deinit();
        return -1;
    }

    if (esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, self) != ESP_OK)
    {
        ESP_LOGE("WIFI", "Failed to register IP event handler");
        esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler);
        esp_wifi_deinit();
        return -1;
    }

    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK)
    {
        ESP_LOGE("WIFI", "Failed to set WiFi mode to STA");
        esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler);
        esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler);
        esp_wifi_deinit();
        return -1;
    }

    self->hw_online = 1;
    ESP_LOGI(self->tag, "WiFi hardware online");

    return 0;
}

/* @Brief: Event handler for WiFi events.
 * @Param arg: Pointer to the NAC context.
 * @Param event_base: The event base (WIFI_EVENT).
 * @Param event_id: The event ID.
 * @Param event_data: Pointer to the event data.
 * @Desc: Handles WiFi events such as connection start, scan done, and connection status changes.
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    wifi_ctx_t *self = (wifi_ctx_t*)arg;
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
            case WIFI_EVENT_STA_START:
            {
                if (self->state == WIFI_STATE_CONNECTING)
                {
                    esp_err_t err = esp_wifi_connect();
                    if (err != ESP_OK && err != ESP_ERR_WIFI_CONN)
                    {
                        ESP_LOGE(self->tag, "esp_wifi_connect failed: %d", err);
                    }
                }
                break;
            }
            case WIFI_STATE_DISCONNECTED:
            {
                if (self->state == WIFI_STATE_CONNECTED || self->state == WIFI_STATE_CONNECTING)
                {
                    ESP_LOGI(self->tag, "Disconnected, attempting to reconnect...");
                    wifi_reconnect();
                }
                break;
            }
            case WIFI_EVENT_SCAN_DONE:
            {
                ESP_LOGI(self->tag, "WiFi scan done.");
                wifi_scan_done();
            }
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(self->tag, "Connected, IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        self->state = WIFI_STATE_CONNECTED;
    }
}

/*
 * Will contain hw_tear_down
 */
static void wifi_disconnect();
/*
 * Will contain hw_tear_down and hw_bring_up
 */
static void wifi_reconnect();


void wifi_dispose(wifi_ctx_t *self)
{
    // TODO: implement
}
