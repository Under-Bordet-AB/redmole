#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

/**
 * @file
 * @brief Sensor-agnostic latest-sample data store.
 *
 * This module stores copied environmental samples. It intentionally does not
 * know which physical sensor or service produced them.
 */

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Latest local environmental sample stored by sensor_data.
 *
 * The sample mirrors bme280_measurement units but stays independent of the
 * BME280 module so this data store does not depend on a specific sensor driver.
 */
typedef struct {
    int64_t timestamp_ms;       /*!< Sample timestamp in milliseconds from esp_timer. */
    int32_t temperature_deci_c; /*!< Temperature in deci-degrees Celsius. */
    int32_t humidity_deci_pct;  /*!< Relative humidity in deci-percent. */
    int32_t pressure_deci_hpa;  /*!< Local pressure in deci-hectopascals. */
    bool valid;                 /*!< True when this sample contains publishable data. */
} sensor_data_sample;

/**
 * @brief Initialize the local sensor data store.
 *
 * @return ESP_OK on success.
 */
esp_err_t sensor_data_init(void);

/**
 * @brief Deinitialize the local sensor data store.
 *
 * Clears the latest sample and internal counters.
 *
 * @return ESP_OK on success.
 */
esp_err_t sensor_data_deinit(void);

/**
 * @brief Publish a new local environmental sample.
 *
 * The sample is copied into the store using a versioned snapshot pattern so
 * readers do not observe partially written data.
 *
 * @param sample Sample to publish.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t sensor_data_submit_local(const sensor_data_sample* sample);

/**
 * @brief Copy the latest published local sensor sample.
 *
 * This module intentionally stores sample data only. It does not own or expose
 * physical sensor health, bus state, or device-specific status. A returned
 * sample can be valid but old if the sensor later disappears, so consumers that
 * care about live data should also call sensor_data_is_local_fresh().
 *
 * @param out Output sample populated from the latest stable snapshot.
 * @return True when a valid sample has been published, false otherwise.
 */
bool sensor_data_get_latest_local(sensor_data_sample* out);

/**
 * @brief Check whether the latest valid local sample is recent enough.
 *
 * Freshness is the consumer-facing guard against stale values. Missing sensors
 * should be represented by the owning service/health layer; this data store only
 * answers whether the latest published measurement is still timely.
 *
 * @param max_age_ms Maximum accepted sample age in milliseconds.
 * @return True when the latest valid sample exists and is no older than max_age_ms.
 */
bool sensor_data_is_local_fresh(uint32_t max_age_ms);

/**
 * @brief Return the number of local samples published since initialization.
 *
 * @return Monotonic update counter, or 0 if the store is not initialized.
 */
uint32_t sensor_data_get_local_update_count(void);

#endif // SENSOR_DATA_H
