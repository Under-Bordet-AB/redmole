/* NAC - Network and communication
 * - This module handles wifi and bluetooth
 */

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
static void wifi_bring_hw_online();
static void wifi_bring_hw_offline();
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

int8_t wifi_init(wifi_ctx_t *self)
{
    self->state = WIFI_STATE_IDLE;
    self->tag   = "WIFI MODULE";
    self->netif = esp_netif_create_default_wifi_sta();
    if (self->netif == NULL)
    {
        ESP_LOGE("WIFI", "Failed to create default WiFi station netif");
        return -1;
    }

    /* Init core Subsystems */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    /* Init WiFi driver */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Register event handlers*/
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

    /* Start hardware */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
    ESP_LOGI(self->tag, "WiFi initialized. Waiting for connection...");

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
                        ESP_LOGI(self->tag, "esp_wifi_connect failed: %d", err);
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
 * Implement these functions to always know initial state
 */
static void wifi_bring_hw_online(); // Will contain unsub wifi events
static void wifi_bring_hw_offline(); // Will contain reg wifi events
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
