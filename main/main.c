#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_err.h"

#include "app_module.h"

void app_main(void)
{
    const char *TAG = "MAIN";
    app_ctx_t *app_ctx = app_init();
    if (!app_ctx) esp_restart();

    /*
     * We will only init the wifi never dedicate a task to it
    BaseType_t wifi = xTaskCreate(wifi_task, "wifi_task", 4096, &app_ctx->wifi, 5, NULL);
    if (wifi != pdPASS)
    {
        ESP_LOGE(TAG, "Could not spawn wifi_task!");
    }
    */

    /* Future while loop here for monitoring
     * Eventgroups to the rescue!
     */
}
