#ifndef RM_NVS_H
#define RM_NVS_H

/*
 * Single-instance NVS wrapper for app-wide persistent storage.
 * The module owns all internal state and exposes no state pointers.
 * Initialize once during startup, then use the API functions directly.
 *
 * Usage notes:
 * - NVS stores key/value pairs inside a namespace.
 * - This wrapper uses one default namespace set during rm_nvs_init().
 * - rm_nvs_init() is idempotent for the same namespace.
 * - Re-initializing with a different namespace returns ESP_ERR_INVALID_STATE.
 * - Key names and namespace names must stay short (ESP-IDF limit is 15 chars).
 * - Writes are committed immediately in each setter.
 * - This is not a task module and does not need its own loop.
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Initialize the module and set the default namespace.
 *
 * Must be called before any read or write operation.
 * Repeated calls with the same namespace return ESP_OK.
 * Repeated calls with a different namespace return ESP_ERR_INVALID_STATE.
 *
 * @param default_namespace Namespace used by all subsequent operations.
 *
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_init(const char* default_namespace);

/**
 * @brief Reset the module's internal state.
 *
 * This only clears the wrapper's internal state. It does not erase stored data.
 *
 * @return ESP_OK.
 */
esp_err_t rm_nvs_deinit(void);

/**
 * @brief Integer accessors for the default namespace.
 *
 * Setters write and commit immediately.
 * Getters read the stored value into the provided output pointer.
 * All functions return ESP_OK on success or an ESP-IDF error code on failure.
 */
esp_err_t rm_nvs_set_u8(const char* key, uint8_t value);
esp_err_t rm_nvs_get_u8(const char* key, uint8_t* out_value);
esp_err_t rm_nvs_set_i8(const char* key, int8_t value);
esp_err_t rm_nvs_get_i8(const char* key, int8_t* out_value);
esp_err_t rm_nvs_set_u16(const char* key, uint16_t value);
esp_err_t rm_nvs_get_u16(const char* key, uint16_t* out_value);
esp_err_t rm_nvs_set_i16(const char* key, int16_t value);
esp_err_t rm_nvs_get_i16(const char* key, int16_t* out_value);
esp_err_t rm_nvs_set_u32(const char* key, uint32_t value);
esp_err_t rm_nvs_get_u32(const char* key, uint32_t* out_value);
esp_err_t rm_nvs_set_i32(const char* key, int32_t value);
esp_err_t rm_nvs_get_i32(const char* key, int32_t* out_value);
esp_err_t rm_nvs_set_u64(const char* key, uint64_t value);
esp_err_t rm_nvs_get_u64(const char* key, uint64_t* out_value);
esp_err_t rm_nvs_set_i64(const char* key, int64_t value);
esp_err_t rm_nvs_get_i64(const char* key, int64_t* out_value);

/**
 * @brief String accessors for the default namespace.
 *
 * Setters write and commit immediately.
 * For reads, @p length is input/output and includes the null terminator.
 * @p buffer may be NULL when querying required size.
 */
esp_err_t rm_nvs_set_str(const char* key, const char* value);
esp_err_t rm_nvs_get_str(const char* key, char* buffer, size_t* length);

/**
 * @brief Blob accessors for the default namespace.
 *
 * Setters write and commit immediately.
 * For reads, @p length is input/output.
 * @p buffer may be NULL when querying required size.
 */
esp_err_t rm_nvs_set_blob(const char* key, const void* value, size_t length);
esp_err_t rm_nvs_get_blob(const char* key, void* buffer, size_t* length);

/**
 * @brief Check if a key exists in the default namespace.
 *
 * @param key NVS key to look up.
 * @param out_exists Set to true when the key exists, otherwise false.
 *
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_key_exists(const char* key, bool* out_exists);

/**
 * @brief Erase a single key from the default namespace.
 *
 * @param key NVS key to erase.
 *
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t rm_nvs_erase_key(const char* key);

/**
 * @brief Run a basic API self-test against the current namespace.
 *
 * This verifies all supported read/write helpers and key erase behavior.
 *
 * @return ESP_OK on success, otherwise the first failing ESP-IDF error code.
 */
esp_err_t rm_nvs_self_test(void);

#endif // RM_NVS_H
