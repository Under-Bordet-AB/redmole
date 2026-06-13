#include "unity.h"

#include "board_i2c.h"

TEST_CASE("board I2C rejects invalid transaction arguments", "[board_i2c][unit]") {
    uint8_t data = 0;
    i2c_master_dev_handle_t device = NULL;

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, board_i2c_read(NULL, &data, sizeof(data)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, board_i2c_write(NULL, &data, sizeof(data)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, board_i2c_read_reg(NULL, 0x00U, &data, sizeof(data)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, board_i2c_add_device(0x80U, 0U, &device));
    TEST_ASSERT_NULL(device);
}
