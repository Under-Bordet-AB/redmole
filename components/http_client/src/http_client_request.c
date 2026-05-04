#include <string.h>
#include "http_client.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_crt_bundle.h"

static const char *TAG = "HTTP Request";

/*
 * Tracks the write position into the caller-supplied buffer across
 * multiple HTTP_EVENT_ON_DATA chunks within a single request.
 */
typedef struct
{
    char   *buf;
    size_t  buf_len;
    size_t  written;
} response_ctx_t;

// Implemented in http_client_tls.c
const char             *http_tls_get_cert(void);
http_client_tls_mode_t  http_tls_get_mode(void);

/*
 * ESP HTTP client event callback.
 * Appends each incoming data chunk into the buffer tracked by response_ctx_t.
 * One byte is always reserved for a null terminator, so at most
 * buf_len - 1 bytes of response data are stored.
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

/*
 * Sends a blocking GET request to url.
 * The response body is written into buf as a null-terminated string,
 * up to buf_len - 1 bytes.
 * Returns ESP_OK on success.
 * Returns ESP_FAIL if the client could not be initialized or the transport failed.
 * Returns the HTTP status code as an error value for responses >= 400.
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