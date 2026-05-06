# Local Sensor Service Component

`local_sensor_service` owns the runtime polling of the board-local environmental
sensor.

It is application service code. It is not an I2C driver, not a BME280 register
driver, and not a GUI module.

## Responsibility

This component owns:

- the application-level BME280 HAL instance
- the FreeRTOS polling task
- the polling period chosen by the HAL backend
- conversion from `bme280_measurement` to `sensor_data_sample`
- publishing successful readings into `sensor_data`
- console status logs when the local BME280 appears or disappears

It does not own:

- I2C bus setup
- BME280 register access
- Bosch compensation math
- global sensor history
- UI rendering
- sea-level pressure correction
- user calibration offsets

## Runtime Flow

The local sensor path is:

```text
board_i2c -> bme280 -> local_sensor_service -> sensor_data -> consumers
```

`local_sensor_service` is the boundary where hardware-specific readings become
application sensor samples.

That means:

- `bme280` can focus on talking to the chip
- `sensor_data` can stay sensor-agnostic
- `main.c` only starts the service instead of owning polling logic

## Startup Behavior

Call order:

1. `local_sensor_service_init()`
2. `local_sensor_service_start()`

`init` prepares the HAL instance and service state.

`start` creates the polling task.

The service tolerates a missing BME280. If the sensor is not present at boot, the
firmware keeps running and the task retries later reads. This supports real
board bring-up cases where the external sensor is unplugged, connected late, or
temporarily has a bad contact.

## Missing Sensor Behavior

When the BME280 is not available:

- no new sample is published to `sensor_data`
- the latest old sample remains in `sensor_data`
- freshness checks are left to consumers through `sensor_data_is_local_fresh()`
- the console logs the missing/detected transition

This is intentional. `sensor_data` stores measurements; it does not store
physical device health.

## What Belongs Here Later

Good future additions:

- a service health API such as `local_sensor_service_get_status()`
- counters for read failures and successful reads
- last error reporting for diagnostics
- application-level calibration offsets
- sea-level pressure calculation using configured altitude

Those features belong above the BME280 driver because they are product behavior,
not chip protocol.
