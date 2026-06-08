#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <stddef.h>
#include "esp_err.h"

/**
 * @file
 * @brief HTTP client module.
 *
 * Manages a periodic polling GET request and exposes a one-shot GET API.
 * The module owns a SPIRAM response buffer and a task_scheduler node that
 * fires at the configured poll interval while the network is up.
 *
 * Lifecycle: call http_client_init() once at startup, then drive network
 * state through http_client_notify_network_up() and
 * http_client_notify_network_down() from the network access controller.
 * Call http_client_deinit() to release resources.
 */

/**
 * @brief TLS mode used for outgoing HTTPS requests.
 */
typedef enum
{
    HTTP_CLIENT_TLS_BUNDLE, /*!< Use the ESP-IDF built-in CA certificate bundle. */
    HTTP_CLIENT_TLS_CERT,   /*!< Use a custom PEM certificate passed to http_client_init(). */
    HTTP_CLIENT_TLS_NONE    /*!< Disable TLS; use plain HTTP. */
} http_client_tls_mode_t;

/**
 * @brief Initialize the HTTP client module.
 *
 * Allocates the response buffer in SPIRAM, initializes TLS state, and
 * prepares the scheduler task node. Must be called once before any other
 * function in this module.
 *
 * @param mode        TLS mode for outgoing requests.
 * @param ca_cert_pem PEM-encoded CA certificate. Required when @p mode is
 *                    HTTP_CLIENT_TLS_CERT; pass NULL for other modes.
 * @return ESP_OK on success, ESP_ERR_NO_MEM if the response buffer
 *         allocation failed, or an esp_err_t from the TLS layer.
 * @see http_client_deinit
 */
esp_err_t http_client_init(http_client_tls_mode_t mode, const char *ca_cert_pem);

/**
 * @brief Deinitialize the HTTP client module and release resources.
 *
 * Frees the SPIRAM response buffer and clears TLS state. Call
 * http_client_notify_network_down() before this if the poll task is active
 * to ensure the scheduler node has been removed first.
 *
 * @see http_client_init
 */
void http_client_deinit(void);

/**
 * @brief Notify the module that the network is up.
 *
 * Enables outgoing requests and registers the periodic poll task with the
 * scheduler. Called by the network access controller on WiFi connect.
 *
 * @see http_client_notify_network_down
 */
void http_client_notify_network_up(void);

/**
 * @brief Notify the module that the network is down.
 *
 * Disables outgoing requests and removes the poll task from the scheduler.
 * Called by the network access controller on WiFi disconnect.
 *
 * @see http_client_notify_network_up
 */
void http_client_notify_network_down(void);

/**
 * @brief Get the most recently received response body.
 *
 * Returns a pointer into the module-owned SPIRAM buffer. The buffer is
 * overwritten on each poll cycle, so callers that need stable data must
 * copy it. The caller must not free or modify the returned pointer.
 *
 * @return Pointer to the null-terminated response body, or NULL if
 *         http_client_init() has not been called.
 */
const char *http_client_get_response(void);

/**
 * @brief Send a one-shot blocking GET request.
 *
 * Performs a synchronous HTTP GET to @p url and writes the response body
 * into @p buf as a null-terminated string, up to @p buf_len - 1 bytes.
 * Blocks the calling task until the request completes or fails. Bypasses
 * the internal poll buffer; the caller supplies their own buffer.
 *
 * @param url     URL to request. Must not be NULL.
 * @param buf     Caller-supplied output buffer. Must not be NULL.
 * @param buf_len Size of @p buf in bytes. Must be at least 1.
 * @return ESP_OK on success, ESP_FAIL if the network is not up or the
 *         request could not be sent, or an HTTP status code (>= 400) on a
 *         server error response.
 */
esp_err_t http_client_get(const char *url, char *buf, size_t buf_len);

// esp_err_t http_client_post(const char *url, const char *body, const char *content_type);


#endif // HTTP_CLIENT_H
