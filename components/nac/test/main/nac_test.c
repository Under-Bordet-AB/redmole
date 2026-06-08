#include "esp_err.h"
#include "esp_event_base.h"
#include "rm_nvs.h"
#include "task_scheduler.h"
#include "unity.h"
#include "nac.h"
#include <stdint.h>
#include <stdbool.h>

static char s_wifi_ssid[WIFI_CRED_MAX_LENGTH];
static char s_wifi_pass[WIFI_CRED_MAX_LENGTH];

const char *TAG = "NAC TEST";

static EventGroupHandle_t s_event_group_handle;

void setUp(void)
{
    task_scheduler_init();
    rm_nvs_init("app");
    strcpy(s_wifi_ssid, "ssid");
    strcpy(s_wifi_pass, "pass");
    s_event_group_handle = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    nac_init(&s_event_group_handle);
}

void tearDown(void)
{
    rm_nvs_deinit();
    nac_dispose();
    vEventGroupDelete(s_event_group_handle);
    esp_event_loop_delete_default();
    esp_netif_deinit();
}

/* Init sets WIFI_STATE to IDLE, the function falls through to NAC_WIFI_DISCONNECTED */
void test_wifi_state_after_init(void)
{
    TEST_ASSERT_EQUAL(NAC_WIFI_DISCONNECTED, nac_get_wifi_status());
}

/* Module accepts the request from IDLE and transitions to CONNECTING */
void test_connect_request_accepted_from_idle(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, nac_request_wifi_connect(s_wifi_ssid, s_wifi_pass));
    TEST_ASSERT_EQUAL(NAC_WIFI_CONNECTING, nac_get_wifi_status());
}

/* PSRAM buffer is allocated at init, count is zero before any scan has run */
void test_get_scan_result_before_scan(void)
{
    uint16_t count = 99;
    const wifi_ap_record_t *records = nac_get_scan_results(&count);
    TEST_ASSERT_NOT_NULL(records);
    TEST_ASSERT_EQUAL(0, count);
}

/* Scan completes and returns a valid buffer with zero or more APs */
void test_scan_completes(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, nac_request_wifi_scan());
    task_scheduler_work();

    int retries = 0;
    while (!nac_scan_is_complete() && retries++ < 100)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    uint16_t count = 0;
    const wifi_ap_record_t *records = nac_get_scan_results(&count);
    TEST_ASSERT_TRUE(nac_scan_is_complete());
    TEST_ASSERT_NOT_NULL(records);
    TEST_ASSERT_GREATER_OR_EQUAL(0, count);
    ESP_LOGI(TAG, "Scan completed! PSRAM ptr is not NULL and AP count is: %d", count);
}

void app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_wifi_state_after_init);
    RUN_TEST(test_connect_request_accepted_from_idle);
    RUN_TEST(test_get_scan_result_before_scan);
    RUN_TEST(test_scan_completes);
    UNITY_END();
}
