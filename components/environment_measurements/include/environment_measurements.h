#ifndef ENVIRONMENT_MEASUREMENTS_H
#define ENVIRONMENT_MEASUREMENTS_H

/**
 * @file
 * @brief Public API for board-local environment measurements.
 *
 * The module owns source detection, polling, simulation fallback, selected
 * latest values, and bounded RAM history. Callers receive copies and never own
 * or access source objects or internal storage.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Latest preferred indoor temperature, humidity, and pressure values.
 *
 * Values use scaled integers:
 * - temperature_deci_c: 231 means 23.1 C
 * - humidity_deci_pct: 453 means 45.3 %
 * - pressure_deci_hpa: 10134 means 1013.4 hPa
 */
typedef struct {
    int64_t timestamp_ms;       /*!< Sample timestamp in milliseconds from esp_timer. */
    int32_t temperature_deci_c; /*!< Temperature in deci-degrees Celsius. */
    int32_t humidity_deci_pct;  /*!< Relative humidity in deci-percent. */
    int32_t pressure_deci_hpa;  /*!< Local pressure in deci-hectopascals. */
    bool valid;                 /*!< True only when all three values are present and fresh. */
} environment_measurement_sample_t;

/**
 * @brief Product location associated with environmental information.
 */
typedef enum {
    ENVIRONMENT_LOCATION_INDOOR,  /*!< Environment inside the product location. */
    ENVIRONMENT_LOCATION_OUTDOOR, /*!< Environment outside the product location. */
    ENVIRONMENT_LOCATION_COUNT,   /*!< Internal sentinel; not a valid API location. */
} environment_location_t;

/**
 * @brief Environmental quantity represented by a value or history channel.
 */
typedef enum {
    ENVIRONMENT_CAPABILITY_TEMPERATURE,
    ENVIRONMENT_CAPABILITY_HUMIDITY,
    ENVIRONMENT_CAPABILITY_PRESSURE,
    ENVIRONMENT_CAPABILITY_CARBON_DIOXIDE,
    ENVIRONMENT_CAPABILITY_VOLATILE_ORGANIC_COMPOUNDS,
    ENVIRONMENT_CAPABILITY_PARTICULATE_MATTER_1,
    ENVIRONMENT_CAPABILITY_PARTICULATE_MATTER_2_5,
    ENVIRONMENT_CAPABILITY_PARTICULATE_MATTER_10,
    ENVIRONMENT_CAPABILITY_ILLUMINANCE,
    ENVIRONMENT_CAPABILITY_SOUND_LEVEL_DECI_DBA,
    ENVIRONMENT_CAPABILITY_RAINFALL_SINCE_MIDNIGHT,
    ENVIRONMENT_CAPABILITY_WIND_SPEED,
    ENVIRONMENT_CAPABILITY_WIND_DIRECTION,
    ENVIRONMENT_CAPABILITY_ULTRAVIOLET_INDEX,
    ENVIRONMENT_CAPABILITY_SOIL_MOISTURE_RELATIVE,
    ENVIRONMENT_CAPABILITY_COUNT, /*!< Internal sentinel; not a reportable capability. */
} environment_capability_t;

/**
 * @brief Canonically scaled value selected by an environment capability.
 *
 * The associated environment_capability_t determines which union member is
 * valid.
 */
typedef union {
    int32_t temperature_deci_c;                 /*!< Temperature in 0.1 degrees Celsius. */
    int32_t humidity_deci_pct;                  /*!< Relative humidity in 0.1 percent. */
    int32_t pressure_deci_hpa;                  /*!< Pressure in 0.1 hectopascals. */
    uint32_t carbon_dioxide_ppm;                /*!< Carbon dioxide in parts per million. */
    uint32_t volatile_organic_compounds_ppb;    /*!< VOC concentration in parts per billion. */
    uint32_t particulate_matter_1_ug_m3;        /*!< PM1 in micrograms per cubic meter. */
    uint32_t particulate_matter_2_5_ug_m3;      /*!< PM2.5 in micrograms per cubic meter. */
    uint32_t particulate_matter_10_ug_m3;       /*!< PM10 in micrograms per cubic meter. */
    uint32_t illuminance_lux;                   /*!< Illuminance in lux. */
    uint32_t sound_level_deci_dba;              /*!< A-weighted sound level in 0.1 dBA. */
    uint32_t rainfall_since_midnight_deci_mm;   /*!< Rainfall since midnight in 0.1 mm. */
    uint32_t wind_speed_deci_m_s;               /*!< Wind speed in 0.1 meters per second. */
    uint16_t wind_direction_degrees;            /*!< Wind direction in degrees from north. */
    uint16_t ultraviolet_index_deci;            /*!< Ultraviolet index in 0.1 UV index. */
    uint16_t soil_moisture_relative_deci_pct;   /*!< Sensor-relative moisture in 0.1 percent. */
} environment_value_t;

/**
 * @brief Caller-owned copy of one retained graph-history value.
 */
typedef struct {
    int64_t timestamp_ms;       /*!< Graph-retention time in monotonic milliseconds. */
    environment_value_t value; /*!< Value selected by the requested history capability. */
} environment_history_sample_t;

/**
 * @brief Initialize the environment measurements module.
 *
 * Creates permanent source resources and initializes fixed internal storage.
 * Physical sensor absence is not an initialization failure. This function is
 * idempotent; polling does not start until environment_measurements_start().
 *
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t environment_measurements_init(void);

/**
 * @brief Start the environment measurement polling task.
 *
 * environment_measurements_init() must succeed first. The function is
 * idempotent and reuses the statically allocated task after a stop.
 *
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t environment_measurements_start(void);

/**
 * @brief Compatibility alias for environment_measurements_stop().
 *
 * Repeated calls are safe and permanent resources remain allocated.
 */
void environment_measurements_deinit(void);

/**
 * @brief Cooperatively stop environment polling.
 *
 * The call blocks until any active source operation finishes and the polling
 * task acknowledges the stop. Permanent source resources and retained values
 * remain available. Repeated calls are safe.
 */
void environment_measurements_stop(void);

/**
 * @brief Copy the latest stable environment measurement.
 *
 * The function briefly blocks on the latest-value mutex.
 *
 * @param out Caller-owned output sample, must not be NULL.
 * @return True when a fresh complete indoor sample was copied, false otherwise.
 */
bool environment_measurements_get_latest(environment_measurement_sample_t* out);

/**
 * @brief Check whether the latest valid sample is recent enough.
 *
 * @param max_age_ms Maximum accepted sample age in milliseconds.
 * @return True when the latest valid sample exists and is no older than max_age_ms.
 */
bool environment_measurements_is_fresh(uint32_t max_age_ms);

/**
 * @brief Return the number of samples published since initialization.
 *
 * @return Monotonic valid-publication counter, or 0 before initialization.
 */
uint32_t environment_measurements_get_update_count(void);

/**
 * @brief Copy retained graph history for one configured location and capability.
 *
 * Samples are copied oldest first. The function briefly blocks on the history
 * mutex and returns zero for channels that are not configured.
 *
 * @param location Location of the requested history channel.
 * @param capability Capability of the requested history channel.
 * @param out Caller-owned output array, must not be NULL.
 * @param max_count Capacity of out in samples; must be greater than zero.
 * @return Number of samples copied into out.
 */
size_t environment_measurements_copy_history(environment_location_t location,
                                             environment_capability_t capability,
                                             environment_history_sample_t* out,
                                             size_t max_count);

#ifdef __cplusplus
}
#endif

#endif // ENVIRONMENT_MEASUREMENTS_H
