#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

typedef struct {
    int64_t timestamp_ms;
    int32_t temperature_deci_c;
    int32_t humidity_deci_pct;
    int32_t pressure_deci_hpa;
    bool valid;
} sensor_data_sample;

esp_err_t sensor_data_init(void);
esp_err_t sensor_data_deinit(void);
esp_err_t sensor_data_submit_local(const sensor_data_sample* sample);
bool sensor_data_get_latest_local(sensor_data_sample* out);
bool sensor_data_is_local_fresh(uint32_t max_age_ms);
uint32_t sensor_data_get_local_update_count(void);

#endif // SENSOR_DATA_H
