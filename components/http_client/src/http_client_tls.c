
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "TLS";
static const char *tls_cert = NULL;

/*
 * TLS initializer
 * Set ca_cert_pem to CA Certificate for TLS
 * Set ca_cert_pem to NULL if plain HTTP is wanted
 */
esp_err_t tls_init(const char *ca_cert_pem)
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
 * TLS certificate getter
 * Returns NULL if no certificate was set
 */
const char *tls_get_cert(void)
{
    return tls_cert; 
}

/*
 * TLS deinitializer
 * Sets certificate to NULL
 */
void tls_deinit(void)
{
    ESP_LOGD(TAG, "Deinitializing TLS");
    tls_cert = NULL;
}