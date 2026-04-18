#include "app_module.h"
#include "nac.h"
#include "task_scheduler.h"
#include "esp_log.h"

static const char *TAG = "MAIN_TEST";

void app_main(void)
{
    app_ctx_t *app = app_init();
    task_scheduler_init();
    if (!app) esp_restart();

    /* ALL THE CODE HERE IS SIMPLY TO TEST FUNCTIONALITY */
    bool scan_triggered = false;
    bool connect_triggered = false;

    while (1)
    {

        task_scheduler_work();

        nac_wifi_status_t status = nac_get_wifi_status(app->nac);
        //ESP_LOGI(TAG, "state=%d status=%d scan=%d scan done=%d connect=%d",
        //         app->nac->wifi.state, status, scan_triggered, app->nac->wifi.scan_complete, connect_triggered);
        /* 1. Trigger Scan */
        if (!scan_triggered)
        {
            ESP_LOGI(TAG, "Starting Scan...");
            if (nac_request_wifi_scan(app->nac) == 0) scan_triggered = true;
        }
        /* 2. Process Scan Results */
        else if (scan_triggered && app->nac->wifi.scan_complete && !connect_triggered)
        {
            uint16_t count = 0;
            const wifi_ap_record_t *list = nac_get_scan_results(app->nac, &count);
            ESP_LOGI(TAG, "WiFi Status: %d", nac_get_wifi_status(app->nac));
            ESP_LOGI(TAG, "Found %d Access Points", count);
            for (int i = 0; i < count; i++)
            {
                ESP_LOGI(TAG, "SSID: %s (RSSI: %d)", list[i].ssid, list[i].rssi);
            }

            /* 3. Trigger Connection */
            ESP_LOGI(TAG, "Connecting to %s...", CONFIG_REDMOLE_WIFI_SSID);
            if (nac_request_wifi_connect(app->nac) == 0) connect_triggered = true;
        }
        /* 4. Monitor Connection */
        else if (connect_triggered && status == NAC_WIFI_CONNECTED)
        {
            // ESP_LOGI(TAG, "WiFi System Online and Robust.");
            /* Test Sequence Complete - stay in loop for maintenance */
            }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
