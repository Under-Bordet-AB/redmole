idf_component_register(
    SRCS "main.c"
         "src/app_gui_bindings.c"
    INCLUDE_DIRS "include"
    REQUIRES log freertos sensor_data nvs gui esp_netif esp_event nac task_scheduler http_client local_sensor_service uart_diag espressif__cjson 
)
