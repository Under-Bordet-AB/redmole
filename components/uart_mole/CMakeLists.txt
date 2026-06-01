idf_component_register(
    SRCS "src/uart_mole.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos log nvs esp_timer heap esp_rom esp_driver_uart esp_driver_gpio task_scheduler http_client sensor_data
)
