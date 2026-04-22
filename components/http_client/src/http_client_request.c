
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "HTTP Request";

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

static esp_err_t on_http_event(esp_http_client_event_t *event)
{
    switch(event->event_id) 
    {
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA");

            break;
        default:
            break;
    }
    return ESP_OK;
}

// esp_err_t request_get(const char* url, char* buf, size_t buf_len)
// {

// }

// esp_err_t request_post(const char *url, const char *body, const char *content_type)
// {

// }