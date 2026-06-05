#include "esp_event_base.h"
#include "unity.h"
#include "nac.h"

#define WIFI_SSID_MAX_LENGTH  32 /*!< Maximum SSID length in bytes, as defined by 802.11. */
#define WIFI_PASS_MAX_LENGTH  64 /*!< Maximum passphrase length in bytes. */
static char s_wifi_ssid[WIFI_CRED_MAX_LENGTH];
static char s_wifi_pass[WIFI_CRED_MAX_LENGTH];


static EventGroupHandle_t s_event_group_handle;
typedef struct
{
    wifi_ctx_t          wifi;
    EventGroupHandle_t *event_group;
} nac_ctx_t;

static nac_ctx_t s_nac_ctx;

void setUp(void)
{
    strcpy(s_wifi_ssid, "ssid");
    strcpy(s_wifi_pass, "pass");
    s_event_group_handle = xEventGroupCreate();
    nac_init(&s_event_group_handle);
}

void tearDown(void)
{
    nac_dispose();
    vEventGroupDelete(s_event_group_handle);
    esp_event_loop_delete_default();
    esp_netif_deinit();
}

void test_wifi_state_after_init(void)
{
    TEST_ASSERT_EQUAL(NAC_WIFI_DISCONNECTED, nac_get_wifi_status());
}

void test_wifi_request_connect_returns_ESP_OK_when_connected(void)
{
    s_nac_ctx.wifi.state = NAC_WIFI_CONNECTED;
    TEST_ASSERT_EQUAL(ESP_OK, nac_request_wifi_connect(s_wifi_ssid, s_wifi_pass));
}

void test_get_scan_result(void)
{
    uint16_t sp_out;
    TEST_ASSERT_EQUAL(NULL, nac_get_scan_results(&sp_out));
}

void app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_wifi_state_after_init);
    RUN_TEST(test_wifi_request_connect_returns_ESP_OK_when_connected);
    RUN_TEST(test_get_scan_result);
    UNITY_END();
}
