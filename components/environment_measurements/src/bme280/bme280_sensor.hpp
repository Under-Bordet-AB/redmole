#pragma once

#include <cstdint>

#include "driver/i2c_master.h"
#include "environment_sensor.hpp"

namespace redmole::environment {

struct Bme280RawSample {
    int32_t adc_temperature;
    int32_t adc_pressure;
    int32_t adc_humidity;
};

struct Bme280Calibration {
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;
};

class Bme280Sensor final : public EnvironmentSensor {
public:
    explicit Bme280Sensor(uint8_t address);

    esp_err_t init() override;
    bool probe() override;
    esp_err_t read(environment_measurement_sample_t& out) override;
    SensorLocation location() const override;
    bool is_simulated() const override;

private:
    esp_err_t ensure_device_handle();
    esp_err_t connect();
    void mark_disconnected();
    esp_err_t configure();
    esp_err_t write_ctrl_meas();
    esp_err_t read_u8(uint8_t reg, uint8_t& out);
    esp_err_t wait_until_ready();
    esp_err_t load_calibration();
    esp_err_t read_raw(Bme280RawSample& out_raw);
    void convert(const Bme280RawSample& raw, environment_measurement_sample_t& out) const;

    uint8_t address_;
    i2c_master_dev_handle_t dev_ = nullptr;
    Bme280Calibration calibration_ = {};
    bool present_ = false;
    bool configured_ = false;
    uint8_t mode_ = 1U;
    uint8_t osrs_t_ = 1U;
    uint8_t osrs_p_ = 1U;
    uint8_t osrs_h_ = 1U;
    uint8_t filter_ = 0U;
    uint8_t standby_ = 5U;
};

} // namespace redmole::environment
