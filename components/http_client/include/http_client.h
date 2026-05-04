#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <stddef.h>
#include "esp_err.h"

/*
 * Selects the TLS mode used for outgoing HTTPS requests.
 * BUNDLE uses the ESP-IDF built-in CA certificate bundle.
 * CERT   uses a custom PEM certificate passed to http_client_init.
 * NONE   disables TLS and uses plain HTTP.
 */
typedef enum
{
    HTTP_CLIENT_TLS_BUNDLE,
    HTTP_CLIENT_TLS_CERT,
    HTTP_CLIENT_TLS_NONE
} http_client_tls_mode_t;

/*
 * Must be called once at startup before any other function.
 * Allocates the response buffer in SPIRAM, initializes TLS, and
 * prepares the poll task node for use with the scheduler.
 * Pass a PEM-encoded CA certificate when using HTTP_CLIENT_TLS_CERT,
 * otherwise pass NULL.
 */
esp_err_t http_client_init(http_client_tls_mode_t mode, const char *ca_cert_pem);

/*
 * Frees the response buffer and clears TLS state.
 * Call http_client_notify_network_down() before this if the poll task is active.
 */
void http_client_deinit(void);

/*
 * Called by NAC when WiFi connects.
 * Enables outgoing requests and registers the poll task with the scheduler.
 */
void http_client_notify_network_up(void);

/*
 * Called by NAC when WiFi disconnects.
 * Disables outgoing requests and removes the poll task from the scheduler.
 */
void http_client_notify_network_down(void);

/*
 * Returns a pointer to the most recently received response body as a C string.
 * The buffer is owned by the module and updated on each poll cycle.
 * Returns NULL if http_client_init has not been called.
 */
const char *http_client_get_response(void);

/*
 * Sends a one-off blocking GET request to url.
 * The response body is written into buf as a null-terminated string,
 * up to buf_len - 1 bytes.
 * Returns ESP_FAIL if the network is not up.
 */
esp_err_t http_client_get(const char *url, char *buf, size_t buf_len);

// esp_err_t http_client_post(const char *url, const char *body, const char *content_type);


#endif // HTTP_CLIENT_H
