idf_component_register(
    SRCS
        "http_client_request_test.c"
        "http_client_test.c"
        "http_client_tls_test.c"
    REQUIRES unity http_client
)
