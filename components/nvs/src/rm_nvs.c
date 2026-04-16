#include "rm_nvs.h"
#include <stdbool.h>
#include <string.h>
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char* TAG = "RM_NVS";
static bool s_is_initialized = false;
static const char* s_default_namespace = NULL;

typedef struct {
    uint32_t id;
    int16_t offset;
    char label[12];
} rm_nvs_test_blob_t;

static esp_err_t rm_nvs_open(nvs_open_mode_t mode, nvs_handle_t* out_handle) {
    if (out_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if ((!s_is_initialized) || (s_default_namespace == NULL)) {
        return ESP_ERR_INVALID_STATE;
    }

    return nvs_open(s_default_namespace, mode, out_handle);
}

esp_err_t rm_nvs_init(const char* default_namespace) {
    esp_err_t rv;

    if (default_namespace == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_is_initialized) {
        if ((s_default_namespace != NULL) && (strcmp(s_default_namespace, default_namespace) == 0)) {
            return ESP_OK;
        }

        return ESP_ERR_INVALID_STATE;
    }

    s_is_initialized = false;
    s_default_namespace = default_namespace;

    rv = nvs_flash_init();
    if ((rv == ESP_ERR_NVS_NO_FREE_PAGES) || (rv == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        rv = nvs_flash_init();
    }

    if (rv == ESP_OK) {
        s_is_initialized = true;
    }

    return rv;
}

esp_err_t rm_nvs_deinit(void) {
    s_is_initialized = false;
    s_default_namespace = NULL;
    return ESP_OK;
}

esp_err_t rm_nvs_set_u8(const char* key, uint8_t value) {
    esp_err_t rv;
    nvs_handle_t handle;

    if (key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    rv = rm_nvs_open(NVS_READWRITE, &handle);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = nvs_set_u8(handle, key, value);
    if (rv == ESP_OK) {
        rv = nvs_commit(handle);
    }

    nvs_close(handle);
    return rv;
}

esp_err_t rm_nvs_get_u8(const char* key, uint8_t* out_value) {
    esp_err_t rv;
    nvs_handle_t handle;

    if ((key == NULL) || (out_value == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    rv = rm_nvs_open(NVS_READONLY, &handle);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = nvs_get_u8(handle, key, out_value);
    nvs_close(handle);
    return rv;
}

esp_err_t rm_nvs_set_i8(const char* key, int8_t value) {
    esp_err_t rv;
    nvs_handle_t handle;

    if (key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    rv = rm_nvs_open(NVS_READWRITE, &handle);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = nvs_set_i8(handle, key, value);
    if (rv == ESP_OK) {
        rv = nvs_commit(handle);
    }

    nvs_close(handle);
    return rv;
}

esp_err_t rm_nvs_get_i8(const char* key, int8_t* out_value) {
    esp_err_t rv;
    nvs_handle_t handle;

    if ((key == NULL) || (out_value == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    rv = rm_nvs_open(NVS_READONLY, &handle);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = nvs_get_i8(handle, key, out_value);
    nvs_close(handle);
    return rv;
}

esp_err_t rm_nvs_set_u16(const char* key, uint16_t value) {
    esp_err_t rv;
    nvs_handle_t handle;

    if (key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    rv = rm_nvs_open(NVS_READWRITE, &handle);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = nvs_set_u16(handle, key, value);
    if (rv == ESP_OK) {
        rv = nvs_commit(handle);
    }

    nvs_close(handle);
    return rv;
}

esp_err_t rm_nvs_get_u16(const char* key, uint16_t* out_value) {
    esp_err_t rv;
    nvs_handle_t handle;

    if ((key == NULL) || (out_value == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    rv = rm_nvs_open(NVS_READONLY, &handle);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = nvs_get_u16(handle, key, out_value);
    nvs_close(handle);
    return rv;
}

esp_err_t rm_nvs_set_i16(const char* key, int16_t value) {
    esp_err_t rv;
    nvs_handle_t handle;

    if (key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    rv = rm_nvs_open(NVS_READWRITE, &handle);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = nvs_set_i16(handle, key, value);
    if (rv == ESP_OK) {
        rv = nvs_commit(handle);
    }

    nvs_close(handle);
    return rv;
}

esp_err_t rm_nvs_get_i16(const char* key, int16_t* out_value) {
    esp_err_t rv;
    nvs_handle_t handle;

    if ((key == NULL) || (out_value == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    rv = rm_nvs_open(NVS_READONLY, &handle);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = nvs_get_i16(handle, key, out_value);
    nvs_close(handle);
    return rv;
}

esp_err_t rm_nvs_set_u32(const char* key, uint32_t value) {
    esp_err_t rv;
    nvs_handle_t handle;

    if (key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    rv = rm_nvs_open(NVS_READWRITE, &handle);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = nvs_set_u32(handle, key, value);
    if (rv == ESP_OK) {
        rv = nvs_commit(handle);
    }

    nvs_close(handle);
    return rv;
}

esp_err_t rm_nvs_get_u32(const char* key, uint32_t* out_value) {
    esp_err_t rv;
    nvs_handle_t handle;

    if ((key == NULL) || (out_value == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    rv = rm_nvs_open(NVS_READONLY, &handle);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = nvs_get_u32(handle, key, out_value);
    nvs_close(handle);
    return rv;
}

esp_err_t rm_nvs_set_i32(const char* key, int32_t value) {
    esp_err_t rv;
    nvs_handle_t handle;

    if (key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    rv = rm_nvs_open(NVS_READWRITE, &handle);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = nvs_set_i32(handle, key, value);
    if (rv == ESP_OK) {
        rv = nvs_commit(handle);
    }

    nvs_close(handle);
    return rv;
}

esp_err_t rm_nvs_get_i32(const char* key, int32_t* out_value) {
    esp_err_t rv;
    nvs_handle_t handle;

    if ((key == NULL) || (out_value == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    rv = rm_nvs_open(NVS_READONLY, &handle);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = nvs_get_i32(handle, key, out_value);
    nvs_close(handle);
    return rv;
}

esp_err_t rm_nvs_set_u64(const char* key, uint64_t value) {
    esp_err_t rv;
    nvs_handle_t handle;

    if (key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    rv = rm_nvs_open(NVS_READWRITE, &handle);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = nvs_set_u64(handle, key, value);
    if (rv == ESP_OK) {
        rv = nvs_commit(handle);
    }

    nvs_close(handle);
    return rv;
}

esp_err_t rm_nvs_get_u64(const char* key, uint64_t* out_value) {
    esp_err_t rv;
    nvs_handle_t handle;

    if ((key == NULL) || (out_value == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    rv = rm_nvs_open(NVS_READONLY, &handle);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = nvs_get_u64(handle, key, out_value);
    nvs_close(handle);
    return rv;
}

esp_err_t rm_nvs_set_i64(const char* key, int64_t value) {
    esp_err_t rv;
    nvs_handle_t handle;

    if (key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    rv = rm_nvs_open(NVS_READWRITE, &handle);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = nvs_set_i64(handle, key, value);
    if (rv == ESP_OK) {
        rv = nvs_commit(handle);
    }

    nvs_close(handle);
    return rv;
}

esp_err_t rm_nvs_get_i64(const char* key, int64_t* out_value) {
    esp_err_t rv;
    nvs_handle_t handle;

    if ((key == NULL) || (out_value == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    rv = rm_nvs_open(NVS_READONLY, &handle);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = nvs_get_i64(handle, key, out_value);
    nvs_close(handle);
    return rv;
}

esp_err_t rm_nvs_set_str(const char* key, const char* value) {
    esp_err_t rv;
    nvs_handle_t handle;

    if ((key == NULL) || (value == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    rv = rm_nvs_open(NVS_READWRITE, &handle);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = nvs_set_str(handle, key, value);
    if (rv == ESP_OK) {
        rv = nvs_commit(handle);
    }

    nvs_close(handle);
    return rv;
}

esp_err_t rm_nvs_get_str(const char* key, char* buffer, size_t* length) {
    esp_err_t rv;
    nvs_handle_t handle;

    if ((key == NULL) || (length == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    rv = rm_nvs_open(NVS_READONLY, &handle);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = nvs_get_str(handle, key, buffer, length);
    nvs_close(handle);
    return rv;
}

esp_err_t rm_nvs_set_blob(const char* key, const void* value, size_t length) {
    esp_err_t rv;
    nvs_handle_t handle;

    if ((key == NULL) || (value == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    rv = rm_nvs_open(NVS_READWRITE, &handle);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = nvs_set_blob(handle, key, value, length);
    if (rv == ESP_OK) {
        rv = nvs_commit(handle);
    }

    nvs_close(handle);
    return rv;
}

esp_err_t rm_nvs_get_blob(const char* key, void* buffer, size_t* length) {
    esp_err_t rv;
    nvs_handle_t handle;

    if ((key == NULL) || (length == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    rv = rm_nvs_open(NVS_READONLY, &handle);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = nvs_get_blob(handle, key, buffer, length);
    nvs_close(handle);
    return rv;
}

esp_err_t rm_nvs_erase_key(const char* key) {
    esp_err_t rv;
    nvs_handle_t handle;

    if (key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    rv = rm_nvs_open(NVS_READWRITE, &handle);
    if (rv != ESP_OK) {
        return rv;
    }

    rv = nvs_erase_key(handle, key);
    if (rv == ESP_OK) {
        rv = nvs_commit(handle);
    }

    nvs_close(handle);
    return rv;
}

esp_err_t rm_nvs_self_test(void) {
    esp_err_t rv;
    uint8_t u8_value = 0;
    int8_t i8_value = 0;
    uint16_t u16_value = 0;
    int16_t i16_value = 0;
    uint32_t u32_value = 0;
    int32_t i32_value = 0;
    uint64_t u64_value = 0;
    int64_t i64_value = 0;
    char username[32] = {0};
    size_t username_len = sizeof(username);
    rm_nvs_test_blob_t blob_in = {0x12345678u, -123, "redmole"};
    rm_nvs_test_blob_t blob_out = {0};
    size_t blob_len = sizeof(blob_out);

    ESP_LOGI(TAG, "Starting NVS self-test");

    rv = rm_nvs_set_u8("test_u8", 42u);
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "rm_nvs_set_u8 failed: %s", esp_err_to_name(rv));
        return rv;
    }

    rv = rm_nvs_get_u8("test_u8", &u8_value);
    if ((rv != ESP_OK) || (u8_value != 42u)) {
        ESP_LOGE(TAG, "rm_nvs_get_u8 failed: %s", esp_err_to_name((rv != ESP_OK) ? rv : ESP_FAIL));
        return (rv != ESP_OK) ? rv : ESP_FAIL;
    }

    rv = rm_nvs_set_i8("test_i8", -42);
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "rm_nvs_set_i8 failed: %s", esp_err_to_name(rv));
        return rv;
    }

    rv = rm_nvs_get_i8("test_i8", &i8_value);
    if ((rv != ESP_OK) || (i8_value != -42)) {
        ESP_LOGE(TAG, "rm_nvs_get_i8 failed: %s", esp_err_to_name((rv != ESP_OK) ? rv : ESP_FAIL));
        return (rv != ESP_OK) ? rv : ESP_FAIL;
    }

    rv = rm_nvs_set_u16("test_u16", 4242u);
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "rm_nvs_set_u16 failed: %s", esp_err_to_name(rv));
        return rv;
    }

    rv = rm_nvs_get_u16("test_u16", &u16_value);
    if ((rv != ESP_OK) || (u16_value != 4242u)) {
        ESP_LOGE(TAG, "rm_nvs_get_u16 failed: %s", esp_err_to_name((rv != ESP_OK) ? rv : ESP_FAIL));
        return (rv != ESP_OK) ? rv : ESP_FAIL;
    }

    rv = rm_nvs_set_i16("test_i16", -4242);
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "rm_nvs_set_i16 failed: %s", esp_err_to_name(rv));
        return rv;
    }

    rv = rm_nvs_get_i16("test_i16", &i16_value);
    if ((rv != ESP_OK) || (i16_value != -4242)) {
        ESP_LOGE(TAG, "rm_nvs_get_i16 failed: %s", esp_err_to_name((rv != ESP_OK) ? rv : ESP_FAIL));
        return (rv != ESP_OK) ? rv : ESP_FAIL;
    }

    rv = rm_nvs_set_u32("test_u32", 424242u);
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "rm_nvs_set_u32 failed: %s", esp_err_to_name(rv));
        return rv;
    }

    rv = rm_nvs_get_u32("test_u32", &u32_value);
    if ((rv != ESP_OK) || (u32_value != 424242u)) {
        ESP_LOGE(TAG, "rm_nvs_get_u32 failed: %s", esp_err_to_name((rv != ESP_OK) ? rv : ESP_FAIL));
        return (rv != ESP_OK) ? rv : ESP_FAIL;
    }

    rv = rm_nvs_set_i32("test_i32", -424242);
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "rm_nvs_set_i32 failed: %s", esp_err_to_name(rv));
        return rv;
    }

    rv = rm_nvs_get_i32("test_i32", &i32_value);
    if ((rv != ESP_OK) || (i32_value != -424242)) {
        ESP_LOGE(TAG, "rm_nvs_get_i32 failed: %s", esp_err_to_name((rv != ESP_OK) ? rv : ESP_FAIL));
        return (rv != ESP_OK) ? rv : ESP_FAIL;
    }

    rv = rm_nvs_set_u64("test_u64", 4242424242ULL);
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "rm_nvs_set_u64 failed: %s", esp_err_to_name(rv));
        return rv;
    }

    rv = rm_nvs_get_u64("test_u64", &u64_value);
    if ((rv != ESP_OK) || (u64_value != 4242424242ULL)) {
        ESP_LOGE(TAG, "rm_nvs_get_u64 failed: %s", esp_err_to_name((rv != ESP_OK) ? rv : ESP_FAIL));
        return (rv != ESP_OK) ? rv : ESP_FAIL;
    }

    rv = rm_nvs_set_i64("test_i64", -4242424242LL);
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "rm_nvs_set_i64 failed: %s", esp_err_to_name(rv));
        return rv;
    }

    rv = rm_nvs_get_i64("test_i64", &i64_value);
    if ((rv != ESP_OK) || (i64_value != -4242424242LL)) {
        ESP_LOGE(TAG, "rm_nvs_get_i64 failed: %s", esp_err_to_name((rv != ESP_OK) ? rv : ESP_FAIL));
        return (rv != ESP_OK) ? rv : ESP_FAIL;
    }

    rv = rm_nvs_set_str("username", "redmole");
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "rm_nvs_set_str failed: %s", esp_err_to_name(rv));
        return rv;
    }

    rv = rm_nvs_get_str("username", username, &username_len);
    if ((rv != ESP_OK) || (strcmp(username, "redmole") != 0)) {
        ESP_LOGE(TAG, "rm_nvs_get_str failed: %s", esp_err_to_name((rv != ESP_OK) ? rv : ESP_FAIL));
        return (rv != ESP_OK) ? rv : ESP_FAIL;
    }

    rv = rm_nvs_set_blob("test_blob", &blob_in, sizeof(blob_in));
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "rm_nvs_set_blob failed: %s", esp_err_to_name(rv));
        return rv;
    }

    rv = rm_nvs_get_blob("test_blob", &blob_out, &blob_len);
    if ((rv != ESP_OK) || (blob_len != sizeof(blob_out)) ||
        (memcmp(&blob_in, &blob_out, sizeof(blob_in)) != 0)) {
        ESP_LOGE(TAG, "rm_nvs_get_blob failed: %s", esp_err_to_name((rv != ESP_OK) ? rv : ESP_FAIL));
        return (rv != ESP_OK) ? rv : ESP_FAIL;
    }

    rv = rm_nvs_erase_key("test_u8");
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "rm_nvs_erase_key failed: %s", esp_err_to_name(rv));
        return rv;
    }

    rv = rm_nvs_get_u8("test_u8", &u8_value);
    if (rv != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "rm_nvs_erase_key verification failed: %s",
                 esp_err_to_name((rv != ESP_OK) ? rv : ESP_FAIL));
        return (rv != ESP_OK) ? rv : ESP_FAIL;
    }

    ESP_LOGI(TAG, "Stored username: %s", username);
    ESP_LOGI(TAG, "NVS self-test passed");
    return ESP_OK;
}
