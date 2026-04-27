
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "TLS";
static const char *tls_cert = NULL;

/*
 * TLS initializer.
 * Stores the PEM-encoded CA certificate pointer used by request_get/post
 * when setting up the esp_http_client TLS context.
 * Pass NULL to disable TLS and use plain HTTP.
 */
esp_err_t _tls_init(const char *ca_cert_pem)
{
    ESP_LOGD(TAG, "Initializing TLS");

    if (ca_cert_pem == NULL)
    {
        // NULL is not an error, it means plain HTTP
        ESP_LOGD(TAG, "ca_cert_pem is NULL. No TLS");
        return ESP_OK;
    }

    tls_cert = ca_cert_pem;
    ESP_LOGD(TAG, "Certificate successfully set");
    return ESP_OK;
}

/*
 * Returns the stored CA certificate PEM string.
 * Returns NULL if no certificate was set (plain HTTP mode).
 * Called by request_get/post to populate cert_pem in the client config.
 */
const char *tls_get_cert(void)
{
    return tls_cert;
}

/*
 * Clears the stored certificate pointer.
 * Called from http_client_deinit().
 */
void _tls_deinit(void)
{
    ESP_LOGD(TAG, "Deinitializing TLS");
    tls_cert = NULL;
}
