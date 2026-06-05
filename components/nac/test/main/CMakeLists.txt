idf_component_register(
    SRCS "nac_test.c"
    INCLUDE_DIRS "."
    REQUIRES nac unity task_scheduler nvs
)
