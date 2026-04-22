#include "gui_module_internal.h"

#include "esp_check.h"
#include "gt911.h"
#include "io_extension.h"
#include "lvgl_port.h"
#include "rgb_lcd_port.h"

static int32_t s_backlight_percent = GUI_MODULE_DEFAULT_BRIGHTNESS;
static bool s_backlight_enabled;

void gui_module_apply_brightness(int32_t brightness_percent)
{
    uint8_t pwm_percent;

    if (brightness_percent < 0) {
        brightness_percent = 0;
    } else if (brightness_percent > 100) {
        brightness_percent = 100;
    }

    s_backlight_percent = brightness_percent;

    if (brightness_percent == 0) {
        if (s_backlight_enabled) {
            wavesahre_rgb_lcd_bl_off();
            s_backlight_enabled = false;
        }
        return;
    }

    if (!s_backlight_enabled) {
        wavesahre_rgb_lcd_bl_on();
        s_backlight_enabled = true;
    }

    pwm_percent = (uint8_t)(100 - brightness_percent);
    IO_EXTENSION_Pwm_Output(pwm_percent);
}

esp_err_t gui_module_platform_init_display(void)
{
    esp_lcd_touch_handle_t touch_handle;
    esp_lcd_panel_handle_t lcd_handle;

    touch_handle = touch_gt911_init();
    lcd_handle = waveshare_esp32_s3_rgb_lcd_init();

    ESP_ERROR_CHECK(lvgl_port_init(lcd_handle, touch_handle));
    return ESP_OK;
}

void gui_module_refresh_timer_cb(lv_timer_t *timer)
{
    gui_ctx_t *self;
    gui_module_runtime_t *runtime;

    if (timer == NULL) {
        return;
    }

    self = (gui_ctx_t *)timer->user_data;
    if ((self == NULL) || (self->module_state == NULL)) {
        return;
    }

    runtime = (gui_module_runtime_t *)self->module_state;
    if ((runtime->control.active_panel != GUI_PANEL_BME280) &&
        (runtime->control.active_panel != GUI_PANEL_SETTINGS)) {
        return;
    }

    gui_module_apply_model(runtime);
}

void gui_module_platform_start_refresh_timer(gui_ctx_t *self, gui_module_runtime_t *runtime)
{
    if ((self == NULL) || (runtime == NULL)) {
        return;
    }

    runtime->refresh_timer = lv_timer_create(gui_module_refresh_timer_cb, 2000, self);
    gui_module_apply_brightness(s_backlight_percent);
}

void gui_module_platform_stop_refresh_timer(gui_module_runtime_t *runtime)
{
    if (runtime == NULL) {
        return;
    }

    if ((runtime->refresh_timer != NULL) && lvgl_port_lock(-1)) {
        lv_timer_del(runtime->refresh_timer);
        runtime->refresh_timer = NULL;
        lvgl_port_unlock();
    }

    gui_module_apply_brightness(0);
}