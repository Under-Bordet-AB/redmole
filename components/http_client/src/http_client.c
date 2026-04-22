#include <stdbool.h>
#include "http_client.h"
#include "esp_log.h"

static const char* TAG = "HTTP Client";
static bool network_up = false;

// ============= FORWARD DECLARATIONS ============= //

// Implemented in http_client_tls.c
esp_err_t tls_init(const char *ca_cert_pem);
const char *tls_get_cert(void);
void tls_deinit(void);

// Implemented in http_client_request.c
esp_err_t request_get(const char *url, char *buf, size_t buf_len);
esp_err_t request_post(const char *url, const char *body, const char *content_type);

// ================================================ //

void http_client_notify_network_up(void)
{
    ESP_LOGD(TAG, "Setting network_up to true");
    network_up = true;
}

void http_client_notify_network_down(void)
{
    ESP_LOGD(TAG, "Setting network_up to false");
    network_up = false;
}

esp_err_t http_client_init(const char* ca_cert_pem) 
{
    ESP_LOGD(TAG, "Initializing client");
    esp_err_t err = tls_init(ca_cert_pem);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "TLS init failed: %s", esp_err_to_name(err));
        return err;
    }
    
    return ESP_OK;
}

esp_err_t http_client_get(const char* url, char* buf, size_t buf_len) 
{
    if (!network_up)
    {
        ESP_LOGE(TAG, "Request failed, network is not up");
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Sending GET request");
    esp_err_t err = request_get(url, buf, buf_len);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Request failed: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t http_client_post(const char* url, const char* body, const char* content_type) 
{
    if (!network_up)
    {
        ESP_LOGE(TAG, "Request failed, network is not up");
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Sending POST request");
    esp_err_t err = request_post(url, body, content_type);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Request failed: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

void http_client_deinit(void) 
{
    ESP_LOGD(TAG, "Deinitializing client");
    tls_deinit();
}