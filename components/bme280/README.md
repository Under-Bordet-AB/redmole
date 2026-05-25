# BME280 Component

`bme280` owns the BME280 sensor protocol.

It exposes one public API:

- `bme280_hal`

The public HAL stays stable while the selected backend changes underneath it.

## Responsibility

This component owns:

- BME280 address selection
- BME280 chip ID verification
- register reads and writes
- reset and measurement configuration
- factory calibration loading
- raw ADC readout
- Bosch compensation math
- conversion into scaled integer measurements

It does not own:

- I2C bus creation
- the polling FreeRTOS task
- app-wide latest sample storage
- GUI state
- sea-level pressure correction
- user calibration offsets

## Runtime Flow

Hardware path:

```text
board_i2c -> hardware backend -> bme280_hal -> local_sensor_service -> sensor_data
```

Simulator path:

```text
sim backend -> bme280_hal -> local_sensor_service -> sensor_data
```

Everything above `bme280_hal` uses the same public API in both modes.

## Hardware Backend

The hardware backend:

1. probes `0x76`
2. probes `0x77` if `0x76` is absent
3. creates an I2C device handle through `board_i2c`
4. reads chip ID register `0xD0`
5. requires chip ID `0x60`
6. resets the chip
7. waits until calibration copy is complete
8. reads calibration registers
9. configures normal measurement mode
10. reads raw pressure, temperature, and humidity registers
11. applies compensation formulas

I2C presence alone is not considered enough. The backend only treats the device as a real BME280 after chip ID verification.

## Missing Hardware

Missing hardware is not fatal.

If the BME280 is absent during init, `bme280_hal_init()` still succeeds so the rest of the firmware can run in degraded mode.

Later reads retry initialization. This allows the sensor to be plugged in after boot.

## Compile-Time Switch

Use:

```bash
idf.py menuconfig
```

Then open:

```text
RedMole Sensor Configuration -> BME280 HAL backend
```

Current choices:

- `CONFIG_REDMOLE_BME280_BACKEND_HW`
- `CONFIG_REDMOLE_BME280_BACKEND_SIM`

Default is hardware backend.

## Measurement Format

Measurements use scaled integers:

- temperature: deci-C
- humidity: deci-percent
- pressure: deci-hPa

Examples:

- `237` means `23.7 C`
- `453` means `45.3 %`
- `10134` means `1013.4 hPa`

The pressure value is local/station pressure. Sea-level correction belongs in an application calibration layer, not in the BME280 driver.

## Backend State

`bme280_hal` is caller-owned, but backend state is private.

The public struct contains an opaque `backend_state` pointer. Only backend implementation files should access it.

This keeps simulator state and hardware state out of the public API while preserving a simple caller-owned context.

## Validation

The compensation logic should be validated against Bosch reference code or known calibration/raw test vectors.

Good validation targets:

- chip ID is `0x60`
- calibration registers are non-zero and stable
- temperature roughly matches a nearby reference after thermal settling
- pressure plus altitude correction matches local weather service trends
- humidity is plausible but should not be treated as calibrated lab data
