# Environment Measurements

`environment_measurements` owns one configured environment sensor, one polling
task, and one latest coherent sample.

## Runtime Flow

Initialization:

1. initialize `board_i2c`
2. initialize the configured sensor
3. create the fixed synchronization objects

Polling task:

1. read the configured sensor
2. publish the complete sample when the read succeeds
3. invalidate the latest sample and log the ESP error when the read fails
4. wait for the configured reading interval

There is no runtime source discovery, priority selection, plug-and-play state
machine, history storage, or automatic simulator fallback.

## Selecting The Sensor

Select exactly one source in Kconfig:

- `REDMOLE_ENVIRONMENT_SOURCE_BME280`: BME280 hardware
- `REDMOLE_ENVIRONMENT_SOURCE_SIMULATOR`: explicit simulated values

The simulator is intended for builds that deliberately need simulated data. A
hardware failure never silently changes the source to simulation.

Hardware builds also explicitly select address `0x76` or `0x77`. This board
defaults to `0x77`.

## Adding Hardware

Implement the private `EnvironmentSensor` interface:

```cpp
class EnvironmentSensor {
  public:
    virtual esp_err_t init() = 0;
    virtual esp_err_t read(environment_measurement_sample_t& sample) = 0;
};
```

Then add a Kconfig source option and select the concrete sensor type in
`src/environment_measurements.cpp`.

## Public API

The public C API initializes, starts, stops, and copies the latest sample:

- `environment_measurements_init()`
- `environment_measurements_start()`
- `environment_measurements_stop()`
- `environment_measurements_deinit()`
- `environment_measurements_get_latest()`
- `environment_measurements_is_fresh()`
- `environment_measurements_get_update_count()`
