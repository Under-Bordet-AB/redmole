#include <string.h>
#include "http_client.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_crt_bundle.h"

/**
 * @file
 * @brief HTTP GET request implementation.
 *
 * Owns the esp_http_client lifecycle for a single blocking request.
 * Chunked response data is accumulated into a caller-supplied buffer via
 * the event callback. TLS settings are read from http_client_tls.c.
 */

static const char *TAG = "HTTP Request";

/**
 * @brief Per-request buffer tracking context passed to the event callback.
 *
 * Tracks write position across multiple HTTP_EVENT_ON_DATA chunks. One byte
 * of @p buf_len is always reserved for the null terminator.
 */
typedef struct
{
    char   *buf;     /*!< Caller-supplied output buffer. */
    size_t  buf_len; /*!< Total capacity of @p buf in bytes. */
    size_t  written; /*!< Bytes written so far, not counting the null terminator. */
} response_ctx_t;

// Implemented in http_client_tls.c
const char             *http_tls_get_cert(void);
http_client_tls_mode_t  http_tls_get_mode(void);

/**
 * @brief esp_http_client event callback; accumulates incoming data chunks.
 *
 * Appends each HTTP_EVENT_ON_DATA chunk into the buffer tracked by
 * response_ctx_t, always maintaining a null terminator at the write
 * position. Excess data beyond buf_len - 1 bytes is silently dropped.
 *
 * @param event esp_http_client event. Only HTTP_EVENT_ON_DATA is processed.
 * @return ESP_OK always.
 */
static esp_err_t on_http_event(esp_http_client_event_t *event)
{
    if (event->event_id != HTTP_EVENT_ON_DATA)
    {
        return ESP_OK;
    }

    response_ctx_t *ctx = (response_ctx_t *)event->user_data;
    size_t space   = ctx->buf_len - ctx->written - 1;
    size_t to_copy = (event->data_len < space) ? event->data_len : space;

    memcpy(ctx->buf + ctx->written, event->data, to_copy);
    ctx->written += to_copy;
    ctx->buf[ctx->written] = '\0';

    return ESP_OK;
}

/**
 * @brief Send a blocking GET request and collect the response body.
 *
 * Configures and performs a synchronous esp_http_client GET. TLS settings
 * are read from the http_client_tls module. The response body is written
 * into @p buf as a null-terminated string, up to @p buf_len - 1 bytes.
 * Excess data is silently dropped.
 *
 * @param url     URL to request. Must not be NULL.
 * @param buf     Caller-supplied output buffer. Must not be NULL.
 * @param buf_len Size of @p buf in bytes. Must be at least 1.
 * @return ESP_OK on success, ESP_FAIL if the client could not be initialized
 *         or the transport failed, or the HTTP status code (>= 400) on a
 *         server error response.
 */
esp_err_t http_request_get(const char *url, char *buf, size_t buf_len)
{
    response_ctx_t ctx =
    {
        .buf     = buf,
        .buf_len = buf_len,
        .written = 0
    };

    http_client_tls_mode_t mode = http_tls_get_mode();

    esp_http_client_config_t config =
    {
        .url               = url,
        .user_data         = &ctx,
        .event_handler     = on_http_event,
        .cert_pem          = http_tls_get_cert(),
        .crt_bundle_attach = (mode == HTTP_CLIENT_TLS_BUNDLE) ? esp_crt_bundle_attach : NULL
    };

    buf[0] = '\0';

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL)
    {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }

    esp_err_t res = esp_http_client_perform(client);
    if (res != ESP_OK)
    {
        ESP_LOGE(TAG, "HTTP perform failed: %s", esp_err_to_name(res));
        goto cleanup;
    }

    int status = esp_http_client_get_status_code(client);
    if (status >= 400)
    {
        ESP_LOGE(TAG, "HTTP error status: %d", status);
        res = status;
        goto cleanup;
    }

cleanup:
    esp_http_client_cleanup(client);
    return res;
}

// esp_err_t http_request_post(const char *url, const char *body, const char *content_type)
// {
//
// }
