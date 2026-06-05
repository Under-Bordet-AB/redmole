#include "esp_err.h"
#include "esp_event_base.h"
#include "task_scheduler.h"
#include "unity.h"
#include "nac.h"
#include <stdint.h>

#define WIFI_SSID_MAX_LENGTH  32 /*!< Maximum SSID length in bytes, as defined by 802.11. */
#define WIFI_PASS_MAX_LENGTH  64 /*!< Maximum passphrase length in bytes. */

const char* TAG = "NAC TEST";
static char s_wifi_ssid[WIFI_CRED_MAX_LENGTH];
static char s_wifi_pass[WIFI_CRED_MAX_LENGTH];


static EventGroupHandle_t s_event_group_handle;
typedef struct
{
    wifi_ctx_t          wifi;
    EventGroupHandle_t *event_group;
} nac_ctx_t;

static nac_ctx_t self;

void setUp(void)
{
    task_scheduler_init();                              // Needed for nac_request_wifi_connect
    strcpy(s_wifi_ssid, "ssid");
    strcpy(s_wifi_pass, "pass");
    s_event_group_handle = xEventGroupCreate();         // Needed for nac_init
    ESP_ERROR_CHECK(esp_netif_init());                  // Needed for wifi functionality
    ESP_ERROR_CHECK(esp_event_loop_create_default());   // Needed for event handling
    nac_init(&s_event_group_handle);
}

void tearDown(void)
{
    // Task scheduler cleanup is not needed
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

/* Wifi request connect tests
 * The function should return ESP_OK when wifi state is connected or connecting.
 * Should return 0/ ESP_OK when handed of to task scheduler
 */
void test_wifi_request_connect_returns_ESP_OK_when_connected(void)
{
    self.wifi.state = NAC_WIFI_CONNECTED;
    TEST_ASSERT_EQUAL(ESP_OK, nac_request_wifi_connect(s_wifi_ssid, s_wifi_pass));
}

void test_wifi_request_connect_returns_ESP_OK_when_connecting(void)
{
    self.wifi.state = NAC_WIFI_CONNECTING;
    TEST_ASSERT_EQUAL(ESP_OK, nac_request_wifi_connect(s_wifi_ssid, s_wifi_pass));
}

void test_wifi_request_connect_returns_ESP_OK_when_state_is_idle(void)
{
    self.wifi.state = WIFI_STATE_IDLE;
    TEST_ASSERT_EQUAL(ESP_OK, nac_request_wifi_connect(s_wifi_ssid, s_wifi_pass));
}

/* Should return valid pointer to PSRAM and zero when not called through the scan function */
void test_get_scan_result(void)
{
    uint16_t count = 99;
    const wifi_ap_record_t *psram_ptr = nac_get_scan_results(&count);
    TEST_ASSERT_NOT_NULL(psram_ptr);
    TEST_ASSERT_EQUAL(0, count);
}

void app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_wifi_state_after_init);
    ESP_LOGI(TAG, "NAC module is in correct state after initialization.");
    RUN_TEST(test_wifi_request_connect_returns_ESP_OK_when_connected);
    ESP_LOGI(TAG, "NAC request connect function returns correctly when already connected.");
    RUN_TEST(test_wifi_request_connect_returns_ESP_OK_when_connecting);
    ESP_LOGI(TAG, "NAC request connect function returns correctyly when already connecting");
    RUN_TEST(test_get_scan_result);
    ESP_LOGI(TAG, "NAC get scan result returns valid pointer to PSRAM and O scanned AP when not called via scan function.");
    UNITY_END();
}
