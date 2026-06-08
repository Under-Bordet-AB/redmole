#pragma once

/**
 * @file
 * @brief Generic measurement types shared by producers, the manager, and the store.
 *
 * These types deliberately describe logical measurements instead of physical
 * sensors. This lets the rest of the module work with a BME280, a simulator,
 * or a future sensor through the same small data model.
 */

#include <array>
#include <cstddef>
#include <cstdint>

namespace redmole::environment {

/** @brief Maximum number of logical values one producer can return in one read. */
constexpr size_t kMaxMeasurementsPerBatch = 4U;

/**
 * @brief Logical destinations for measurements stored by this module.
 *
 * Each channel has one canonical internal unit. The public C API converts these
 * internal units into its application-facing units.
 */
enum class MeasurementChannel : uint8_t {
    IndoorAmbientTemperature, /*!< Indoor ambient temperature in milli-degrees Celsius. */
    IndoorRelativeHumidity,   /*!< Indoor relative humidity in milli-percent. */
    IndoorPressure,           /*!< Indoor atmospheric pressure in pascals. */
    Count,                    /*!< Number of real channels; never stores a measurement. */
};

/** @brief One logical channel value returned by a producer. */
struct Measurement {
    MeasurementChannel channel; /*!< Logical destination and definition of the value's unit. */
    int64_t value;              /*!< Value expressed in the channel's canonical internal unit. */
};

/**
 * @brief Fixed-capacity result from one coherent producer acquisition.
 *
 * Only entries before @ref count are meaningful. A producer must set count on
 * every read and must not return the same channel more than once.
 */
struct MeasurementBatch {
    std::array<Measurement, kMaxMeasurementsPerBatch> measurements = {}; /*!< Batch storage. */
    size_t count = 0U; /*!< Number of populated entries in measurements. */
};

/** @brief Latest stored state for one logical measurement channel. */
struct MeasurementRecord {
    int64_t timestamp_ms = 0;          /*!< Publication time in milliseconds. */
    int64_t value = 0;                 /*!< Value in the channel's canonical internal unit. */
    uint64_t publication_version = 0U; /*!< Version shared by one published batch. */
    bool valid = false;                /*!< True only while the latest producer read succeeded. */
};

/** @brief Convert a channel enum into its fixed-array index. */
constexpr size_t measurement_channel_index(MeasurementChannel channel) {
    return static_cast<size_t>(channel);
}

/** @brief Check that a channel names a real stored channel rather than Count. */
constexpr bool measurement_channel_is_valid(MeasurementChannel channel) {
    return measurement_channel_index(channel) <
           measurement_channel_index(MeasurementChannel::Count);
}

} // namespace redmole::environment
