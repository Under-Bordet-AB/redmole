#ifndef GUI_MODULE_H
#define GUI_MODULE_H

#include "freertos/FreeRTOS.h"

typedef struct {
    bool is_ready;
    void *frame_buffer; // Pointer to PSRAM allocated memory
} gui_ctx_t;

void gui_init(gui_ctx_t *self);
void gui_run(gui_ctx_t *self); // Task to process LVGL timers and render
void gui_deinit(gui_ctx_t *self);

#endif // GUI_MODULE_H
