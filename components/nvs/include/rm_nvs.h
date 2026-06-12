#ifndef RM_NVS_H
#define RM_NVS_H

/**
 * @file
 * @brief Single-instance persistent-storage API for the application namespace.
 *
 * The module initializes the default ESP-IDF NVS partition, owns one configured
 * namespace, and commits each successful write or erase immediately.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

/**
 * @brief Initialize NVS and select the application namespace.
 *
 * The namespace is validated and copied into module-owned storage. Repeated
 * calls with the same namespace return ESP_OK; a different namespace returns
 * ESP_ERR_INVALID_STATE.
 *
 * @warning Recovery from ESP_ERR_NVS_NO_FREE_PAGES or
 *          ESP_ERR_NVS_NEW_VERSION_FOUND erases the default NVS partition.
 *
 * @param default_namespace Non-empty namespace no longer than 15 characters;
 *                          must not be NULL.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_init(const char* default_namespace);

/**
 * @brief Disable the wrapper without erasing stored data or deinitializing NVS flash.
 *
 * Repeated calls are safe. Other ESP-IDF components may continue using the
 * default NVS partition after this function returns.
 *
 * @return ESP_OK.
 */
esp_err_t rm_nvs_deinit(void);

/**
 * @brief Write and immediately commit an unsigned 8-bit value.
 * @param key NVS key, must not be NULL.
 * @param value Value to store.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_set_u8(const char* key, uint8_t value);

/**
 * @brief Read an unsigned 8-bit value.
 * @param key NVS key, must not be NULL.
 * @param out_value Caller-owned output, must not be NULL.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_get_u8(const char* key, uint8_t* out_value);

/**
 * @brief Write and commit a signed 8-bit value.
 * @param key NVS key, must not be NULL.
 * @param value Value to store.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_set_i8(const char* key, int8_t value);
/**
 * @brief Read a signed 8-bit value.
 * @param key NVS key, must not be NULL.
 * @param out_value Caller-owned output, must not be NULL.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_get_i8(const char* key, int8_t* out_value);
/**
 * @brief Write and commit an unsigned 16-bit value.
 * @param key NVS key, must not be NULL.
 * @param value Value to store.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_set_u16(const char* key, uint16_t value);
/**
 * @brief Read an unsigned 16-bit value.
 * @param key NVS key, must not be NULL.
 * @param out_value Caller-owned output, must not be NULL.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_get_u16(const char* key, uint16_t* out_value);
/**
 * @brief Write and commit a signed 16-bit value.
 * @param key NVS key, must not be NULL.
 * @param value Value to store.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_set_i16(const char* key, int16_t value);
/**
 * @brief Read a signed 16-bit value.
 * @param key NVS key, must not be NULL.
 * @param out_value Caller-owned output, must not be NULL.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_get_i16(const char* key, int16_t* out_value);
/**
 * @brief Write and commit an unsigned 32-bit value.
 * @param key NVS key, must not be NULL.
 * @param value Value to store.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_set_u32(const char* key, uint32_t value);
/**
 * @brief Read an unsigned 32-bit value.
 * @param key NVS key, must not be NULL.
 * @param out_value Caller-owned output, must not be NULL.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_get_u32(const char* key, uint32_t* out_value);
/**
 * @brief Write and commit a signed 32-bit value.
 * @param key NVS key, must not be NULL.
 * @param value Value to store.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_set_i32(const char* key, int32_t value);
/**
 * @brief Read a signed 32-bit value.
 * @param key NVS key, must not be NULL.
 * @param out_value Caller-owned output, must not be NULL.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_get_i32(const char* key, int32_t* out_value);
/**
 * @brief Write and commit an unsigned 64-bit value.
 * @param key NVS key, must not be NULL.
 * @param value Value to store.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_set_u64(const char* key, uint64_t value);
/**
 * @brief Read an unsigned 64-bit value.
 * @param key NVS key, must not be NULL.
 * @param out_value Caller-owned output, must not be NULL.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_get_u64(const char* key, uint64_t* out_value);
/**
 * @brief Write and commit a signed 64-bit value.
 * @param key NVS key, must not be NULL.
 * @param value Value to store.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_set_i64(const char* key, int64_t value);
/**
 * @brief Read a signed 64-bit value.
 * @param key NVS key, must not be NULL.
 * @param out_value Caller-owned output, must not be NULL.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_get_i64(const char* key, int64_t* out_value);

/**
 * @brief Write and immediately commit a null-terminated string.
 * @param key NVS key, must not be NULL.
 * @param value Null-terminated string, must not be NULL.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_set_str(const char* key, const char* value);

/**
 * @brief Read a null-terminated string or query its required size.
 * @param key NVS key, must not be NULL.
 * @param buffer Caller-owned output, or NULL to query required size.
 * @param length Input/output buffer size including the null terminator; must not be NULL.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_get_str(const char* key, char* buffer, size_t* length);

/**
 * @brief Write and immediately commit a binary blob.
 * @param key NVS key, must not be NULL.
 * @param value Blob bytes, must not be NULL.
 * @param length Number of bytes to store.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_set_blob(const char* key, const void* value, size_t length);

/**
 * @brief Read a binary blob or query its required size.
 * @param key NVS key, must not be NULL.
 * @param buffer Caller-owned output, or NULL to query required size.
 * @param length Input/output buffer size in bytes; must not be NULL.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_get_blob(const char* key, void* buffer, size_t* length);

/**
 * @brief Check whether a key exists in the configured namespace.
 * @param key NVS key, must not be NULL.
 * @param out_exists Set to true when the key exists; must not be NULL.
 * @return ESP_OK on a completed check, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_key_exists(const char* key, bool* out_exists);

/**
 * @brief Erase and immediately commit one key.
 * @param key NVS key, must not be NULL.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_erase_key(const char* key);

/**
 * @brief Exercise every supported value type using reserved test keys.
 *
 * The test removes stale test keys before starting and cleans them after a
 * successful run. It modifies flash and logs progress.
 *
 * @return ESP_OK on success, otherwise the first failing ESP-IDF error code.
 */
esp_err_t rm_nvs_self_test(void);

#endif // RM_NVS_H
