#include <stdbool.h>
#include <string.h>
#include "http_client.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "task_scheduler.h"

/*
 * Internal state for the HTTP client module.
 * task_node is embedded so the scheduler does not need to allocate it.
 * network_up gates all outgoing requests, and stays false until WiFi is up.
 */
typedef struct
{
    task_node_t task_node;
    bool network_up;
} http_client_ctx_t;

static http_client_ctx_t s_client = {0};
static const char* TAG = "HTTP Client";

// ============= FORWARD DECLARATIONS ============= //

// Implemented in http_client_tls.c
esp_err_t _tls_init(http_client_tls_mode_t mode, const char *ca_cert_pem);
const char *tls_get_cert(void);
http_client_tls_mode_t tls_get_mode(void);
void _tls_deinit(void);

// Implemented in http_client_request.c
esp_err_t request_get(const char *url, char *buf, size_t buf_len);
// esp_err_t request_post(const char *url, const char *body, const char *content_type);

// ================================================ //

/*
 * Called by NAC when WiFi connects.
 * Enables outgoing requests and adds the poll node to the scheduler
 * so http_poll starts firing immediately on the next scheduler tick.
 */
void http_client_notify_network_up(void)
{
    ESP_LOGD(TAG, "Setting network_up to true");
    s_client.network_up = true;
    task_scheduler_add(&s_client.task_node, 0);
}

/*
 * Called by NAC when WiFi disconnects.
 * Disables outgoing requests and removes the poll node from the scheduler
 * so no further polls are attempted while offline.
 */
void http_client_notify_network_down(void)
{
    ESP_LOGD(TAG, "Setting network_up to false");
    s_client.network_up = false;
    task_scheduler_remove(&s_client.task_node);
}

/*
 * Scheduler callback, fires once per poll interval.
 * Uses container_of to recover the full context from the node pointer.
 * Always reschedules itself by updating run_at_tick before returning
 * TASK_RUN_AGAIN, keeping the node alive in the scheduler indefinitely.
 * This is where the actual API fetch and result handling will live.
 */
static task_status_t http_poll(task_node_t *node)
{
    http_client_ctx_t *self = container_of(node, http_client_ctx_t, task_node);
    (void)self; // TODO: call request_get() and forward result to sensor_data

    node->run_at_tick = xTaskGetTickCount() + pdMS_TO_TICKS(CONFIG_REDMOLE_HTTP_POLL_INTERVAL_MS);
    return TASK_RUN_AGAIN;
}

/*
 * Initializes the HTTP client module.
 * Pass a PEM-encoded CA certificate string for TLS, or NULL for plain HTTP.
 * Assigns the poll callback to the task node so it is ready to be
 * scheduled when the network comes up.
 */
esp_err_t http_client_init(http_client_tls_mode_t mode, const char* ca_cert_pem)
{
    ESP_LOGD(TAG, "Initializing client");
    
    esp_err_t err = _tls_init(mode, ca_cert_pem);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "TLS init failed: %s", esp_err_to_name(err));
        return err;
    }

    s_client.task_node.work = http_poll;
    return ESP_OK;
}

/*
 * Sends a synchronous GET request to url.
 * Response body is written into buf up to buf_len bytes.
 * Returns ESP_FAIL if the network is not up.
 * Returns ESP_OK on success, or the ESP error code on failure.
 */
esp_err_t http_client_get(const char* url, char* buf, size_t buf_len)
{
    if (!s_client.network_up)
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

/*
 * Sends a synchronous POST request to url.
 * body is sent as the request body with the given content_type header.
 * Returns ESP_FAIL if the network is not up.
 * Returns ESP_OK on success, or the ESP error code on failure.
 */
// esp_err_t http_client_post(const char* url, const char* body, const char* content_type)
// {
//     if (!s_client.network_up)
//     {
//         ESP_LOGE(TAG, "Request failed, network is not up");
//         return ESP_FAIL;
//     }

//     ESP_LOGD(TAG, "Sending POST request");
//     esp_err_t err = request_post(url, body, content_type);
//     if (err != ESP_OK)
//     {
//         ESP_LOGE(TAG, "Request failed: %s", esp_err_to_name(err));
//         return err;
//     }

//     return ESP_OK;
// }

/*
 * Deinitializes the HTTP client module.
 * Clears the TLS certificate. Does not remove the task node,
 * so call http_client_notify_network_down() first if the node is active.
 */
void http_client_deinit(void)
{
    ESP_LOGD(TAG, "Deinitializing client");
    _tls_deinit();
}
