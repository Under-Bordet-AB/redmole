idf_component_register(
    SRCS "main.c"
         "src/app_gui_bindings.c"
         "src/blufi_security.c"
         "src/blufi_init.c"
         "src/blufi_main.c"
    INCLUDE_DIRS "include"
    REQUIRES log freertos bme280 sensor_data nvs gui esp_netif esp_event nac task_scheduler protocomm bt
)
