# GUI Module

Temporary LVGL screen module for the Waveshare `ESP32-S3-LCD-7B` board.

## Current Role

The GUI module:

- initializes the board RGB LCD with the current Waveshare timing and pin map
- enables the LCD backlight through the board I2C IO expander
- starts LVGL on the LCD
- reads the latest local `sensor_data` snapshot
- renders temperature, humidity, pressure, and freshness/update count

This module is a display consumer only.
It does not talk to the BME280 directly.

## Ownership Model

`gui` is currently a single-instance module.

The public API is:

- `gui_init()`
- `gui_start()`
- `gui_is_ready()`
- `gui_deinit()`

All GUI runtime state is owned privately inside the module.
`main.c` does not own a GUI context object.

## Data Source

Current active path:

`selected BME280 HAL backend -> bme280_hal -> sensor_data -> gui`

The GUI reads from `sensor_data`.
It does not own sensor state and it does not keep borrowed pointers into sensor storage.

## Backend Switching

The app always calls `bme280_hal_read()`.

Which values appear on screen depends on the selected HAL backend:

- sim backend: GUI shows simulated values
- hardware backend: GUI shows real values once the hardware backend is implemented

Compile-time selection is in:

`idf.py menuconfig -> RedMole Sensor Configuration -> BME280 HAL backend`

## Measurement Format

The GUI renders scaled integer values from `sensor_data`:

- temperature: deci-C
- humidity: deci-percent
- pressure: deci-hPa

Example:

- `112` is rendered as `11.2 C`

## Current Limits

This module still mixes several layers for bring-up convenience:

- board pin/timing setup
- LCD panel setup
- LVGL integration
- screen creation
- label formatting

That is acceptable for current bring-up, but later it should be split more cleanly.
