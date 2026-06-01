idf_component_register(
    SRCS "main.c"
         "src/app_gui_bindings.c"
         "src/app_gui_bindings/app_gui_forecast.c"
         "src/app_gui_bindings/app_gui_leop.c"
         "src/app_gui_bindings/app_gui_settings_store.c"
         "src/app_gui_bindings/app_gui_sync.c"
         "src/app_gui_bindings/app_gui_time.c"
         "src/app_gui_bindings/app_gui_wifi.c"
    INCLUDE_DIRS "include"
    REQUIRES log freertos sensor_data nvs gui esp_netif esp_event nac task_scheduler http_client local_sensor_service uart_mole espressif__cjson
)
