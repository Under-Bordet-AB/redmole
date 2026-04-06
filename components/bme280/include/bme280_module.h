#ifndef BME280_MODULE_H
#define BME280_MODULE_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef enum {
    BME_STATE_INIT,
    BME_STATE_READING,
    BME_STATE_IDLE,
    BME_STATE_ERROR
} bme_state_t;

typedef struct {
    SemaphoreHandle_t data_mutex; // Protects shared data (Sec 2.2)
    bme_state_t state;            // FSM state (Sec 3.1)
    float temperature;
    float humidity;
    float pressure;
} bme280_ctx_t;

void bme280_init(bme280_ctx_t *self, QueueHandle_t log_queue); // Dependency Injection example
void bme280_run(bme280_ctx_t *self);
void bme280_deinit(bme280_ctx_t *self);

#endif // BME280_MODULE_H
