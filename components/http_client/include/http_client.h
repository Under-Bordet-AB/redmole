#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "esp_err.h"

/*
 * Called by NAC when WiFi connects. Starts the periodic poll task.
 * Called by NAC when WiFi disconnects. Stops the periodic poll task.
 */
void http_client_notify_network_up(void);
void http_client_notify_network_down(void);

/*
 * Initializes the HTTP client. Must be called once at startup before
 * any other function. Pass a PEM-encoded CA certificate for TLS, or
 * NULL for plain HTTP.
 */
esp_err_t http_client_init(const char *ca_cert_pem);

/*
 * Sends a blocking GET request to url.
 * Response body is written into buf, up to buf_len bytes.
 * Returns ESP_FAIL if the network is not up.
 */
esp_err_t http_client_get(const char *url, char *buf, size_t buf_len);

/*
 * Sends a blocking POST request to url with the given body and content_type.
 * Returns ESP_FAIL if the network is not up.
 */
esp_err_t http_client_post(const char *url, const char *body, const char *content_type);

/*
 * Deinitializes the HTTP client. Call http_client_notify_network_down()
 * first if the poll task is currently active.
 */
void http_client_deinit(void);


#endif // HTTP_CLIENT_H
