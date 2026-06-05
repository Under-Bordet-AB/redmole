#pragma once

/**
 * @file
 * @brief Cross-file contracts for environmental values, batches, and sources.
 *
 * Sources report fixed-capacity batches of tagged canonical values. They do not
 * own locations, selection policy, or published storage.
 */

#include <array>
#include <cstddef>
#include <cstdint>

#include "esp_err.h"

namespace redmole::environment {

constexpr int64_t kUsPerMs = 1000LL;
constexpr size_t kMaxMeasurementsPerPoll = 8U;

/** @brief Environmental quantity represented by a tagged measurement. */
enum class EnvironmentCapability : uint8_t {
    Temperature,
    Humidity,
    Pressure,
    CarbonDioxide,
    VolatileOrganicCompounds,
    ParticulateMatter1,
    ParticulateMatter2_5,
    ParticulateMatter10,
    Illuminance,
    SoundLevelDeciDbA,
    RainfallSinceMidnight,
    WindSpeed,
    WindDirection,
    UltravioletIndex,
    SoilMoistureRelative,
    Count, /*!< Internal array-size sentinel; never reportable. */
};

/** @brief Product location assigned to a source binding. */
enum class EnvironmentLocation : uint8_t {
    Indoor,
    Outdoor,
    Count, /*!< Internal array-size sentinel; never assigned to a source. */
};

/**
 * @brief Canonically scaled environmental value selected by a capability tag.
 */
union EnvironmentValue {
    int32_t temperature_deci_c; /*!< Temperature in 0.1 degrees Celsius. */
    int32_t humidity_deci_pct;  /*!< Relative humidity in 0.1 percent. */
    int32_t pressure_deci_hpa;  /*!< Pressure in 0.1 hectopascals. */
    uint32_t carbon_dioxide_ppm;              /*!< Carbon dioxide in parts per million. */
    uint32_t volatile_organic_compounds_ppb;  /*!< VOC concentration in parts per billion. */
    uint32_t particulate_matter_1_ug_m3;      /*!< PM1 in micrograms per cubic meter. */
    uint32_t particulate_matter_2_5_ug_m3;    /*!< PM2.5 in micrograms per cubic meter. */
    uint32_t particulate_matter_10_ug_m3;     /*!< PM10 in micrograms per cubic meter. */
    uint32_t illuminance_lux;                 /*!< Illuminance in lux. */
    uint32_t sound_level_deci_dba;            /*!< A-weighted sound level in 0.1 dBA. */
    uint32_t rainfall_since_midnight_deci_mm; /*!< Rainfall since midnight in 0.1 mm. */
    uint32_t wind_speed_deci_m_s;             /*!< Wind speed in 0.1 meters per second. */
    uint16_t wind_direction_degrees;          /*!< Direction in degrees from north. */
    uint16_t ultraviolet_index_deci;          /*!< Ultraviolet index in 0.1 UV index. */
    uint16_t soil_moisture_relative_deci_pct; /*!< Sensor-relative moisture in 0.1 percent. */
};

/** @brief One validated, tagged environmental acquisition value. */
struct EnvironmentMeasurement {
    EnvironmentCapability capability; /*!< Tag selecting the valid value member. */
    EnvironmentValue value;           /*!< Canonically scaled acquired value. */
    int64_t timestamp_ms;              /*!< Monotonic acquisition time in milliseconds. */
};

/**
 * @brief Construct a validated temperature measurement.
 * @param value Temperature in 0.1 degrees Celsius.
 * @param timestamp_ms Monotonic acquisition time in milliseconds.
 * @param out Output measurement, unchanged on failure.
 * @return True when value and timestamp are valid, false otherwise.
 */
bool make_temperature(int32_t value, int64_t timestamp_ms, EnvironmentMeasurement& out);

/**
 * @brief Construct a validated humidity measurement.
 * @param value Relative humidity in 0.1 percent.
 * @param timestamp_ms Monotonic acquisition time in milliseconds.
 * @param out Output measurement, unchanged on failure.
 * @return True when value and timestamp are valid, false otherwise.
 */
bool make_humidity(int32_t value, int64_t timestamp_ms, EnvironmentMeasurement& out);

/**
 * @brief Construct a validated pressure measurement.
 * @param value Pressure in 0.1 hectopascals.
 * @param timestamp_ms Monotonic acquisition time in milliseconds.
 * @param out Output measurement, unchanged on failure.
 * @return True when value and timestamp are valid, false otherwise.
 */
bool make_pressure(int32_t value, int64_t timestamp_ms, EnvironmentMeasurement& out);

/**
 * @brief Validate the tag, canonical range, and non-negative timestamp.
 * @param measurement Measurement to validate.
 * @return True when the measurement is structurally valid, false otherwise.
 */
bool validate_measurement(const EnvironmentMeasurement& measurement);

/**
 * @brief Fixed-capacity coherent output from one source poll.
 *
 * Reporting copies measurements into owned storage and rejects duplicate
 * capabilities. The manager commits or discards the complete batch.
 */
class MeasurementBatch {
  public:
    /** @brief Remove all previously reported measurements. */
    void clear();

    /**
     * @brief Copy one unique validated measurement into the batch.
     * @param measurement Measurement to copy.
     * @return True on success, false when invalid, duplicate, or full.
     */
    bool report(const EnvironmentMeasurement& measurement);

    /**
     * @brief Return the number of measurements currently stored.
     * @return Number of measurements currently stored.
     */
    size_t size() const;

    /**
     * @brief Access one stored measurement.
     * @param index Zero-based index less than size().
     * @return Read-only reference valid until the batch is modified.
     */
    const EnvironmentMeasurement& operator[](size_t index) const;

  private:
    std::array<EnvironmentMeasurement, kMaxMeasurementsPerPoll> measurements_{};
    size_t count_ = 0U;
};

/**
 * @brief Process-lifetime environmental hardware, simulation, or test source.
 *
 * init() may create permanent resources. probe(), activate(), and poll() run
 * after initialization and must not allocate or free heap memory.
 */
class EnvironmentSource {
  public:
    EnvironmentSource(const EnvironmentSource&) = delete;
    EnvironmentSource& operator=(const EnvironmentSource&) = delete;
    EnvironmentSource(EnvironmentSource&&) = delete;
    EnvironmentSource& operator=(EnvironmentSource&&) = delete;
    virtual ~EnvironmentSource() = default;
    /**
     * @brief Create or register permanent source resources.
     * @return ESP_OK when permanent resources are ready, otherwise an error code.
     */
    virtual esp_err_t init() = 0;

    /**
     * @brief Check whether the configured source is currently available.
     * @return True when configured hardware is currently available.
     */
    virtual bool probe() = 0;

    /**
     * @brief Configure a detected source for polling.
     * @return ESP_OK when the detected source is configured for polling.
     */
    virtual esp_err_t activate() = 0;

    /**
     * @brief Perform one logical acquisition and report a coherent batch.
     * @param batch Empty manager-owned batch receiving copied measurements.
     * @return ESP_OK on complete success, otherwise an ESP-IDF error code.
     */
    virtual esp_err_t poll(MeasurementBatch& batch) = 0;

  protected:
    EnvironmentSource() = default;
};

} // namespace redmole::environment
