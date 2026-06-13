#include "unity.h"

#include "sim/sim_producer.hpp"

using redmole::environment::MeasurementBatch;
using redmole::environment::MeasurementChannel;
using redmole::environment::sim::SimProducer;

TEST_CASE("sim producer returns three measurements", "[environment_measurements][unit][sim]") {
    SimProducer producer(MeasurementChannel::IndoorAmbientTemperature,
                         MeasurementChannel::IndoorRelativeHumidity,
                         MeasurementChannel::IndoorPressure);
    MeasurementBatch batch = {};

    TEST_ASSERT_EQUAL(ESP_OK, producer.init());
    TEST_ASSERT_EQUAL(ESP_OK, producer.read(batch));
    TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(batch.count));
}
