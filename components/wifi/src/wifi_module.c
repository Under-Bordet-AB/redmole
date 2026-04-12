/*
 * WiFi Module
 */
#include "wifi_module.h"
#include <string.h>
#include <unistd.h>
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#define REDMOLE_WIFI_SSID      CONFIG_REDMOLE_WIFI_SSID
#define REDMOLE_WIFI_PASS      CONFIG_REDMOLE_WIFI_PASSWORD
#define REDMOLE_MAX_RETRY      CONFIG_REDMOLE_MAXIMUM_RETRY

#if CONFIG_REDMOLE_WPA3_SAE_PWE_HUNT_AND_PECK
    #define REDMOLE_WPA3_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
    #define REDMOLE_H2E_IDENTIFIER ""
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

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    wifi_ctx_t *self = (wifi_ctx_t*)arg;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        self->state = WIFI_STATE_CONNECTING;
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (self->retry_count < REDMOLE_MAX_RETRY)
        {
            esp_wifi_connect();
            self->retry_count++;
            ESP_LOGI(self->tag, "Retryingx connection, try number %d", self->retry_count);
        }
        else
        {
            self->state = WIFI_STATE_DISCONNECTED;
            ESP_LOGI(self->tag, "Failed to connect, max retries reached");
            xEventGroupSetBits(self->wifi_event_group, WIFI_FAIL_BIT);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        self->state = WIFI_STATE_CONNECTED;
        self->retry_count = 0;
        ESP_LOGI(self->tag, "Connected, IP address: %s", ip4addr_ntoa(&((ip_event_got_ip_t*)event_data)->ip_info.ip));
        xEventGroupSetBits(self->wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init(wifi_ctx_t *self)
{
    self->state = WIFI_STATE_DISCONNECTED;
    self->tag   = "WIFI MODULE";
    self->retry_count = 0;
    self->wifi_event_group = xEventGroupCreate();

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
            .sae_h2e_identifier = REDMOLE_SAE_H2E_IDENTIFIER
        },
    };

    /* Start hardware */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
    ESP_LOGI(self->tag, "WiFi initialized. Waiting for connection...");
}

void wifi_task(void *pvParameters)
{
    wifi_ctx_t *self = (wifi_ctx_t *)pvParameters;

    while (1)
    {
        /* * This is our "Gatekeeper."
         * We block here for up to 500ms. If NO event happens, 'bits' will be 0.
         * This prevents the loop from spinning at 240MHz for no reason.
         */
        EventBits_t bits = xEventGroupWaitBits(self->wifi_event_group,
                                               WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                               pdTRUE,
                                               pdFALSE,
                                               pdMS_TO_TICKS(500));

        switch (self->state)
        {
            case WIFI_STATE_DISCONNECTED:
                if (self->retry_count < REDMOLE_MAX_RETRY)
                {
                    ESP_LOGI(self->tag, "Retrying... Attempt: (%d)", self->retry_count);
                    self->state = WIFI_STATE_CONNECTING;
                    self->retry_count++;
                    esp_wifi_connect();
                }
                else
                {
                    ESP_LOGE(self->tag, "Max attempts reached. Waiting before trying again.");
                    vTaskDelay(pdMS_TO_TICKS(10000)); // Sleep 10s before trying again
                    self->retry_count = 0;
                }
                break;

            case WIFI_STATE_CONNECTING:
                if (bits & WIFI_CONNECTED_BIT)
                {
                    self->state = WIFI_STATE_CONNECTED;
                    self->retry_count = 0;
                    ESP_LOGI(self->tag, "RedMole is now connected");
                }
                else if (bits & WIFI_FAIL_BIT)
                {
                    self->state = WIFI_STATE_DISCONNECTED;
                }
                break;

            case WIFI_STATE_CONNECTED:
            /* We will stay here
             * Until bits AND WIFI_FAIL_BIT is 1
             */
                if (bits & WIFI_FAIL_BIT)
                {
                    ESP_LOGW(self->tag, "Lost connection to WiFi network: %s", REDMOLE_WIFI_SSID);
                    self->state = WIFI_STATE_DISCONNECTED;
                }
                break;

            default:
                break;
        }

        /* Yield */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void wifi_deinit(wifi_ctx_t *self)
{
    // TODO: implement
}
