idf_component_register(
    SRCS "src/blufi_init.c"
         "src/blufi_main.c"
         "src/blufi_security.c"
    INCLUDE_DIRS "include"
    REQUIRES log freertos esp_netif esp_event nac bt
)