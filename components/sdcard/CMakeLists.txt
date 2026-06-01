idf_component_register(
    SRCS "src/sdcard_log.c" "src/sdcard.c"
    INCLUDE_DIRS "include"
    REQUIRES driver fatfs sdmmc io_extension nvs)