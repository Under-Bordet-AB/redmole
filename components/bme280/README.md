# BME280 Component

This component exposes one public local sensor API:

- `bme280_hal`

The public HAL stays the same while the private backend changes underneath it.

## Current Status

The hardware backend is not implemented yet.

Today:

- `bme280_hal_init()` initializes the selected backend
- `bme280_hal_read()` returns scaled integer measurements
- the sim backend generates plausible fake measurements
- the hardware backend exists as named internal stubs

This is intentional. The higher layers can already be built and tested before the real BME280 bus/device code exists.

## Public Shape

The app only talks to:

- `bme280_hal_init(...)`
- `bme280_hal_read(...)`
- `bme280_hal_get_period_ms(...)`
- `bme280_hal_deinit(...)`

The app does not call a simulator directly.

## Internal Shape

Private backend files live under:

- `components/bme280/src/hal_internal/`

Current internal backends:

- `bme280_hal_backend_sim.c`
- `bme280_hal_backend.c`

Current flow:

- sim path: `sim backend -> bme280_hal -> sensor_data`
- hardware path: `hardware -> hardware backend -> bme280_hal -> sensor_data`

So everything above the HAL stays the same.

## Compile-Time Switch

Use:

`idf.py menuconfig`

Then open:

`RedMole Sensor Configuration -> BME280 HAL backend`

Current choices:

- `CONFIG_REDMOLE_BME280_BACKEND_SIM`
- `CONFIG_REDMOLE_BME280_BACKEND_HW`

Default is sim backend.

Current behavior:

- sim backend: `bme280_hal_read()` returns fake measurements
- hardware backend: `bme280_hal_read()` calls the hardware backend

Because the hardware backend is still stubbed, hardware mode compiles but logs read failures until the real implementation is added.

## Measurement Format

Measurements use scaled integers:

- temperature: deci-C
- humidity: deci-percent
- pressure: deci-hPa

Examples:

- `112` means `11.2 C`
- `503` means `50.3 %`
- `10132` means `1013.2 hPa`

## Intended Hardware Work

When real hardware support is added, keep the ownership boundaries the same:

- HAL owns bus/device details
- app code polls the public HAL
- `sensor_data` owns the latest published local sample

The hardware backend should eventually own:

- bus/device configuration details
- chip communication
- calibration reads
- raw measurement conversion
- hardware error reporting

It should not own:

- app-wide latest sample storage
- GUI state
- API data
- graph/history ownership

The expected hardware backend work is:

1. initialize the chosen bus
2. initialize the BME280 device
3. perform chip-id / reset / calibration setup
4. read raw measurements
5. convert raw measurements into `bme280_measurement`
