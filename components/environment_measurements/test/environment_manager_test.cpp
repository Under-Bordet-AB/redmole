#include "environment_manager.hpp"

#include <array>

#include "unity.h"
#include "unity_test_runner.h"

namespace {

using redmole::environment::EnvironmentLocation;
using redmole::environment::EnvironmentManager;
using redmole::environment::EnvironmentMeasurement;
using redmole::environment::EnvironmentSource;
using redmole::environment::MeasurementBatch;
using redmole::environment::SourceBinding;
using redmole::environment::SourceKind;
using redmole::environment::SourceState;
using redmole::environment::make_humidity;
using redmole::environment::make_pressure;
using redmole::environment::make_temperature;

class FakeSource final : public EnvironmentSource {
  public:
    esp_err_t init() override {
        return init_result;
    }

    bool probe() override {
        return present;
    }

    esp_err_t activate() override {
        return activate_result;
    }

    esp_err_t poll(MeasurementBatch& batch) override {
        if (poll_result != ESP_OK) {
            return poll_result;
        }

        EnvironmentMeasurement temperature{};
        EnvironmentMeasurement humidity{};
        EnvironmentMeasurement pressure{};
        if (!make_temperature(temperature_deci_c, timestamp_ms, temperature) ||
            !make_humidity(humidity_deci_pct, timestamp_ms, humidity) ||
            !make_pressure(pressure_deci_hpa, timestamp_ms, pressure) ||
            !batch.report(temperature) || !batch.report(humidity) || !batch.report(pressure)) {
            return ESP_ERR_INVALID_RESPONSE;
        }

        return ESP_OK;
    }

    esp_err_t init_result = ESP_OK;
    esp_err_t activate_result = ESP_OK;
    esp_err_t poll_result = ESP_OK;
    bool present = true;
    int64_t timestamp_ms = 0LL;
    int32_t temperature_deci_c = 200;
    int32_t humidity_deci_pct = 500;
    int32_t pressure_deci_hpa = 10000;
};

SourceBinding binding(FakeSource& source, SourceKind kind, uint8_t priority) {
    return {&source, "fake", EnvironmentLocation::Indoor, kind, priority, SourceState::Disabled,
            0LL};
}

void activate_and_poll(EnvironmentManager& manager, int64_t now_ms) {
    manager.maintain_sources(now_ms);
    manager.poll_active_sources(now_ms);
}

} // namespace

TEST_CASE("measurement batch rejects duplicate capabilities", "[environment_measurements]") {
    MeasurementBatch batch;
    EnvironmentMeasurement first{};
    EnvironmentMeasurement duplicate{};

    TEST_ASSERT_TRUE(make_temperature(200, 10, first));
    TEST_ASSERT_TRUE(make_temperature(210, 10, duplicate));
    TEST_ASSERT_TRUE(batch.report(first));
    TEST_ASSERT_FALSE(batch.report(duplicate));
    TEST_ASSERT_EQUAL_UINT32(1U, batch.size());
}

TEST_CASE("hardware candidates win over simulation candidates", "[environment_measurements]") {
    FakeSource hardware;
    FakeSource simulation;
    hardware.temperature_deci_c = 210;
    simulation.temperature_deci_c = 300;
    std::array<SourceBinding, 2> bindings{
        binding(hardware, SourceKind::Hardware, 10U),
        binding(simulation, SourceKind::Simulation, 1U),
    };
    EnvironmentManager manager(bindings.data(), bindings.size());

    TEST_ASSERT_EQUAL(ESP_OK, manager.init_sources(100));
    hardware.timestamp_ms = 100;
    simulation.timestamp_ms = 100;
    activate_and_poll(manager, 100);

    environment_measurement_sample_t sample{};
    TEST_ASSERT_TRUE(manager.copy_compatibility(100, sample));
    TEST_ASSERT_EQUAL_INT32(210, sample.temperature_deci_c);
}

TEST_CASE("failed hardware immediately falls back to fresh simulation",
          "[environment_measurements]") {
    FakeSource hardware;
    FakeSource simulation;
    hardware.temperature_deci_c = 210;
    simulation.temperature_deci_c = 300;
    std::array<SourceBinding, 2> bindings{
        binding(hardware, SourceKind::Hardware, 10U),
        binding(simulation, SourceKind::Simulation, 100U),
    };
    EnvironmentManager manager(bindings.data(), bindings.size());

    TEST_ASSERT_EQUAL(ESP_OK, manager.init_sources(100));
    hardware.timestamp_ms = 100;
    simulation.timestamp_ms = 100;
    activate_and_poll(manager, 100);

    hardware.poll_result = ESP_FAIL;
    simulation.timestamp_ms = 200;
    manager.poll_active_sources(200);

    environment_measurement_sample_t sample{};
    TEST_ASSERT_TRUE(manager.copy_compatibility(200, sample));
    TEST_ASSERT_EQUAL_INT32(300, sample.temperature_deci_c);
}

TEST_CASE("selected values become invalid when stale without another commit",
          "[environment_measurements]") {
    FakeSource source;
    std::array<SourceBinding, 1> bindings{
        binding(source, SourceKind::Hardware, 10U),
    };
    EnvironmentManager manager(bindings.data(), bindings.size());

    TEST_ASSERT_EQUAL(ESP_OK, manager.init_sources(100));
    source.timestamp_ms = 100;
    activate_and_poll(manager, 100);

    environment_measurement_sample_t sample{};
    TEST_ASSERT_TRUE(manager.copy_compatibility(100, sample));
    TEST_ASSERT_FALSE(
        manager.copy_compatibility(100 + EnvironmentManager::kStaleTimeoutMs + 1LL, sample));
}

TEST_CASE("poll accepts acquisition timestamps created during the poll",
          "[environment_measurements]") {
    FakeSource source;
    std::array<SourceBinding, 1> bindings{
        binding(source, SourceKind::Hardware, 10U),
    };
    EnvironmentManager manager(bindings.data(), bindings.size());

    TEST_ASSERT_EQUAL(ESP_OK, manager.init_sources(100));
    manager.maintain_sources(100);
    source.timestamp_ms = 125;
    manager.poll_active_sources(100);

    environment_measurement_sample_t sample{};
    TEST_ASSERT_TRUE(manager.copy_compatibility(125, sample));
    TEST_ASSERT_EQUAL_INT64(125, sample.timestamp_ms);
}

TEST_CASE("alternate BME280 address can provide indoor compatibility values",
          "[environment_measurements]") {
    FakeSource alternate_address;
    std::array<SourceBinding, 1> bindings{
        binding(alternate_address, SourceKind::Hardware, 20U),
    };
    EnvironmentManager manager(bindings.data(), bindings.size());

    TEST_ASSERT_EQUAL(ESP_OK, manager.init_sources(100));
    alternate_address.timestamp_ms = 100;
    activate_and_poll(manager, 100);

    environment_measurement_sample_t sample{};
    TEST_ASSERT_TRUE(manager.copy_compatibility(100, sample));
}
