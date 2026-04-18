idf_component_register(
    SRCS "main.c"
         "src/app_module.c"

    INCLUDE_DIRS "include"

    # IDF components used directly by main.c and app_module.c.
    # Custom components (nac, task_scheduler) are auto-resolved by the
    # build system from /components — no need to list them here.
    REQUIRES nvs_flash log freertos esp_netif esp_event nac
)
