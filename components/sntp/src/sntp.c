#include "sntp.h"
#include "esp_sntp.h"
#include "esp_log.h"

static const char *TAG = "TIME";

static void on_time_synced(struct timeval *tv)
{
    ESP_LOGI(TAG, "Time synced");
}

/*
 * Starts the SNTP client and returns immediately.
 * The system clock is set asynchronously by the SNTP stack once the first
 * response arrives from pool.ntp.org. Re-syncs automatically every hour.
 * Call sntp_sync_stop() when the network goes down.
 */
void sntp_sync_start(void)
{
    ESP_LOGI(TAG, "Starting SNTP");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_set_time_sync_notification_cb(on_time_synced);
    esp_sntp_init();
}

/*
 * Stops the SNTP client.
 */
void sntp_sync_stop(void)
{
    ESP_LOGI(TAG, "Stopping SNTP");
    esp_sntp_stop();
}