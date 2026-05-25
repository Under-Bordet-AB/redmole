#include "http_client.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "HTTP TLS";

static const char *s_cert = NULL;
static http_client_tls_mode_t s_mode = HTTP_CLIENT_TLS_NONE;

/*
 * Stores the TLS mode and PEM certificate pointer for use by the request layer.
 * Called once from http_client_init.
 */
esp_err_t http_tls_init(http_client_tls_mode_t mode, const char *ca_cert_pem)
{
    s_mode = mode;

    if (s_mode == HTTP_CLIENT_TLS_NONE)
    {
        ESP_LOGD(TAG, "No TLS");
        return ESP_OK;
    }
    else if (s_mode == HTTP_CLIENT_TLS_BUNDLE)
    {
        ESP_LOGD(TAG, "Using built-in certificate bundle");
        return ESP_OK;
    }
    else if (s_mode == HTTP_CLIENT_TLS_CERT && ca_cert_pem != NULL)
    {
        s_cert = ca_cert_pem;
        ESP_LOGD(TAG, "Custom certificate set");
        return ESP_OK;
    }

    return ESP_ERR_INVALID_ARG;
}

/*
 * Returns the PEM certificate pointer when in CERT mode, otherwise NULL.
 * Passed as cert_pem in the esp_http_client config by the request layer.
 */
const char *http_tls_get_cert(void)
{
    return (s_mode == HTTP_CLIENT_TLS_CERT) ? s_cert : NULL;
}

/*
 * Returns the current TLS mode.
 * Used by the request layer to decide whether to attach the certificate bundle.
 */
http_client_tls_mode_t http_tls_get_mode(void)
{
    return s_mode;
}

/*
 * Clears TLS state. Called from http_client_deinit.
 */
void http_tls_deinit(void)
{
    ESP_LOGD(TAG, "Deinitializing");
    s_cert = NULL;
    s_mode = HTTP_CLIENT_TLS_NONE;
}
