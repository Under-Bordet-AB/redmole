#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#include "rm_nvs.h" // JJ: single instance NVS wrapper
#include "app_module.h" // Only include this once!

static const char *TAG = "MAIN";

static bool init_single_instance_modules(void)
{
    esp_err_t rv;

    rv = rm_nvs_init("app");
    if (rv != ESP_OK){
        ESP_LOGE(TAG, "rm_nvs_init failed: %s", esp_err_to_name(rv));
        goto error;
    }

    return true;

    error:
    return false;
}

void app_main(void)
{
    if (init_single_instance_modules() == false) {
        goto fatal_error;
    }

    app_ctx_t *app_ctx = app_init();
    if (!app_ctx) {
        esp_restart();
    }

    BaseType_t wifi = xTaskCreate(wifi_task, "wifi_task", 4096, &app_ctx->wifi, 5, NULL);
    if (wifi != pdPASS) {
        ESP_LOGE(TAG, "Could not spawn wifi_task!");
    }

    /* Future while loop here for monitoring
     * Eventgroups to the rescue!
     */

fatal_error:
    // log and retry?
    return;
}
