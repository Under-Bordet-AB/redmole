idf_component_register(
    SRCS
        "src/sntp.c"
    INCLUDE_DIRS "include"
    REQUIRES
        lwip
        log
        freertos
)