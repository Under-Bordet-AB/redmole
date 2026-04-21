#include "gui_module.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_types.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lvgl.h"
#include "sensor_data.h"

static const char* TAG = "GUI";

#define GUI_LCD_H_RES 1024
#define GUI_LCD_V_RES 600
//#define GUI_LCD_PIXEL_CLOCK_HZ 30850000UL
#define GUI_LCD_PIXEL_CLOCK_HZ 24000000UL
#define GUI_LCD_DATA_WIDTH 16
#define GUI_LCD_NUM_FRAME_BUFFERS 2
#define GUI_LCD_BOUNCE_BUFFER_SIZE_PX (GUI_LCD_H_RES * 10)
#define GUI_LVGL_TICK_PERIOD_MS 2
#define GUI_LVGL_TASK_MAX_DELAY_MS 500
#define GUI_LVGL_TASK_MIN_DELAY_MS 1
#define GUI_LABEL_UPDATE_PERIOD_MS 500
#define GUI_I2C_TIMEOUT_MS 100
//#define GUI_TASK_STACK_SIZE 8192
#define GUI_TASK_STACK_SIZE 10240
#define GUI_TASK_PRIORITY 4

#define GUI_I2C_SDA_GPIO GPIO_NUM_8
#define GUI_I2C_SCL_GPIO GPIO_NUM_9
#define GUI_I2C_PORT I2C_NUM_0
#define GUI_I2C_FREQUENCY_HZ 400000UL
#define GUI_IO_EXPANDER_ADDRESS 0x24
#define GUI_IO_EXPANDER_REG_MODE 0x02
#define GUI_IO_EXPANDER_REG_OUTPUT 0x03
#define GUI_IO_EXPANDER_BACKLIGHT_PIN 2

typedef struct {
    bool (*get_sample)(sensor_data_sample* out_sample, void* user_ctx);
    bool (*is_fresh)(uint32_t max_age_ms, void* user_ctx);
    uint32_t (*get_update_count)(void* user_ctx);
    void* user_ctx;
} gui_data_provider;

typedef struct {
    bool initialized;
    bool is_ready;
    bool task_started;
    TaskHandle_t task_handle;
    esp_lcd_panel_handle_t panel_handle;
    lv_display_t* display;
    i2c_master_bus_handle_t i2c_bus;
    i2c_master_dev_handle_t io_expander;
    esp_timer_handle_t tick_timer;
    lv_obj_t* label_title;
    lv_obj_t* label_temperature;
    lv_obj_t* label_humidity;
    lv_obj_t* label_pressure;
    lv_obj_t* label_status;
    uint8_t io_output_state;
    gui_data_provider data_provider;
} gui_runtime;

static gui_runtime s_gui = {0};

static bool gui_sensor_data_get_sample(sensor_data_sample* out_sample, void* user_ctx) {
    (void)user_ctx;
    return sensor_data_get_latest_local(out_sample);
}

static bool gui_sensor_data_is_fresh(uint32_t max_age_ms, void* user_ctx) {
    (void)user_ctx;
    return sensor_data_is_local_fresh(max_age_ms);
}

static uint32_t gui_sensor_data_get_update_count(void* user_ctx) {
    (void)user_ctx;
    return sensor_data_get_local_update_count();
}

static const gui_data_provider s_default_provider = {
    .get_sample = gui_sensor_data_get_sample,
    .is_fresh = gui_sensor_data_is_fresh,
    .get_update_count = gui_sensor_data_get_update_count,
    .user_ctx = NULL,
};

static int32_t gui_abs_i32(int32_t value) {
    return (value < 0) ? -value : value;
}

static void gui_set_default_provider(void) {
    s_gui.data_provider = s_default_provider;
}

static bool gui_fetch_sample(sensor_data_sample* out_sample) {
    if ((out_sample == NULL) || (s_gui.data_provider.get_sample == NULL)) {
        return false;
    }

    return s_gui.data_provider.get_sample(out_sample, s_gui.data_provider.user_ctx);
}

static bool gui_sample_is_fresh(uint32_t max_age_ms) {
    if (s_gui.data_provider.is_fresh == NULL) {
        return false;
    }

    return s_gui.data_provider.is_fresh(max_age_ms, s_gui.data_provider.user_ctx);
}

static uint32_t gui_get_update_count(void) {
    if (s_gui.data_provider.get_update_count == NULL) {
        return 0U;
    }

    return s_gui.data_provider.get_update_count(s_gui.data_provider.user_ctx);
}

static void gui_refresh_measurement_labels(void) {
    sensor_data_sample sample = {0};
    char line[96];
    bool has_sample = gui_fetch_sample(&sample);

    if (has_sample && sample.valid) {
        snprintf(line, sizeof(line), "Temperature: %" PRId32 ".%" PRId32 " C",
                 sample.temperature_deci_c / 10, gui_abs_i32(sample.temperature_deci_c % 10));
        lv_label_set_text(s_gui.label_temperature, line);

        snprintf(line, sizeof(line), "Humidity: %" PRId32 ".%" PRId32 " %%",
                 sample.humidity_deci_pct / 10, gui_abs_i32(sample.humidity_deci_pct % 10));
        lv_label_set_text(s_gui.label_humidity, line);

        snprintf(line, sizeof(line), "Pressure: %" PRId32 ".%" PRId32 " hPa",
                 sample.pressure_deci_hpa / 10, gui_abs_i32(sample.pressure_deci_hpa % 10));
        lv_label_set_text(s_gui.label_pressure, line);
    } else {
        lv_label_set_text(s_gui.label_temperature, "Temperature: --");
        lv_label_set_text(s_gui.label_humidity, "Humidity: --");
        lv_label_set_text(s_gui.label_pressure, "Pressure: --");
    }

    snprintf(line, sizeof(line), "Local sample: %s   Updates: %" PRIu32,
             gui_sample_is_fresh(3000U) ? "fresh" : "stale", gui_get_update_count());
    lv_label_set_text(s_gui.label_status, line);
}

static bool gui_rgb_flush_ready(esp_lcd_panel_handle_t panel,
                                const esp_lcd_rgb_panel_event_data_t* event_data, void* user_ctx) {
    LV_UNUSED(panel);
    LV_UNUSED(event_data);
    lv_display_flush_ready((lv_display_t*)user_ctx);
    return false;
}

static void gui_lvgl_flush_cb(lv_display_t* display, const lv_area_t* area, uint8_t* px_map) {
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)lv_display_get_user_data(display);

    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map);
}

static void gui_lvgl_tick_cb(void* arg) {
    LV_UNUSED(arg);
    lv_tick_inc(GUI_LVGL_TICK_PERIOD_MS);
}

static esp_err_t gui_io_expander_write_register(uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {reg, value};

    if (s_gui.io_expander == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    return i2c_master_transmit(s_gui.io_expander, buffer, sizeof(buffer), GUI_I2C_TIMEOUT_MS);
}

static esp_err_t gui_board_init_io_expander(void) {
    i2c_master_bus_config_t bus_config = {
        .i2c_port = GUI_I2C_PORT,
        .sda_io_num = GUI_I2C_SDA_GPIO,
        .scl_io_num = GUI_I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_config, &s_gui.i2c_bus), TAG,
                        "i2c bus init failed");

    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = GUI_IO_EXPANDER_ADDRESS,
        .scl_speed_hz = GUI_I2C_FREQUENCY_HZ,
    };

    ESP_RETURN_ON_ERROR(
        i2c_master_bus_add_device(s_gui.i2c_bus, &device_config, &s_gui.io_expander), TAG,
        "io expander add failed");

    s_gui.io_output_state = 0xFF;

    ESP_RETURN_ON_ERROR(gui_io_expander_write_register(GUI_IO_EXPANDER_REG_MODE, 0xFF), TAG,
                        "io expander mode write failed");
    ESP_RETURN_ON_ERROR(
        gui_io_expander_write_register(GUI_IO_EXPANDER_REG_OUTPUT, s_gui.io_output_state), TAG,
        "io expander output write failed");

    return ESP_OK;
}

static esp_err_t gui_board_set_backlight(bool enabled) {
    if (enabled) {
        s_gui.io_output_state |= (uint8_t)(1U << GUI_IO_EXPANDER_BACKLIGHT_PIN);
    } else {
        s_gui.io_output_state &= (uint8_t)~(1U << GUI_IO_EXPANDER_BACKLIGHT_PIN);
    }

    return gui_io_expander_write_register(GUI_IO_EXPANDER_REG_OUTPUT, s_gui.io_output_state);
}

static esp_err_t gui_board_init_panel(void) {
    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .data_width = GUI_LCD_DATA_WIDTH,
        .in_color_format = LCD_COLOR_FMT_RGB565,
        .num_fbs = GUI_LCD_NUM_FRAME_BUFFERS,
        .bounce_buffer_size_px = GUI_LCD_BOUNCE_BUFFER_SIZE_PX,
        .dma_burst_size = 64,
        .hsync_gpio_num = GPIO_NUM_46,
        .vsync_gpio_num = GPIO_NUM_3,
        .de_gpio_num = GPIO_NUM_5,
        .pclk_gpio_num = GPIO_NUM_7,
        .disp_gpio_num = GPIO_NUM_NC,
        .data_gpio_nums =
            {
                GPIO_NUM_14,
                GPIO_NUM_38,
                GPIO_NUM_18,
                GPIO_NUM_17,
                GPIO_NUM_10,
                GPIO_NUM_39,
                GPIO_NUM_0,
                GPIO_NUM_45,
                GPIO_NUM_48,
                GPIO_NUM_47,
                GPIO_NUM_21,
                GPIO_NUM_1,
                GPIO_NUM_2,
                GPIO_NUM_42,
                GPIO_NUM_41,
                GPIO_NUM_40,
            },
        .timings =
            {
                .pclk_hz = GUI_LCD_PIXEL_CLOCK_HZ,
                .h_res = GUI_LCD_H_RES,
                .v_res = GUI_LCD_V_RES,
                .hsync_pulse_width = 162,
                .hsync_back_porch = 152,
                .hsync_front_porch = 48,
                .vsync_pulse_width = 45,
                .vsync_back_porch = 13,
                .vsync_front_porch = 3,
                .flags.pclk_active_neg = 1,
            },
        .flags.fb_in_psram = 1,
    };

    ESP_RETURN_ON_ERROR(esp_lcd_new_rgb_panel(&panel_config, &s_gui.panel_handle), TAG,
                        "rgb panel create failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_gui.panel_handle), TAG, "rgb panel reset failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_gui.panel_handle), TAG, "rgb panel init failed");

    return ESP_OK;
}

static esp_err_t gui_lvgl_init(void) {
    lv_init();

    s_gui.display = lv_display_create(GUI_LCD_H_RES, GUI_LCD_V_RES);
    if (s_gui.display == NULL) {
        return ESP_ERR_NO_MEM;
    }

    lv_display_set_user_data(s_gui.display, s_gui.panel_handle);
    lv_display_set_color_format(s_gui.display, LV_COLOR_FORMAT_RGB565);

    void* buf1 = NULL;
    void* buf2 = NULL;
    ESP_RETURN_ON_ERROR(esp_lcd_rgb_panel_get_frame_buffer(s_gui.panel_handle, 2, &buf1, &buf2),
                        TAG, "frame buffer fetch failed");

    lv_display_set_buffers(s_gui.display, buf1, buf2,
                           GUI_LCD_H_RES * GUI_LCD_V_RES * sizeof(uint16_t),
                           LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(s_gui.display, gui_lvgl_flush_cb);

    esp_lcd_rgb_panel_event_callbacks_t callbacks = {
        .on_color_trans_done = gui_rgb_flush_ready,
    };
    ESP_RETURN_ON_ERROR(
        esp_lcd_rgb_panel_register_event_callbacks(s_gui.panel_handle, &callbacks, s_gui.display),
        TAG, "rgb callbacks failed");

    const esp_timer_create_args_t tick_timer_args = {
        .callback = gui_lvgl_tick_cb,
        .name = "gui_lvgl_tick",
    };
    ESP_RETURN_ON_ERROR(esp_timer_create(&tick_timer_args, &s_gui.tick_timer), TAG,
                        "tick timer create failed");
    ESP_RETURN_ON_ERROR(esp_timer_start_periodic(s_gui.tick_timer, GUI_LVGL_TICK_PERIOD_MS * 1000U),
                        TAG, "tick timer start failed");

    return ESP_OK;
}

static void gui_update_labels(lv_timer_t* timer) {
    LV_UNUSED(timer);
    gui_refresh_measurement_labels();
}

static esp_err_t gui_build_screen(void) {
    lv_obj_t* screen = lv_screen_active();
    lv_obj_t* panel = lv_obj_create(screen);
    lv_obj_set_size(panel, 900, 380);
    lv_obj_center(panel);
    lv_obj_set_style_pad_all(panel, 24, 0);
    lv_obj_set_style_radius(panel, 12, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xE9EEF3), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(0xC6D0DA), 0);
    lv_obj_set_style_text_color(panel, lv_color_hex(0x1F2933), 0);

    s_gui.label_title = lv_label_create(panel);
    lv_label_set_text(s_gui.label_title, "Redmole Local Sensor");

    s_gui.label_temperature = lv_label_create(panel);
    s_gui.label_humidity = lv_label_create(panel);
    s_gui.label_pressure = lv_label_create(panel);
    s_gui.label_status = lv_label_create(panel);

    gui_refresh_measurement_labels();
    lv_timer_t* timer = lv_timer_create(gui_update_labels, GUI_LABEL_UPDATE_PERIOD_MS, NULL);
    if (timer == NULL) {
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

static void gui_task(void* pvParameters) {
    (void)pvParameters;

    while (true) {
        uint32_t delay_ms = lv_timer_handler();
        delay_ms = MAX(delay_ms, GUI_LVGL_TASK_MIN_DELAY_MS);
        delay_ms = MIN(delay_ms, GUI_LVGL_TASK_MAX_DELAY_MS);
        usleep(delay_ms * 1000U);
    }
}

esp_err_t gui_init(void) {
    if (s_gui.initialized) {
        return ESP_OK;
    }

    memset(&s_gui, 0, sizeof(s_gui));
    gui_set_default_provider();

    ESP_RETURN_ON_ERROR(gui_board_init_io_expander(), TAG, "board io expander init failed");
    ESP_RETURN_ON_ERROR(gui_board_init_panel(), TAG, "board panel init failed");
    ESP_RETURN_ON_ERROR(gui_lvgl_init(), TAG, "lvgl init failed");
    ESP_RETURN_ON_ERROR(gui_build_screen(), TAG, "screen build failed");
    ESP_RETURN_ON_ERROR(gui_board_set_backlight(true), TAG, "backlight enable failed");

    s_gui.initialized = true;
    s_gui.is_ready = true;
    ESP_LOGI(TAG, "Temporary GUI ready for Waveshare ESP32-S3-LCD-7B");
    return ESP_OK;
}

void gui_deinit(void) {
    if (!s_gui.initialized) {
        return;
    }

    if (s_gui.tick_timer != NULL) {
        esp_timer_stop(s_gui.tick_timer);
        esp_timer_delete(s_gui.tick_timer);
        s_gui.tick_timer = NULL;
    }

    if (s_gui.panel_handle != NULL) {
        gui_board_set_backlight(false);
        esp_lcd_panel_del(s_gui.panel_handle);
        s_gui.panel_handle = NULL;
    }

    if (s_gui.io_expander != NULL) {
        i2c_master_bus_rm_device(s_gui.io_expander);
        s_gui.io_expander = NULL;
    }

    if (s_gui.i2c_bus != NULL) {
        i2c_del_master_bus(s_gui.i2c_bus);
        s_gui.i2c_bus = NULL;
    }

    memset(&s_gui, 0, sizeof(s_gui));
}

bool gui_is_ready(void) {
    return s_gui.is_ready;
}

esp_err_t gui_start(void) {
    if (!s_gui.is_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    if (s_gui.task_started) {
        return ESP_OK;
    }

    BaseType_t rv = xTaskCreate(gui_task, "gui_task", GUI_TASK_STACK_SIZE, NULL, GUI_TASK_PRIORITY,
                                &s_gui.task_handle);
    if (rv != pdPASS) {
        return ESP_FAIL;
    }

    s_gui.task_started = true;
    return ESP_OK;
}

esp_err_t gui_set_data_provider(const gui_data_provider* provider) {
    if (provider == NULL) {
        gui_set_default_provider();
        return ESP_OK;
    }

    if ((provider->get_sample == NULL) || (provider->is_fresh == NULL) ||
        (provider->get_update_count == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    s_gui.data_provider = *provider;
    return ESP_OK;
}
