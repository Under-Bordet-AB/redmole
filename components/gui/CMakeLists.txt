idf_component_register(
	SRCS "src/module/gui_module.c"
	     "src/module/gui_module_runtime.c"
	     "src/module/gui_module_events.c"
	     "src/module/gui_module_platform.c"
	     "src/control/gui_control.c"
	     "src/control/gui_control_internal.c"
	     "src/view/gui_view.c"
	     "src/view/gui_view_common.c"
	     "src/view/assets/hk_bg.c"
	     src/view/assets/terminal_bg.c
	     "src/view/assets/fonts/hellokitty18.c"
	     "src/view/assets/fonts/hellokitty24.c"
	     src/view/assets/fonts/terminal18.c
	     src/view/assets/fonts/terminal24.c
	     "src/control/gui_control_wifi.c"
	     "src/control/gui_control_model.c"
	     "src/view/panels/gui_view_bme280_panel.c"
	     "src/view/panels/gui_view_energy_panel.c"
	     "src/view/panels/gui_view_forecast_panel.c"
	     "src/view/panels/gui_view_settings_panel.c"
	INCLUDE_DIRS "include"
	REQUIRES lvgl_port rgb_lcd_port touch io_extension esp_lcd log sensor_data esp_timer
)
