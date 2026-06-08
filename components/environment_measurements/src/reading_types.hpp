#pragma once

/**
 * @file
 * @brief Strong value types shared by environment sensor readings.
 */

#include <cstdint>

namespace redmole::environment {

/** @brief Number of microseconds in one millisecond for ESP timer conversion. */
constexpr int64_t kMicrosecondsPerMillisecond = 1000LL;

/** @brief Temperature in thousandths of a degree Celsius. */
struct Temperature {
    int64_t milli_c; /*!< Signed milli-degrees Celsius. */
};

/** @brief Relative humidity in thousandths of a percent. */
struct Humidity {
    int64_t milli_pct; /*!< Milli-percent relative humidity. */
};

/** @brief Pressure in pascals. */
struct Pressure {
    int64_t pa; /*!< Atmospheric pressure in pascals. */
};

} // namespace redmole::environment
