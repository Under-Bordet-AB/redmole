idf_component_register(
    SRCS
        "src/environment_measurements.cpp"
        "src/measurement_store.cpp"
        "src/measurements_manager.cpp"
        "src/bme280/bme280_producer.cpp"
        "src/bme280/bme280_sensor.cpp"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES board_i2c esp_timer freertos log
)
