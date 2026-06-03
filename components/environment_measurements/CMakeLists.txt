idf_component_register(
    SRCS "src/environment_measurements.cpp"
    INCLUDE_DIRS "include"
    REQUIRES bme280 board_i2c esp_timer freertos log
)
