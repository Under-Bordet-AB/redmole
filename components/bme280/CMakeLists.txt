set(BME280_BACKEND_SRC "src/hal_internal/bme280_hal_backend.c")

if(CONFIG_REDMOLE_BME280_BACKEND_SIM)
    set(BME280_BACKEND_SRC "src/hal_internal/bme280_hal_backend_sim.c")
endif()

idf_component_register(
    SRCS
        "src/bme280_hal.c"
        "${BME280_BACKEND_SRC}"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES esp_timer log
)
