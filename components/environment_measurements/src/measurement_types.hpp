#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace redmole::environment {

enum class MeasurementChannel : uint8_t {
    IndoorAmbientTemperature,
    IndoorRelativeHumidity,
    IndoorPressure,
    Count,
};

struct Measurement {
    MeasurementChannel channel;
    int64_t value;
};

constexpr size_t kMaxMeasurementsPerBatch = 4U;

struct MeasurementBatch {
    std::array<Measurement, kMaxMeasurementsPerBatch> measurements = {};
    size_t count = 0U;
};

struct StoredMeasurement {
    int64_t timestamp_ms = 0;
    int64_t value = 0;
    uint64_t publication_version = 0U;
    bool valid = false;
};

constexpr size_t measurement_channel_index(MeasurementChannel channel) {
    return static_cast<size_t>(channel);
}

constexpr bool measurement_channel_is_valid(MeasurementChannel channel) {
    return measurement_channel_index(channel) <
           measurement_channel_index(MeasurementChannel::Count);
}

} // namespace redmole::environment
