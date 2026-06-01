idf_component_register(
    SRCS "src/local_sensor_service.c"
    INCLUDE_DIRS "include"
    REQUIRES bme280 board_i2c sensor_data freertos log
)
