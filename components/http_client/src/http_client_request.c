
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "HTTP Request";

/*
 * Context passed as user_data into the ESP HTTP client.
 * Tracks the caller-supplied buffer and how many bytes have been written
 * so the event handler can append each incoming chunk correctly.
 */
typedef struct
{
    char *buf;
    size_t buf_len;
    size_t written;
} response_ctx_t;

// ============= FORWARD DECLARATIONS ============= //

// Implemented in http_client_tls.c
const char *tls_get_cert(void);

// ================================================ //

/*
 * Event handler registered with esp_http_client_perform().
 * Fires once per received data chunk. Appends each chunk into the
 * caller's buffer via response_ctx_t, clamping to the available space
 * to prevent overflow if the response is larger than the buffer.
 */
static esp_err_t on_http_event(esp_http_client_event_t *event)
{
    switch(event->event_id) 
    {
        case HTTP_EVENT_ON_DATA:
        {
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA");

            response_ctx_t *ctx = (response_ctx_t *)event->user_data;
            size_t space_remaining = ctx->buf_len - ctx->written;
            size_t to_copy;

            // Clamp to remaining space so we never write past the end of the buffer
            if (event->data_len < space_remaining)
            {
                to_copy = event->data_len;
            } else
                to_copy = space_remaining;
        
            memcpy(ctx->buf + ctx->written, event->data, to_copy);
            ctx->written += to_copy;
            break;
        }
        default:
            break;
    }
    return ESP_OK;
}

/*
 * Sends a blocking GET request to url.
 * Response body is written into buf up to buf_len bytes via on_http_event.
 * Returns ESP_OK on success.
 * Returns ESP_FAIL if the client could not be initialized or the request failed.
 * Returns the HTTP status code (>= 400) if the server returned an error.
 */
esp_err_t request_get(const char* url, char* buf, size_t buf_len)
{
    response_ctx_t ctx =
    {
        .buf     = buf,
        .buf_len = buf_len,
        .written = 0
    };

    esp_http_client_config_t config =
    {
        .cert_pem      = tls_get_cert(),
        .event_handler = on_http_event,
        .url           = url,
        .user_data     = &ctx
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
        ESP_LOGE(TAG, "esp_http_client_perform failed");
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

/*
 * Sends a POST request
 */
// esp_err_t request_post(const char *url, const char *body, const char *content_type)
// {

// }