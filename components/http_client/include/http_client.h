#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "esp_err.h"

void http_client_notify_network_up(void);
void http_client_notify_network_down(void);

esp_err_t http_client_init(const char *ca_cert_pem);
esp_err_t http_client_get(const char *url, char *buf, size_t buf_len);
esp_err_t http_client_post(const char *url, const char *body, const char *content_type);
void      http_client_deinit(void);


#endif // HTTP_CLIENT_H
