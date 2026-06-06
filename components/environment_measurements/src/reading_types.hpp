#pragma once

/**
 * @file
 * @brief Strong value types shared by environment sensor readings.
 */

#include <cstdint>

namespace redmole::environment {

constexpr int64_t kMicrosecondsPerMillisecond = 1000LL;

/** @brief Temperature in thousandths of a degree Celsius. */
struct Temperature {
    int64_t milli_c;
};

/** @brief Relative humidity in thousandths of a percent. */
struct Humidity {
    int64_t milli_pct;
};

/** @brief Pressure in pascals. */
struct Pressure {
    int64_t pa;
};

} // namespace redmole::environment
