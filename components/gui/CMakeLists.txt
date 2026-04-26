set(gui_srcs
    "src/module/gui_module.c"
    "src/module/gui_module_runtime.c"
    "src/module/gui_module_events.c"
    "src/module/gui_module_platform.c"
    "src/control/gui_control.c"
    "src/control/gui_control_internal.c"
    "src/view/gui_view.c"
    "src/view/gui_view_common.c"
    "src/view/gui_theme_defs.c"
    "src/control/gui_control_wifi.c"
    "src/control/gui_control_model.c"
    "src/view/panels/gui_view_bme280_panel.c"
    "src/view/panels/gui_view_energy_panel.c"
    "src/view/panels/gui_view_forecast_panel.c"
    "src/view/panels/gui_view_settings_panel.c"
)

if(CONFIG_REDMOLE_GUI_THEME_HELLO_KITTY)
    list(APPEND gui_srcs
         "src/view/assets/hk_bg.c"
         "src/view/assets/hk_bg_night.c"
         "src/view/assets/fonts/hellokitty18.c"
         "src/view/assets/fonts/hellokitty24.c")
endif()

if(CONFIG_REDMOLE_GUI_THEME_TERMINAL)
    list(APPEND gui_srcs
         "src/view/assets/terminal_bg.c"
         "src/view/assets/fonts/terminal20.c"
         "src/view/assets/fonts/terminal26.c")
endif()

if(CONFIG_REDMOLE_GUI_THEME_SPONGEBOB)
    list(APPEND gui_srcs
         "src/view/assets/spongebob_bg.c"
         "src/view/assets/fonts/spongebob18.c"
         "src/view/assets/fonts/spongebob24.c")
endif()

if(CONFIG_REDMOLE_GUI_THEME_DEATH_NOTE)
    list(APPEND gui_srcs
         "src/view/assets/deathnote_bg.c"
         "src/view/assets/fonts/deathnote20.c"
         "src/view/assets/fonts/deathnote26.c")
endif()

idf_component_register(
    SRCS ${gui_srcs}
    INCLUDE_DIRS "include"
    REQUIRES lvgl_port rgb_lcd_port touch io_extension esp_lcd log sensor_data esp_timer
)
