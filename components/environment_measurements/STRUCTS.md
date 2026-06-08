# Environment Measurements Data Structures

This document shows every `struct` defined by the `environment_measurements`
component and how data moves between them. It also includes the enums and
classes that give the structs their meaning.

## Data Flow

The current production configuration has one `Bme280Sensor`, one
`Bme280Producer`, and one `ProducerRegistration`. A successful sensor
acquisition becomes three logical measurements that are published together.

## Structure Summary

| Struct | Layer | Purpose |
|---|---|---|
| `Temperature` | Compensated sensor value | Temperature in milli-degrees Celsius. |
| `Humidity` | Compensated sensor value | Relative humidity in milli-percent. |
| `Pressure` | Compensated sensor value | Pressure in pascals. |
| `Bme280Settings` | BME280 driver | Complete retained sensor configuration. |
| `Bme280Status` | BME280 driver | Live sensor work flags. |
| `Bme280RawSample` | BME280 driver | Uncompensated register values. |
| `Bme280Reading` | BME280 driver | One complete compensated acquisition. |
| `Bme280Calibration` | BME280 driver | Factory coefficients used for compensation. |
| `Measurement` | Generic measurement layer | One logical channel and canonical integer value. |
| `MeasurementBatch` | Generic measurement layer | Fixed-capacity result of one producer acquisition. |
| `MeasurementRecord` | Storage layer | Stored value and runtime metadata for one channel. |
| `ProducerRegistration` | Manager configuration | Connects a producer to the channels it owns. |
| `environment_measurement_sample_t` | Public C API | Application-facing combined indoor sample. |

## Complete Connection View

```text
Bme280Sensor
  owns:
    Bme280Settings settings_
    Bme280Calibration calibration_
  temporarily creates:
    Bme280Status       while waiting for the sensor
    Bme280RawSample    while reading registers
  outputs:
    Bme280Reading
      -> Temperature
      -> Humidity
      -> Pressure

Bme280Producer
  references:
    Bme280Sensor
  converts Bme280Reading into:
    MeasurementBatch
      -> Measurement[channel = IndoorAmbientTemperature, value = milli_c]
      -> Measurement[channel = IndoorRelativeHumidity, value = milli_pct]
      -> Measurement[channel = IndoorPressure, value = pa]

ProducerRegistration
  references:
    Bme280Producer through MeasurementProducer&
    array of the three owned MeasurementChannel values

MeasurementsManager
  references:
    array of ProducerRegistration
    MeasurementStore
  reads MeasurementBatch, validates channel ownership, adds one timestamp,
  and publishes or invalidates all registered channels.

MeasurementStore
  owns:
    one MeasurementRecord per MeasurementChannel

Public C API
  copies the three MeasurementRecord values and converts them into:
    environment_measurement_sample_t
```

## Compensated Reading Structs

Defined in `src/reading_types.hpp`.

### `Temperature`

```cpp
struct Temperature {
    int64_t milli_c;
};
```

| Field | Unit | Example |
|---|---|---|
| `milli_c` | 0.001 degrees Celsius | `23125` means `23.125 C`. |

### `Humidity`

```cpp
struct Humidity {
    int64_t milli_pct;
};
```

| Field | Unit | Example |
|---|---|---|
| `milli_pct` | 0.001 percent relative humidity | `45321` means `45.321 % RH`. |

### `Pressure`

```cpp
struct Pressure {
    int64_t pa;
};
```

| Field | Unit | Example |
|---|---|---|
| `pa` | pascals | `101340` means `1013.4 hPa`. |

These wrappers prevent the BME280 driver from accidentally mixing compensated
values with different units. They exist only in the sensor-specific part of
the module. `Bme280Producer` unwraps them into generic `int64_t` values.

## BME280 Driver Structs

Defined in `src/bme280/bme280_sensor.hpp`.

### `Bme280Settings`

```cpp
struct Bme280Settings {
    Bme280Mode mode;
    Bme280Oversampling temperature_oversampling;
    Bme280Oversampling pressure_oversampling;
    Bme280Oversampling humidity_oversampling;
    Bme280Filter filter;
    Bme280Standby standby;
};
```

`Bme280Sensor` owns one copy as `settings_`. It applies the settings during
initialization and reapplies them after recovery from a sensor failure.

| Field | Type | Meaning |
|---|---|---|
| `mode` | `Bme280Mode` | Sleep, one-shot forced, or continuous normal acquisition. |
| `temperature_oversampling` | `Bme280Oversampling` | Temperature conversion averaging. |
| `pressure_oversampling` | `Bme280Oversampling` | Pressure conversion averaging. |
| `humidity_oversampling` | `Bme280Oversampling` | Humidity conversion averaging. |
| `filter` | `Bme280Filter` | Temperature and pressure IIR filter coefficient. |
| `standby` | `Bme280Standby` | Delay between acquisitions in normal mode. |

### `Bme280Status`

```cpp
struct Bme280Status {
    bool measuring;
    bool updating_calibration;
};
```

This is a temporary decoded view of the BME280 status register. It is used by
`Bme280Sensor::wait_until_ready()`.

### `Bme280RawSample`

```cpp
struct Bme280RawSample {
    int32_t adc_temperature;
    int32_t adc_pressure;
    int32_t adc_humidity;
};
```

This contains raw, uncompensated ADC values read from the sensor. The values
are not usable as physical measurements until combined with
`Bme280Calibration`.

### `Bme280Reading`

```cpp
struct Bme280Reading {
    Temperature temperature;
    Humidity humidity;
    Pressure pressure;
};
```

This is one complete compensated physical acquisition. The three values stay
together until `Bme280Producer` converts them into a `MeasurementBatch`.

### `Bme280Calibration`

```cpp
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
```

`Bme280Sensor` owns this as `calibration_`. It is loaded from factory-programmed
sensor registers after every reset and used with `Bme280RawSample` to produce a
`Bme280Reading`.

| Prefix | Used to compensate |
|---|---|
| `dig_T*` | Temperature and the shared `t_fine` intermediate. |
| `dig_P*` | Pressure. |
| `dig_H*` | Relative humidity. |

## Generic Measurement Structs

Defined in `src/measurement_types.hpp`.

### Supporting `MeasurementChannel` Enum

```cpp
enum class MeasurementChannel : uint8_t {
    IndoorAmbientTemperature,
    IndoorRelativeHumidity,
    IndoorPressure,
    Count,
};
```

A channel identifies the semantic meaning and canonical unit of a generic
measurement. `Count` is a sentinel used to size and validate arrays; it is not
a real channel.

| Channel | Canonical internal unit |
|---|---|
| `IndoorAmbientTemperature` | milli-degrees Celsius |
| `IndoorRelativeHumidity` | milli-percent relative humidity |
| `IndoorPressure` | pascals |

### `Measurement`

```cpp
struct Measurement {
    MeasurementChannel channel;
    int64_t value;
};
```

The channel determines what the otherwise generic integer means. For example,
`{IndoorPressure, 101340}` means `101340 Pa`.

### `MeasurementBatch`

```cpp
struct MeasurementBatch {
    std::array<Measurement, kMaxMeasurementsPerBatch> measurements = {};
    size_t count = 0U;
};
```

| Field | Meaning |
|---|---|
| `measurements` | Fixed-capacity storage for up to four logical measurements. |
| `count` | Number of populated entries starting at index zero. |

One batch represents one coherent producer acquisition. The current
`Bme280Producer` fills indexes 0 through 2 with temperature, humidity, and
pressure, then sets `count` to `3`.

The manager rejects empty, oversized, duplicate-channel, or unregistered-channel
batches. The store also validates batches before publishing them.

### `MeasurementRecord`

```cpp
struct MeasurementRecord {
    int64_t timestamp_ms = 0;
    int64_t value = 0;
    uint64_t publication_version = 0U;
    bool valid = false;
};
```

| Field | Meaning |
|---|---|
| `timestamp_ms` | Monotonic milliseconds when the manager published the batch. |
| `value` | Channel value in that channel's canonical internal unit. |
| `publication_version` | Shared version assigned to every value from one published batch. |
| `valid` | Whether consumers may use this stored channel. |

`MeasurementStore` owns an array containing one `MeasurementRecord` for each
real `MeasurementChannel`. All three records from one BME280 batch receive the
same timestamp and publication version.

## Manager Configuration Struct

Defined in `src/measurements_manager.hpp`.

### `ProducerRegistration`

```cpp
struct ProducerRegistration {
    const char* diagnostic_name;
    MeasurementProducer& producer;
    const MeasurementChannel* channels;
    size_t channel_count;
};
```

| Field | Ownership and purpose |
|---|---|
| `diagnostic_name` | Non-owned process-lifetime string used for failure and recovery logs. |
| `producer` | Reference to the producer that performs acquisitions. |
| `channels` | Non-owned process-lifetime array of channels the producer may publish. |
| `channel_count` | Number of entries in `channels`. |

The registration is the connection between a producer and the generic
measurement system. `MeasurementsManager` uses it to validate every returned
batch and to know which channels to invalidate after a producer failure.

The production registration connects `"indoor BME280"` and
`s_indoor_producer` to all three indoor channels.

## Public API Struct

Defined in `include/environment_measurements.h`.

### `environment_measurement_sample_t`

```c
typedef struct {
    int64_t timestamp_ms;
    int32_t temperature_deci_c;
    int32_t humidity_deci_pct;
    int32_t pressure_deci_hpa;
    bool valid;
} environment_measurement_sample_t;
```

| Field | Unit and source |
|---|---|
| `timestamp_ms` | Oldest timestamp among the three copied `MeasurementRecord` values. |
| `temperature_deci_c` | `IndoorAmbientTemperature` converted from milli-Celsius to deci-Celsius. |
| `humidity_deci_pct` | `IndoorRelativeHumidity` converted from milli-percent to deci-percent. |
| `pressure_deci_hpa` | `IndoorPressure` converted from pascals to deci-hectopascals. |
| `valid` | True only when every required stored channel is valid, fresh, and convertible. |

This is the only struct exposed to application code. It combines three generic
stored channels into the shape expected by GUI, UART, and other C consumers.

## Classes That Own And Transform The Structs

These are classes rather than `struct`s, but they define the important
relationships between the data structures.

| Class | Owns or references | Input | Output |
|---|---|---|---|
| `Bme280Sensor` | Owns `Bme280Settings` and `Bme280Calibration`. | BME280 registers. | `Bme280Status`, `Bme280RawSample`, and finally `Bme280Reading`. |
| `Bme280Producer` | References one `Bme280Sensor` and stores three channel IDs. | `Bme280Reading`. | `MeasurementBatch`. |
| `MeasurementsManager` | References registrations and store; owns task state. | Producer `MeasurementBatch`. | Published or invalidated store channels. |
| `MeasurementStore` | Owns one `MeasurementRecord` per channel and a publication counter. | Validated `MeasurementBatch`. | Atomic copies of requested `MeasurementRecord` values. |

`MeasurementProducer` is the abstract interface used by
`ProducerRegistration`. `Bme280Producer` implements it, allowing the manager to
poll producers without knowing their hardware type.

## Success And Failure Paths

### Successful Poll

```text
Bme280RawSample + Bme280Calibration
  -> Bme280Reading
  -> MeasurementBatch(count = 3)
  -> manager validates against ProducerRegistration
  -> store writes 3 MeasurementRecord values atomically
  -> public API copies and converts all 3 records
  -> environment_measurement_sample_t(valid = true)
```

### Failed Poll

```text
Bme280Sensor or Bme280Producer returns an error
  -> manager uses ProducerRegistration.channels
  -> store sets valid = false on all 3 owned MeasurementRecord values
  -> public API rejects the combined sample
```

The old values remain in each `MeasurementRecord`, but `valid = false` prevents
them from being served.
