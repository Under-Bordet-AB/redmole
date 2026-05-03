/* UART Mole
 * - Desc will go here later
 */

#include "uart_mole.h"
#include "driver/uart.h"
#include "esp_timer.h"
#include "task_scheduler.h"
#include <stdint.h>

#define UART_MOLE_PORT          UART_PORT_0
#define UART_MOLE_TX            GPIO_NUM_43
#define UART_MOLE_RX            GPIO_NUM_44
#define UART_MOLE_RX_BUF_SIZE   128
#define UART_MOLE_TX_BUF_SIZE   1024

/* Forward declarations */
static task_status_t uart_mole_work(task_node_t *node);
static void uart_mole_listener(void *pvParameters);

/* @brief Initialize UART mole, configure UART driver, start UART listener task.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t uart_mole_init(void)
{
    /* Configure UART */
    const uart_config_t uart_config =
    {
        .baud_rate  = 115200,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    /* Set UART configuration */
    if (uart_param_config(UART_MOLE_PORT, &uart_config) != ESP_OK)
    {
        return ESP_FAIL;
    }

    /* Set UART pins */
    if (uart_set_pin(UART_MOLE_PORT, UART_MOLE_TX, UART_MOLE_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK)
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}

/* @brief Deinitialize the UART mole context and ESP-IDF UART driver.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t uart_mole_deinit(void)
{

    return ESP_OK;
}
