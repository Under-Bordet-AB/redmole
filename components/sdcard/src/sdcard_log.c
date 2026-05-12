#include "sdcard_log.h"
#include "sdcard.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log_write.h"

#define LOG_MSG_MAX_LEN 200
#define LOG_BUFFER_SIZE 2048
#define LOG_FLUSH_INTERVAL pdMS_TO_TICKS(2000)
#define LOG_QUEUE_LENGTH 32

typedef struct {
    char text[LOG_MSG_MAX_LEN];
} log_message_t;

static QueueHandle_t log_queue = NULL;
static TaskHandle_t log_task_handle = NULL;
static vprintf_like_t original_vprintf = NULL;

static char log_buffer[LOG_BUFFER_SIZE];
static const char *log_file_path = NULL;


/* =========================
   Hook for printf
   ========================= */

int sdcard_log_vprintf(const char *fmt, va_list args)
{
    if (!log_queue) {
        return vprintf(fmt, args);
    }

    log_message_t msg;
    vsnprintf(msg.text, LOG_MSG_MAX_LEN, fmt, args);

    xQueueSend(log_queue, &msg, 0);

    return vprintf(fmt, args);
}

/* =========================
   Logging task
   ========================= */
static void log_task(void *arg)
{
    size_t offset = 0;
    TickType_t last_flush = xTaskGetTickCount();

    log_message_t msg;

    while (1) {
        if ((LOG_BUFFER_SIZE - offset >= LOG_MSG_MAX_LEN) || offset == 0) {
            if (xQueueReceive(log_queue, &msg, LOG_FLUSH_INTERVAL) == pdTRUE) {
                size_t written = snprintf(
                    log_buffer + offset,
                    LOG_BUFFER_SIZE - offset,
                    "%s",
                    msg.text
                );
                offset += written;
            }
        }

        TickType_t now = xTaskGetTickCount();

        if ((offset > 0) &&
            ((now - last_flush > LOG_FLUSH_INTERVAL) ||
             (offset > (LOG_BUFFER_SIZE * 3) / 4))) {

            sdcard_append_file(log_file_path, log_buffer);
            memset(log_buffer, 0, sizeof(log_buffer));
            offset = 0;
            last_flush = now;
        }
    }
}

/* =========================
   Public API
   ========================= */

esp_err_t sdcard_log_init(const char *file_path)
{
    if (log_queue != NULL) {
        return ESP_OK; // already initialized
    }

    if (!sdcard_is_initialized()) {
        return ESP_ERR_INVALID_STATE;
    }

    log_file_path = file_path;

    log_queue = xQueueCreate(LOG_QUEUE_LENGTH, sizeof(log_message_t));
    if (!log_queue) {
        return ESP_ERR_NO_MEM;
    }

    original_vprintf = esp_log_set_vprintf(sdcard_log_vprintf);
    BaseType_t res = xTaskCreate(
        log_task,
        "sd_log_task",
        4096,
        NULL,
        tskIDLE_PRIORITY + 1,
        &log_task_handle
    );

    if (res != pdPASS) {
        vQueueDelete(log_queue);
        log_queue = NULL;
        return ESP_FAIL;
    }


    return ESP_OK;
}

void sdcard_log_dispose(void)
{
    if (log_task_handle) {
        vTaskDelete(log_task_handle);
        log_task_handle = NULL;
    }

    if (log_queue) {
        vQueueDelete(log_queue);
        log_queue = NULL;
    }

    if (original_vprintf){
        esp_log_set_vprintf(original_vprintf);
    }

    memset(log_buffer, 0, sizeof(log_buffer));
    log_file_path = NULL;
}