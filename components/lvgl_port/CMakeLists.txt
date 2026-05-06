
idf_component_register(SRCS "lvgl_port.c" 
                        INCLUDE_DIRS "."
                        REQUIRES esp_lcd gpio io_extension rgb_lcd_port touch lvgl
                    )

                        
