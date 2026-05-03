idf_component_register(
    SRCS "src/.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos log nvs task_scheduler http_client
)
