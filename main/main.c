#include "app_module.h"
#include "nac.h"
#include "task_scheduler.h"
#include "esp_log.h"
#include <stdint.h>

static const char *TAG = "MAIN_TEST";

void app_main(void)
{
    app_ctx_t *app = app_init();
    task_scheduler_init();
    if (!app) esp_restart();

    nac_request_wifi_scan(app->nac);

    TickType_t connected_at = 0;
    const uint32_t CONNECTION_TIMEOUT_MS = 30000;

    while (1)
    {
        task_scheduler_work();

        nac_wifi_status_t status = nac_get_wifi_status(app->nac);
        if (app->nac->wifi.scan_complete)
        {
            uint16_t count = 0;
            const wifi_ap_record_t *list = nac_get_scan_results(app->nac, &count);
            ESP_LOGI(TAG, "Found %d APs", count);
            for (int i = 0; i < count; i++)
            {
                ESP_LOGI(TAG, "  %s (RSSI: %d)", list[i].ssid, list[i].rssi);
            }
            app->nac->wifi.scan_complete = 0;
            nac_request_wifi_connect(app->nac);
        }

        if (status == NAC_WIFI_CONNECTED && connected_at == 0)
        {
            ESP_LOGI(TAG, "Connected — will disconnect in %"PRIu32" ms", CONNECTION_TIMEOUT_MS);
            connected_at = xTaskGetTickCount();
        }

        if (connected_at != 0 &&
            (int32_t)(xTaskGetTickCount() - connected_at) >= (int32_t)pdMS_TO_TICKS(CONNECTION_TIMEOUT_MS))
        {
            ESP_LOGI(TAG, "Simulating connection loss — disconnecting");
            nac_request_wifi_disconnect(app->nac);
            connected_at = 0;

            app->nac->wifi.scan_complete = 0;
            nac_request_wifi_scan(app->nac);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
