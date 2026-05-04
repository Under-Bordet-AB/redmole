/* UART Mole
 * - Desc will go here later
 */

#include "uart_mole.h"
#include "driver/uart.h"
#include "esp_crc.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/idf_additions.h"
#include "nac.h"
#include "sensor_data.h"
#include "task_scheduler.h"
#include <stdint.h>
#include <string.h>
#include <time.h>

#define UART_MOLE_PORT          0
#define UART_MOLE_TX            43
#define UART_MOLE_RX            44
#define UART_MOLE_RX_BUF_SIZE   128
#define UART_MOLE_TX_BUF_SIZE   1024
#define UART_MOLE_BAUD_RATE     115200
#define UART_MOLE_STACK_SIZE    4096

static const char *TAG = "UART_MOLE";

static uart_ctx_t s_uart_mole;

/* Forward declarations */
static task_status_t uart_mole_work(task_node_t *node);
static void uart_mole_listener_task(void *pvParameters);

esp_err_t uart_mole_init(EventGroupHandle_t *event_group)
{
    memset(&s_uart_mole, 0, sizeof(uart_ctx_t));
    s_uart_mole.event_group       = event_group;
    s_uart_mole.tag               = TAG;
    s_uart_mole.uart_mole_port    = UART_MOLE_PORT;
    s_uart_mole.uart_mole_tx_pin  = UART_MOLE_TX;
    s_uart_mole.uart_mole_rx_pin  = UART_MOLE_RX;
    s_uart_mole.task_node.work    = uart_mole_work;

    const uart_config_t uart_config =
    {
        .baud_rate  = UART_MOLE_BAUD_RATE,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err = uart_param_config(UART_MOLE_PORT, &uart_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(err));
        return err;
    }

    err = uart_set_pin(UART_MOLE_PORT, UART_MOLE_TX, UART_MOLE_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(err));
        return err;
    }

    err = uart_driver_install(UART_MOLE_PORT, UART_MOLE_RX_BUF_SIZE, UART_MOLE_TX_BUF_SIZE,
                              20, &s_uart_mole.queue, NULL);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(err));
        return err;
    }

    BaseType_t rc = xTaskCreate(uart_mole_listener_task, "uart_mole_listener",
                                UART_MOLE_STACK_SIZE, NULL, 12, &s_uart_mole.rtos_task);
    if (rc != pdPASS)
    {
        ESP_LOGE(TAG, "xTaskCreate failed");
        uart_driver_delete(UART_MOLE_PORT);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "initialized");
    return ESP_OK;
}

esp_err_t uart_mole_deinit(void)
{
    if (s_uart_mole.rtos_task)
    {
        vTaskDelete(s_uart_mole.rtos_task);
        s_uart_mole.rtos_task = NULL;
    }
    uart_driver_delete(UART_MOLE_PORT);
    return ESP_OK;
}

/* Called by the scheduler to transmit the pending package over UART.
 * Wire format: [tag_bit (1)] [data_len (2)] [payload (data_len bytes)]
 * data_len is the byte count of everything that follows data_len itself,
 * including the trailing crc16. The receiver reads tag, then data_len,
 * then reads exactly data_len bytes — no start/end bytes needed.
 * Frees the package allocation when done; does not free the context.
 */
static task_status_t uart_mole_work(task_node_t *node)
{
    uart_ctx_t *self = container_of(node, uart_ctx_t, task_node);
    uart_port_t port = (uart_port_t)self->uart_mole_port;

    switch (self->pkg_tag)
    {
        case UART_STATUS_PKG:
        {
            uart_status_pkg_t *pkg = (uart_status_pkg_t *)self->pkg_data;
            /* payload: status + uptime_s + timestamp_s + crc16 */
            uart_write_bytes(port, &pkg->tag_bit, sizeof(pkg->tag_bit) + sizeof(pkg->data_len));
            uart_write_bytes(port, &pkg->status, pkg->data_len);
            heap_caps_free(pkg);
            break;
        }

        case UART_SENSOR_PKG:
        {
            uart_sensor_pkg_t *pkg = (uart_sensor_pkg_t *)self->pkg_data;
            /* payload: temperature_x_100 + humidity_x_100 + pressure_x_100 + timestamp_s + crc16 */
            uart_write_bytes(port, &pkg->tag_bit, sizeof(pkg->tag_bit) + sizeof(pkg->data_len));
            uart_write_bytes(port, &pkg->temperature_x_100, pkg->data_len);
            heap_caps_free(pkg);
            break;
        }

        case UART_CONFIG_PKG:
        {
            uart_config_pkg_t *pkg = (uart_config_pkg_t *)self->pkg_data;
            /* payload: rslt_bit + timestamp_s + crc16 */
            uart_write_bytes(port, &pkg->tag_bit, sizeof(pkg->tag_bit) + sizeof(pkg->data_len));
            uart_write_bytes(port, &pkg->rslt_bit, pkg->data_len);
            heap_caps_free(pkg);
            break;
        }

        case UART_DIAG_PKG:
        {
            uart_diag_pkg_t *pkg = (uart_diag_pkg_t *)self->pkg_data;
            /* payload: number_of_tasks + stack_hw + stack_used + timestamp_s + crc16 */
            uart_write_bytes(port, &pkg->tag_bit, sizeof(pkg->tag_bit) + sizeof(pkg->data_len));
            uart_write_bytes(port, &pkg->number_of_tasks, pkg->data_len);
            heap_caps_free(pkg);
            break;
        }

        case UART_SERVER_PKG:
        {
            uart_server_pkg_t *pkg = (uart_server_pkg_t *)self->pkg_data;
            /* payload: json_data (variable) + timestamp_s + crc16
             * json_data is not inline in the struct so it cannot be sent in one write.
             * json_len is recovered from data_len: data_len = json_len + timestamp_s + crc16 */
            uint16_t json_len = pkg->data_len - sizeof(pkg->timestamp_s) - sizeof(pkg->crc16);
            uart_write_bytes(port, &pkg->tag_bit, sizeof(pkg->tag_bit) + sizeof(pkg->data_len));
            uart_write_bytes(port, pkg->json_data, json_len);
            /* timestamp_s and crc16 are contiguous at the end of the packed struct */
            uart_write_bytes(port, &pkg->timestamp_s, sizeof(pkg->timestamp_s) + sizeof(pkg->crc16));
            heap_caps_free(pkg);
            break;
        }
    }

    self->pkg_data = NULL;
    return TASK_DONE;
}

/* Waits for incoming UART commands, allocates and populates the appropriate
 * package on PSRAM, then hands it to the scheduler for transmission.
 */
static void uart_mole_listener_task(void *pvParameters)
{
    (void)pvParameters;
    uart_event_t event;
    uint8_t cmd;

    for (;;)
    {
        if (!xQueueReceive(s_uart_mole.queue, &event, portMAX_DELAY))
            continue;

        if (event.type != UART_DATA)
            continue;

        if (uart_read_bytes(UART_MOLE_PORT, &cmd, 1, pdMS_TO_TICKS(20)) != 1)
            continue;

        if (s_uart_mole.task_node.active)
        {
            ESP_LOGW(TAG, "TX pending, dropping cmd 0x%02x", cmd);
            continue;
        }

        switch ((uart_pkg_tag_t)cmd)
        {
            /* data_len: byte count of everything after the data_len field, i.e. the full
             * payload including the trailing crc16. Receiver reads tag, then data_len,
             * then reads exactly data_len bytes — equivalent to sizeof(pkg) - 3.
             *
             * crc16: covers all payload bytes except crc16 itself, i.e. the first
             * (data_len - sizeof(crc16)) bytes starting from the first data field.
             * Computed with esp_crc16_le, initial value 0.
             */
            case UART_STATUS_PKG:
            {
                /* payload (data_len bytes): status + uptime_s + timestamp_s + crc16
                 * crc16 covers:             status + uptime_s + timestamp_s            */
                uart_status_pkg_t *pkg = heap_caps_malloc(sizeof(*pkg), MALLOC_CAP_SPIRAM);
                if (!pkg) { ESP_LOGE(TAG, "alloc failed"); break; }

                uint8_t status = 0;
                if (nac_get_wifi_status() == NAC_WIFI_CONNECTED)
                    status |= SET_WIFI_ONLINE;
                if (sensor_data_is_local_fresh(5000))
                    status |= SET_BME280_SENSOR_ONLINE;

                pkg->tag_bit     = UART_STATUS_PKG;
                pkg->data_len    = sizeof(*pkg) - sizeof(pkg->tag_bit) - sizeof(pkg->data_len);
                pkg->status      = status;
                pkg->uptime_s    = (uint32_t)(esp_timer_get_time() / 1000000ULL);
                pkg->timestamp_s = (uint32_t)time(NULL);
                pkg->crc16       = esp_crc16_le(0, &pkg->status,
                                                (uint32_t)(pkg->data_len - sizeof(pkg->crc16)));

                s_uart_mole.pkg_tag  = UART_STATUS_PKG;
                s_uart_mole.pkg_data = pkg;
                s_uart_mole.stats.uart_mole_status_pkg++;
                task_scheduler_add(&s_uart_mole.task_node, 0);
                break;
            }

            case UART_SENSOR_PKG:
            {
                /* payload (data_len bytes): temperature_x_100 + humidity_x_100 + pressure_x_100 + timestamp_s + crc16
                 * crc16 covers:             temperature_x_100 + humidity_x_100 + pressure_x_100 + timestamp_s         */
                sensor_data_sample sample;
                if (!sensor_data_get_latest_local(&sample))
                {
                    ESP_LOGW(TAG, "no sensor data available");
                    break;
                }

                uart_sensor_pkg_t *pkg = heap_caps_malloc(sizeof(*pkg), MALLOC_CAP_SPIRAM);
                if (!pkg) { ESP_LOGE(TAG, "alloc failed"); break; }

                pkg->tag_bit           = UART_SENSOR_PKG;
                pkg->data_len          = sizeof(*pkg) - sizeof(pkg->tag_bit) - sizeof(pkg->data_len);
                pkg->temperature_x_100 = sample.temperature_deci_c * 10;
                pkg->humidity_x_100    = sample.humidity_deci_pct * 10;
                pkg->pressure_x_100    = sample.pressure_deci_hpa * 10;
                pkg->timestamp_s       = (uint32_t)(sample.timestamp_ms / 1000);
                pkg->crc16             = esp_crc16_le(0, (uint8_t const *)&pkg->temperature_x_100,
                                                      (uint32_t)(pkg->data_len - sizeof(pkg->crc16)));

                s_uart_mole.pkg_tag  = UART_SENSOR_PKG;
                s_uart_mole.pkg_data = pkg;
                s_uart_mole.stats.uart_mole_sensor_pkg++;
                task_scheduler_add(&s_uart_mole.task_node, 0);
                break;
            }

            case UART_CONFIG_PKG:
            {
                /* payload (data_len bytes): rslt_bit + timestamp_s + crc16
                 * crc16 covers:             rslt_bit + timestamp_s            */
                uart_config_pkg_t *pkg = heap_caps_malloc(sizeof(*pkg), MALLOC_CAP_SPIRAM);
                if (!pkg) { ESP_LOGE(TAG, "alloc failed"); break; }

                pkg->tag_bit     = UART_CONFIG_PKG;
                pkg->data_len    = sizeof(*pkg) - sizeof(pkg->tag_bit) - sizeof(pkg->data_len);
                pkg->rslt_bit    = SET_CONFIG_OK;
                pkg->timestamp_s = (uint32_t)time(NULL);
                pkg->crc16       = esp_crc16_le(0, &pkg->rslt_bit,
                                                (uint32_t)(pkg->data_len - sizeof(pkg->crc16)));

                s_uart_mole.pkg_tag  = UART_CONFIG_PKG;
                s_uart_mole.pkg_data = pkg;
                s_uart_mole.stats.uart_mole_config_pkg++;
                task_scheduler_add(&s_uart_mole.task_node, 0);
                break;
            }

            case UART_DIAG_PKG:
            {
                /* payload (data_len bytes): number_of_tasks + stack_hw + stack_used + timestamp_s + crc16
                 * crc16 covers:             number_of_tasks + stack_hw + stack_used + timestamp_s         */
                uart_diag_pkg_t *pkg = heap_caps_malloc(sizeof(*pkg), MALLOC_CAP_SPIRAM);
                if (!pkg) { ESP_LOGE(TAG, "alloc failed"); break; }

                UBaseType_t free_words = uxTaskGetStackHighWaterMark(s_uart_mole.rtos_task);
                uint32_t    free_bytes = (uint32_t)(free_words * sizeof(StackType_t));

                pkg->tag_bit         = UART_DIAG_PKG;
                pkg->data_len        = sizeof(*pkg) - sizeof(pkg->tag_bit) - sizeof(pkg->data_len);
                pkg->number_of_tasks = (uint8_t)uxTaskGetNumberOfTasks();
                pkg->stack_hw        = free_bytes;
                pkg->stack_used      = UART_MOLE_STACK_SIZE - free_bytes;
                pkg->timestamp_s     = (uint32_t)time(NULL);
                pkg->crc16           = esp_crc16_le(0, &pkg->number_of_tasks,
                                                    (uint32_t)(pkg->data_len - sizeof(pkg->crc16)));

                s_uart_mole.pkg_tag  = UART_DIAG_PKG;
                s_uart_mole.pkg_data = pkg;
                task_scheduler_add(&s_uart_mole.task_node, 0);
                break;
            }

            case UART_SERVER_PKG:
            {
                /* TODO: allocate single PSRAM block (struct + JSON copy) once the
                 * http_client getter API is available:
                 *
                 *   const char *json_ptr;
                 *   size_t json_len;
                 *   http_client_get_cached(&json_ptr, &json_len);
                 *
                 *   size_t total = sizeof(uart_server_pkg_t) + json_len;
                 *   uart_server_pkg_t *pkg = heap_caps_malloc(total, MALLOC_CAP_SPIRAM);
                 *   pkg->json_data = (char *)(pkg + 1);
                 *   memcpy(pkg->json_data, json_ptr, json_len);
                 *   ...
                 */
                ESP_LOGW(TAG, "SERVER pkg not yet implemented");
                break;
            }

            default:
                ESP_LOGW(TAG, "unknown cmd 0x%02x", cmd);
                break;
        }
    }
}
