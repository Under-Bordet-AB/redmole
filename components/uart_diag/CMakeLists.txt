idf_component_register(
    SRCS "src/uart_mole.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos log nvs task_scheduler http_client uart
)
