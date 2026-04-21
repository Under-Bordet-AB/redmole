idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "include"
    REQUIRES log freertos bme280 sensor_data nvs gui esp_netif esp_event nac task_scheduler
)
