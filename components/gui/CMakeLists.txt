idf_component_register(
    SRCS "src/gui_module.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos esp_timer esp_lcd esp_driver_i2c sensor_data lvgl__lvgl log event_notify
)
