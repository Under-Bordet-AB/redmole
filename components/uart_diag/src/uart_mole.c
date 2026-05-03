/* UART Mole
 * - Desc will go here later
 */

#include "uart_mole.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "task_scheduler.h"
#include <stdint.h>

#define UART_MOLE_PORT          0
#define UART_MOLE_TX            43
#define UART_MOLE_RX            44
#define UART_MOLE_RX_BUF_SIZE   128
#define UART_MOLE_TX_BUF_SIZE   1024
#define UART_MOLE_BAUD_RATE     115200

static QueueHandle_t s_uart_mole_queue         = NULL;
static TaskHandle_t  s_uart_mole_listener_task = NULL;

static const char *TAG = "uart_mole";

/* Forward declarations */
static task_status_t uart_mole_work(task_node_t *node);
static void uart_mole_listener_task(void *pvParameters);

/* @brief Initialize UART mole, configure UART driver, start UART listener task.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t uart_mole_init(void)
{
    /* Configure UART */
    const uart_config_t uart_config =
    {
        .baud_rate  = UART_MOLE_BAUD_RATE,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err;
    /* Set UART configuration
     * This is the HAL layer. We're setting the timing and framing (UART frame).
     */
    err = uart_param_config(UART_MOLE_PORT, &uart_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(err));
        return err;
    }

    /* Set UART pins, double check if this is correct...
     * This is the still the HAL layer, we're connecting the internal UART silicon to the GPIO pins.
     */
    err = uart_set_pin(UART_MOLE_PORT, UART_MOLE_TX, UART_MOLE_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_set_pin_failed: %s", esp_err_to_name(err));
        return err;
    }

    /* Install UART driver
     * This is driver- and RTOS level. Memory allocation for ringbuffers,
     * sets up RTOS queue and registers interrupt handler (ISR) in the background.
     */
    err = uart_driver_install(UART_MOLE_PORT, UART_MOLE_RX_BUF_SIZE, UART_MOLE_TX_BUF_SIZE, 20, &s_uart_mole_queue, NULL);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(err));
        return err;
    }

    /* Create a task to handle UART mole data processing. */
    err = xTaskCreate(uart_mole_listener_task, "uart_mole_listener_task", 4096, NULL, 12, &s_uart_mole_listener_task);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "xTaskCreate failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "UART mole initialized successfully");
    return ESP_OK;
}

/* @brief Deinitialize the UART mole context and ESP-IDF UART driver.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t uart_mole_deinit(void)
{

    return ESP_OK;
}
