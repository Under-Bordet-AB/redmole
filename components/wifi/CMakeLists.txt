idf_component_register(
    SRCS "src/wifi_module.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_event nvs_flash
)
