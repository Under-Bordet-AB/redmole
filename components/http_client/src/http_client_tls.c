#include "http_client.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "TLS";
static const char *tls_cert = NULL;
static http_client_tls_mode_t tls_mode = HTTP_CLIENT_TLS_NONE;

/*
 * TLS initializer.
 * Stores the PEM-encoded CA certificate pointer used by request_get/post
 * when setting up the esp_http_client TLS context.
 * Pass NULL to disable TLS and use plain HTTP.
 */
esp_err_t _tls_init(http_client_tls_mode_t mode, const char *ca_cert_pem)
{
    ESP_LOGD(TAG, "Initializing TLS");
    tls_mode = mode;

    if (tls_mode == HTTP_CLIENT_TLS_NONE)
    {
        // Plain HTTP
        ESP_LOGD(TAG, "HTTP_CLIENT_TLS_NONE. No TLS");
        return ESP_OK;
    }
    else if (tls_mode == HTTP_CLIENT_TLS_BUNDLE)
    {
        // Use default CA certificate
        ESP_LOGD(TAG, "HTTP_CLIENT_TLS_BUNDLE. Using default bundle");
        return ESP_OK;
    }
    else if (tls_mode == HTTP_CLIENT_TLS_CERT && ca_cert_pem != NULL)
    {
        // Use custom CA certificate
        tls_cert = ca_cert_pem;
        ESP_LOGD(TAG, "Certificate successfully set");
        return ESP_OK;
    }
    return ESP_ERR_INVALID_ARG;
}

/*
 * Returns the stored CA certificate PEM string.
 * Returns NULL if no certificate was set (plain HTTP mode or built-in bundle).
 * Called by request_get/post to populate cert_pem in the client config.
 */
const char *tls_get_cert(void)
{
    if (tls_mode == HTTP_CLIENT_TLS_CERT)
    {
        return tls_cert;
    } 
    return NULL;
}

http_client_tls_mode_t tls_get_mode(void)
{
    return tls_mode;
}

/*
 * Clears the stored certificate pointer.
 * Called from http_client_deinit().
 */
void _tls_deinit(void)
{
    ESP_LOGD(TAG, "Deinitializing TLS");
    tls_cert = NULL;
    tls_mode = HTTP_CLIENT_TLS_NONE;
}
