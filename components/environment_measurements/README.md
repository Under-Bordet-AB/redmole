# Environment Measurements

`environment_measurements` owns the board's indoor BME280, polls it from a
FreeRTOS task, and exposes the latest complete temperature, humidity, and
pressure sample through a thread-safe C API.

The component stores only the latest sample. It does not keep measurement
history or expose the BME280 driver directly to application code.

## Responsibilities

The component:

- configures and owns one BME280 at address `0x76` or `0x77`
- uses the shared bus provided by `board_i2c`
- reads and compensates the BME280's temperature, humidity, and pressure data
- publishes complete readings in fixed-point units
- invalidates the latest sample after a read failure
- retries full sensor initialization on later reads after a failure
- provides freshness checks and a monotonic publication count

`board_i2c` must be initialized before this component.

## Runtime Flow

```text
BME280 -> board_i2c -> Bme280Sensor -> EnvironmentMeasurements -> public C API
                                                                    |
                                                                    +-> GUI
                                                                    +-> UART
```

`environment_measurements_init()` creates the synchronization objects and
attempts to initialize the BME280. A missing sensor is logged but does not make
module initialization fail. This allows startup to continue and lets later
polling attempts recover when the sensor becomes available.

`environment_measurements_start()` creates a statically allocated FreeRTOS task
on its first call. The task reads the sensor immediately, then waits for the
configured interval before each later read.

A successful read replaces the complete latest sample while holding a mutex and
increments the update count. A failed read marks the stored sample invalid.

## Public API

Include:

```c
#include "environment_measurements.h"
```

| Function | Purpose |
|---|---|
| `environment_measurements_init()` | Initialize synchronization and attempt to initialize the BME280. |
| `environment_measurements_start()` | Start or resume polling. Requires successful module initialization. |
| `environment_measurements_stop()` | Cooperatively stop polling and wait for the task to acknowledge it. |
| `environment_measurements_deinit()` | Stop polling; process-lifetime storage is retained. |
| `environment_measurements_get_latest(out)` | Copy the latest valid sample if it is no older than five seconds. |
| `environment_measurements_is_fresh(max_age_ms)` | Check the latest sample against a caller-selected age limit. |
| `environment_measurements_get_update_count()` | Return the number of successfully published samples. |

Initialization and start are idempotent. Stop and deinit are safe to call
repeatedly.

### Example

```c
esp_err_t result = board_i2c_init();
if (result == ESP_OK) {
    result = environment_measurements_init();
}
if (result == ESP_OK) {
    result = environment_measurements_start();
}

environment_measurement_sample_t sample = {0};
if (environment_measurements_get_latest(&sample)) {
    printf("temperature: %.1f C\n", sample.temperature_deci_c / 10.0);
    printf("humidity: %.1f %%\n", sample.humidity_deci_pct / 10.0);
    printf("pressure: %.1f hPa\n", sample.pressure_deci_hpa / 10.0);
}
```

## Sample Format

`environment_measurement_sample_t` contains:

| Field | Meaning |
|---|---|
| `timestamp_ms` | Monotonic milliseconds since boot from `esp_timer`; not wall-clock time. |
| `temperature_deci_c` | Temperature in tenths of a degree Celsius. |
| `humidity_deci_pct` | Relative humidity in tenths of a percent. |
| `pressure_deci_hpa` | Pressure in tenths of a hectopascal. |
| `valid` | Whether the copied sample is valid and usable. |

For example, `temperature_deci_c = 231` means `23.1 C`.

`environment_measurements_get_latest()` returns `false` when:

- the module has not been initialized
- `out` is `NULL`
- no successful reading exists
- the latest read failed
- the sample is older than the fixed five-second timeout
- the sample timestamp is later than the current monotonic time

When a copied sample is stale, its `valid` field is set to `false`.

## BME280 Behavior

The private BME280 source:

1. adds a device to the shared board I2C bus
2. probes the configured address and verifies chip ID `0x60`
3. resets the sensor and waits until it is ready
4. reads the factory calibration data
5. applies and verifies the configured settings
6. reads raw values and applies the BME280 compensation formulas
7. converts the result to the public fixed-point units

Temperature, humidity, and pressure must all be enabled. The component publishes
only complete readings.

After an acquisition error, the driver marks itself unready. Its next read
attempt performs the complete initialization sequence again, allowing recovery
after a disconnect or transient I2C failure. The manager logs the first failure
and the first later recovery without logging every repeated failure.

## Configuration

Run `idf.py menuconfig` and open `RedMole Sensor Configuration`.

| Setting | Meaning | Default |
|---|---|---|
| `REDMOLE_ENVIRONMENT_READING_INTERVAL_SEC` | Delay between polling attempts, from 1 to 3600 seconds. | `1` |
| `REDMOLE_BME280_ADDRESS_0X76` / `0X77` | BME280 I2C address. | `0x77` |
| `REDMOLE_BME280_MODE` | `0` sleep, `1` forced, `2` alternate forced, `3` normal. | `1` |
| `REDMOLE_BME280_OVERSAMPLING_TEMPERATURE` | Temperature oversampling, `1` through `5` for x1 through x16. | `1` |
| `REDMOLE_BME280_OVERSAMPLING_PRESSURE` | Pressure oversampling, `1` through `5` for x1 through x16. | `1` |
| `REDMOLE_BME280_OVERSAMPLING_HUMIDITY` | Humidity oversampling, `1` through `5` for x1 through x16. | `1` |
| `REDMOLE_BME280_IIR_FILTER` | IIR setting: off or coefficient 2, 4, 8, or 16. | off |
| `REDMOLE_BME280_STANDBY_TIME` | Standby setting used in normal mode. | 1000 ms |

Forced mode is the normal choice because the polling task owns the acquisition
cadence. Sleep mode deliberately produces no successful samples.

## Implementation

| Path | Responsibility |
|---|---|
| `include/environment_measurements.h` | Stable application-facing C API and sample type. |
| `src/environment_measurements.cpp` | Production BME280 composition and C API forwarding. |
| `src/environment_measurements_internal.hpp/.cpp` | Polling task, synchronization, publication, freshness, and injectable controller. |
| `src/temperature_humidity_pressure_source.hpp` | Private interface for a complete sensor source. |
| `src/reading_types.hpp` | Private strongly typed fixed-point values. |
| `src/bme280/bme280_sensor.hpp/.cpp` | BME280 protocol, settings, calibration, compensation, and recovery. |

The controller uses:

- one statically allocated FreeRTOS task
- a static mutex protecting the latest sample
- a static binary semaphore for stop acknowledgement
- atomics for the stop request and update count
- no dynamic measurement history

The internal `TemperatureHumidityPressureSource` keeps sensor acquisition
separate from polling, publication, and freshness behavior.

## Logs

Relevant log tags:

- `ENV_MEASURE`: source initialization, read failure, and recovery
- `ENV_BME280`: BME280 initialization and chip-ID diagnostics
