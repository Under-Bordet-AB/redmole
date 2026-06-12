#include "http_client.h"
#include "unity.h"

// Implemented in http_client_tls.c
esp_err_t   http_tls_init(http_client_tls_mode_t mode, const char *ca_cert_pem);
void        http_tls_deinit(void);
const char *http_tls_get_cert(void);

// Implemented in http_client_request.c
esp_err_t http_request_get(const char *url, char *buf, size_t buf_len);

TEST_CASE("Should return ESP_FAIL when network is down", "[http_client][unit]")
{
    http_client_notify_network_down();
    char buf[16] = "mock buffer";
    esp_err_t err = http_client_get("www.example.com", buf, sizeof(buf));
    TEST_ASSERT_EQUAL(ESP_FAIL, err);
}

TEST_CASE("Should return the cert that was passed in init", "[http_client][unit]")
{
    const char *cert = "mock cert";
    esp_err_t err = http_tls_init(HTTP_CLIENT_TLS_CERT, cert);
    const char *cert2 = http_tls_get_cert();
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL_PTR(cert, cert2);
}

TEST_CASE("Passing TLS_CERT but no actual cert string", "[http_client][unit]")
{
    esp_err_t err = http_tls_init(HTTP_CLIENT_TLS_CERT, NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

TEST_CASE("Setting TLS to BUNDLE should keep cert string NULL", "[http_client][unit]")
{
    esp_err_t err = http_tls_init(HTTP_CLIENT_TLS_BUNDLE, NULL);
    const char *cert = http_tls_get_cert();
    TEST_ASSERT_EQUAL_PTR(NULL, cert);
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

TEST_CASE("Setting TLS to NONE should keep cert string NULL", "[http_client][unit]")
{
    esp_err_t err = http_tls_init(HTTP_CLIENT_TLS_NONE, NULL);
    const char *cert = http_tls_get_cert();
    TEST_ASSERT_EQUAL_PTR(NULL, cert);
    TEST_ASSERT_EQUAL(ESP_OK, err);
}