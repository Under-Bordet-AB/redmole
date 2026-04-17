idf_component_register(
    # 1. List ALL source files inside the main folder
    SRCS "main.c"

    # 2. Where the app_module.h header lives
    INCLUDE_DIRS "include"

    # 3. Dependencies
    # In ESP-IDF, the 'main' component automatically depends on ALL custom
    # components in the /components folder. However, it's good practice
    # to explicitly list the standard IDF libraries your main.c uses directly.
    REQUIRES log freertos bme280 sensor_data nvs gui event_notify
)
