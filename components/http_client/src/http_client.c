#include <stdbool.h>
#include <string.h>
#include "http_client.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "task_scheduler.h"

/**
 * @file
 * @brief HTTP client module state and lifecycle management.
 *
 * Owns the module-level context, SPIRAM response buffer, and the scheduler
 * task node used for periodic polling. Network lifecycle is driven by the
 * NAC via http_client_notify_network_up() and
 * http_client_notify_network_down().
 */

/**
 * @brief Module-level state for the HTTP client.
 *
 * @p task_node is embedded directly so the scheduler does not need to
 * allocate it. @p network_up is set by the NAC callbacks and gates all
 * outgoing requests.
 */
typedef struct
{
    task_node_t task_node; /*!< Scheduler node; embedded to avoid heap allocation. */
    bool network_up;       /*!< True when the network is available for requests. */
} http_client_ctx_t;

static http_client_ctx_t s_client = {0};
static char *s_response_buf = NULL;
static const char *TAG = "HTTP Client";

// Implemented in http_client_tls.c
esp_err_t http_tls_init(http_client_tls_mode_t mode, const char *ca_cert_pem);
void      http_tls_deinit(void);

// Implemented in http_client_request.c
esp_err_t http_request_get(const char *url, char *buf, size_t buf_len);
// esp_err_t http_request_post(const char *url, const char *body, const char *content_type);

void http_client_notify_network_up(void)
{
    ESP_LOGD(TAG, "Network up");
    s_client.network_up = true;
    task_scheduler_add(&s_client.task_node, 0);
}

void http_client_notify_network_down(void)
{
    ESP_LOGD(TAG, "Network down");
    s_client.network_up = false;
    task_scheduler_remove(&s_client.task_node);
}

const char *http_client_get_response(void)
{
    return s_response_buf;
}

/**
 * @brief Scheduler callback; fires once per poll interval.
 *
 * Sends a GET request to the configured endpoint and stores the response in
 * @p s_response_buf. Always reschedules itself so polling continues
 * indefinitely while the network is up.
 *
 * @param node Pointer to the embedded task_node inside http_client_ctx_t.
 * @return TASK_RUN_AGAIN to keep the node active in the scheduler.
 */
static task_status_t http_poll(task_node_t *node)
{
    http_client_ctx_t *self = container_of(node, http_client_ctx_t, task_node);
    (void)self;

    if (http_client_get("http://ubdev.emk530.net:10480/result.json", s_response_buf, CONFIG_REDMOLE_HTTP_RESPONSE_BUF_SIZE) != ESP_OK)
    {
        ESP_LOGW(TAG, "Poll request failed");
    }

    node->run_at_tick = xTaskGetTickCount() + pdMS_TO_TICKS(CONFIG_REDMOLE_HTTP_POLL_INTERVAL_MS);
    return TASK_RUN_AGAIN;
}

esp_err_t http_client_init(http_client_tls_mode_t mode, const char *ca_cert_pem)
{
    ESP_LOGD(TAG, "Initializing");

    s_response_buf = heap_caps_malloc(CONFIG_REDMOLE_HTTP_RESPONSE_BUF_SIZE, MALLOC_CAP_SPIRAM);
    if (!s_response_buf)
    {
        ESP_LOGE(TAG, "Response buffer allocation failed");
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = http_tls_init(mode, ca_cert_pem);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "TLS init failed: %s", esp_err_to_name(err));
        free(s_response_buf);
        s_response_buf = NULL;
        return err;
    }

    s_client.task_node.work = http_poll;
    return ESP_OK;
}

esp_err_t http_client_get(const char *url, char *buf, size_t buf_len)
{
    if (!s_client.network_up)
    {
        ESP_LOGE(TAG, "Request failed, network is not up");
        return ESP_FAIL;
    }

    return http_request_get(url, buf, buf_len);
}

// esp_err_t http_client_post(const char *url, const char *body, const char *content_type)
// {
//     if (!s_client.network_up)
//     {
//         ESP_LOGE(TAG, "Request failed, network is not up");
//         return ESP_FAIL;
//     }
//
//     return http_request_post(url, body, content_type);
// }

void http_client_deinit(void)
{
    ESP_LOGD(TAG, "Deinitializing");
    s_client.network_up = false;
    free(s_response_buf);
    s_response_buf = NULL;
    http_tls_deinit();
}
