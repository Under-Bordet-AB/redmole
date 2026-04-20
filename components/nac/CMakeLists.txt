idf_component_register(
    SRCS "src/nac.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_event esp_netif freertos log nvs task_scheduler
)
