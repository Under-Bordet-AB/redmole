#ifndef LOCAL_SENSOR_SERVICE_H
#define LOCAL_SENSOR_SERVICE_H

/**
 * @file
 * @brief Application service that polls the board-local environmental sensor.
 *
 * The service bridges the BME280 HAL and the sensor_data store. It owns the
 * polling task and keeps main.c free of sensor polling details.
 */

#include "esp_err.h"

/**
 * @brief Initialize the local environmental sensor service.
 *
 * The service owns the BME280 HAL instance and prepares the polling task state.
 * It does not start polling until local_sensor_service_start() is called.
 *
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t local_sensor_service_init(void);

/**
 * @brief Start the local environmental sensor polling task.
 *
 * The task reads the BME280 HAL at the backend-recommended period and publishes
 * successful measurements into sensor_data. Missing hardware is treated as a
 * non-fatal degraded state and is reported through logs.
 *
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t local_sensor_service_start(void);

/**
 * @brief Stop the local sensor task and release owned HAL state.
 */
void local_sensor_service_deinit(void);

#endif // LOCAL_SENSOR_SERVICE_H
