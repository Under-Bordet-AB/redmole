idf_component_register(
    # 1. List ALL source files inside the main folder
    SRCS "main.c"
         "src/app_module.c"

    # 2. Where the app_module.h header lives
    INCLUDE_DIRS "include"

    # 3. Dependencies
    # In ESP-IDF, the 'main' component automatically depends on ALL custom
    # components in the /components folder. However, it's good practice
    # to explicitly list the standard IDF libraries your main.c uses directly.
    REQUIRES nvs_flash log freertos wifi
)
