#include "unity.h"

#include "nvs.h"
#include "rm_nvs.h"

TEST_CASE("NVS stores reads and erases a u8 value", "[nvs][integration]") {
    static const char* test_key = "unity_u8";
    uint8_t stored_value = 0U;

    TEST_ASSERT_EQUAL(ESP_OK, rm_nvs_init("unity_test"));

    esp_err_t erase_result = rm_nvs_erase_key(test_key);
    TEST_ASSERT_TRUE(erase_result == ESP_OK || erase_result == ESP_ERR_NVS_NOT_FOUND);

    TEST_ASSERT_EQUAL(ESP_OK, rm_nvs_set_u8(test_key, 42U));
    TEST_ASSERT_EQUAL(ESP_OK, rm_nvs_get_u8(test_key, &stored_value));
    TEST_ASSERT_EQUAL_UINT8(42U, stored_value);

    TEST_ASSERT_EQUAL(ESP_OK, rm_nvs_erase_key(test_key));
    TEST_ASSERT_EQUAL(ESP_ERR_NVS_NOT_FOUND, rm_nvs_get_u8(test_key, &stored_value));
    TEST_ASSERT_EQUAL(ESP_OK, rm_nvs_deinit());
}
