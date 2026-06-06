#pragma once

#include "bme280_sensor.hpp"
#include "measurement_producer.hpp"

namespace redmole::environment::bme280 {

class Bme280Producer final : public MeasurementProducer {
  public:
    Bme280Producer(Bme280Sensor& sensor, MeasurementChannel temperature_channel,
                   MeasurementChannel humidity_channel, MeasurementChannel pressure_channel);

    esp_err_t init() override;
    esp_err_t read(MeasurementBatch& out) override;

  private:
    Bme280Sensor& sensor_;
    MeasurementChannel temperature_channel_;
    MeasurementChannel humidity_channel_;
    MeasurementChannel pressure_channel_;
};

} // namespace redmole::environment::bme280
