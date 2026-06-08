#include "http_client.h"
#include "esp_log.h"
#include "esp_err.h"

/**
 * @file
 * @brief TLS configuration state for the HTTP client module.
 *
 * Stores the active TLS mode and optional PEM certificate pointer so that
 * http_client_request.c can retrieve them when building each request config.
 * State is set once by http_client_init() and cleared by http_client_deinit().
 */

static const char *TAG = "HTTP TLS";

static const char             *s_cert = NULL;
static http_client_tls_mode_t  s_mode = HTTP_CLIENT_TLS_NONE;

/**
 * @brief Store TLS configuration for use by the request layer.
 *
 * Called once from http_client_init(). Validates that a certificate is
 * provided when @p mode is HTTP_CLIENT_TLS_CERT.
 *
 * @param mode        TLS mode to apply to all outgoing requests.
 * @param ca_cert_pem PEM-encoded CA certificate. Required when @p mode is
 *                    HTTP_CLIENT_TLS_CERT; ignored otherwise.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if @p mode is
 *         HTTP_CLIENT_TLS_CERT and @p ca_cert_pem is NULL.
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

/**
 * @brief Get the stored PEM certificate pointer.
 *
 * Returns the certificate only when the active mode is HTTP_CLIENT_TLS_CERT;
 * otherwise returns NULL. Passed as @p cert_pem in the esp_http_client
 * config by the request layer.
 *
 * @return PEM certificate pointer, or NULL if not in CERT mode.
 */
const char *http_tls_get_cert(void)
{
    return (s_mode == HTTP_CLIENT_TLS_CERT) ? s_cert : NULL;
}

/**
 * @brief Get the active TLS mode.
 *
 * Used by the request layer to decide whether to attach the certificate
 * bundle via @p crt_bundle_attach.
 *
 * @return The current http_client_tls_mode_t value.
 */
http_client_tls_mode_t http_tls_get_mode(void)
{
    return s_mode;
}

/**
 * @brief Clear TLS state.
 *
 * Called from http_client_deinit(). Resets the certificate pointer and mode
 * to their default values.
 */
void http_tls_deinit(void)
{
    ESP_LOGD(TAG, "Deinitializing");
    s_cert = NULL;
    s_mode = HTTP_CLIENT_TLS_NONE;
}
