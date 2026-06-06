# Environment Measurements

`environment_measurements` owns one polling task and the latest coherent indoor
sample. Its `EnvironmentSensors` composition owns the fixed product sensor list.

## Runtime Flow

Initialization:

1. initialize `board_i2c`
2. initialize every sensor in the fixed product sensor list
3. create the fixed synchronization objects

Polling task:

1. acknowledge a requested stop and wait for the next start
2. read each sensor in the fixed product sensor list
3. publish a complete indoor sample when the read succeeds
4. invalidate the indoor sample and log the named sensor when its read fails
5. wait for the configured reading interval

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

## BME280 Driver Coverage

The BME280 driver supports every sensor control available through this
product's I2C connection:

- sleep, both forced-mode encodings, and normal mode
- skipped, x1, x2, x4, x8, and x16 oversampling for each channel
- disabled, 2, 4, 8, and 16 IIR filter coefficients
- every normal-mode standby period
- software reset
- live measuring and calibration-update status
- settings readback and verification
- raw register reads
- full-precision compensated reads in degrees Celsius, pascals, and percent
- application reads converted to the environment module's stable deci-unit API

Skipped channels are reported as absent instead of being converted into false
measurements. Temperature must be enabled when pressure or humidity is enabled
because the BME280 compensation formulas require it.

The Waveshare breakout also exposes SPI pins, but this product connects the
module to the shared board I2C bus. SPI and three-wire SPI therefore require a
different transport and board wiring; they are intentionally outside this I2C
driver.

Reference documents:

- [Bosch BME280 datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf)
- [Waveshare BME280 Environmental Sensor](https://www.waveshare.com/wiki/BME280_Environmental_Sensor)

## Adding Hardware

Implement the private `EnvironmentSensor` interface:

```cpp
class EnvironmentSensor {
  public:
    virtual esp_err_t init() = 0;
    virtual esp_err_t read(EnvironmentSensorReading& reading) = 0;
};
```

Then add the concrete sensor object and its explicit name, location, and address
to `EnvironmentSensors`.

`EnvironmentSensorReading` marks temperature, humidity, and pressure
individually as present. A future sensor may therefore report only one or two
values without pretending to provide a complete BME280-style sample.

```cpp
struct EnvironmentSensorBinding {
    const char* name;
    EnvironmentLocation location;
    EnvironmentSensor* sensor;
    bool failed;
};
```

The hardware sensor only knows how to communicate with its device.
`EnvironmentSensors` supplies product meaning such as `"indoor BME280"` and
`EnvironmentLocation::Indoor`. `EnvironmentMeasurements` owns scheduling,
logging, and publication through the C API.

The BME280 driver deliberately retries its full initialization sequence after a
failed read. This is the only recovery state in the driver and allows a sensor
to recover after being disconnected and reconnected.

## Public API

The public C API initializes, starts, stops, and copies the latest sample:

- `environment_measurements_init()`
- `environment_measurements_start()`
- `environment_measurements_stop()`
- `environment_measurements_deinit()`
- `environment_measurements_get_latest()`
- `environment_measurements_is_fresh()`
- `environment_measurements_get_update_count()`
