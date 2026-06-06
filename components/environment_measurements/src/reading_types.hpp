#pragma once

/**
 * @file
 * @brief Strong value types shared by environment sensor readings.
 */

#include <cstdint>

namespace redmole::environment {

constexpr int64_t kMicrosecondsPerMillisecond = 1000LL;

/** @brief Temperature in tenths of a degree Celsius. */
struct Temperature {
    int32_t deci_c;
};

/** @brief Relative humidity in tenths of a percent. */
struct Humidity {
    int32_t deci_pct;
};

/** @brief Pressure in tenths of a hectopascal. */
struct Pressure {
    int32_t deci_hpa;
};

} // namespace redmole::environment
