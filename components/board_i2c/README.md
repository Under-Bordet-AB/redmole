# Board I2C Component

`board_i2c` owns the shared I2C bus on the Waveshare ESP32-S3 display board.

It is a board-support component, not a sensor driver.

## Responsibility

This component owns:

- the ESP-IDF I2C master bus configuration
- SDA/SCL pin selection
- bus creation and teardown
- shared low-level read/write helpers
- diagnostic I2C scans
- simple address probes

Current board bus:

- SDA: `GPIO8`
- SCL: `GPIO9`
- Port: `I2C_NUM_0`
- Default speed: `400 kHz`

Known devices on this bus:

- `0x24`: IO expander
- `0x5d` or `0x14`: GT911 touch controller
- `0x76` or `0x77`: BME280 on the external I2C header

## Public API Shape

Use `board_i2c_init()` when a module needs the bus ready.

Use `board_i2c_get_bus()` only when an ESP-IDF API needs the raw bus handle, for example the GT911 LCD/touch panel IO setup.

Use `board_i2c_add_device()` to create a device handle for normal register access.

Use `board_i2c_scan()` only for diagnostics and bring-up. Normal runtime code should not rely on full-bus scans.

## Device Handles

Current behavior:

- `board_i2c_add_device()` creates a new device handle
- callers must avoid adding the same address repeatedly

Planned direction:

- `board_i2c` should own a small address registry
- repeated requests for the same address and speed should return the existing handle
- repeated requests for the same address with a different speed should fail with `ESP_ERR_INVALID_STATE`

This keeps duplicate-device policy in one place instead of spreading idempotency checks across drivers.

## What Does Not Belong Here

This component should not own:

- BME280 chip ID checks
- BME280 calibration or compensation
- GT911 touch parsing
- IO expander behavior
- sensor samples
- GUI state

It should provide board-level bus access only.
