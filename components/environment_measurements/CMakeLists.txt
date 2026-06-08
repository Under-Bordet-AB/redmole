# Compile every implementation into one ESP-IDF component library.
idf_component_register(
    SRCS
        "src/environment_measurements.cpp"
        "src/measurement_store.cpp"
        "src/measurements_manager.cpp"
        "src/bme280/bme280_producer.cpp"
        "src/bme280/bme280_sensor.cpp"
        "src/sim/sim_producer.cpp"
    # Application code may include only the stable public C API.
    INCLUDE_DIRS "include"

    # Internal C++ headers remain visible only while building this component.
    PRIV_INCLUDE_DIRS "src"

    # These ESP-IDF and project components provide the hardware and RTOS APIs used above.
    REQUIRES board_i2c esp_timer freertos log
)
