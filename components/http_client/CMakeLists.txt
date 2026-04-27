idf_component_register(
    SRCS 
        "src/http_client.c"
        "src/http_client_request.c"
        "src/http_client_tls.c"
    INCLUDE_DIRS "include"
    REQUIRES 
        esp_http_client
        esp-tls
        esp_event
        task_scheduler
)