# GUI Module

LVGL-based GUI module for the Waveshare `ESP32-S3-LCD-7B` board.

## Current Layout

The GUI component is now organized under `components/gui/src` as:

- `gui.c`: public API entrypoints and top-level coordination
- `gui_state.*`: GUI-owned state and pure state transitions
- `gui_screen.*`: screen-facing wrapper over LVGL rendering and event wiring
- `gui_platform.*`: display bring-up, backlight, and refresh timer glue
- `view/`: LVGL widget composition, shared UI helpers, and panel-specific visuals under `view/panels/`
- `gui_defs.h`: shared internal GUI definitions

See `components/gui/src/README.md` for the internal ownership rules and file breakdown.

## Theme Authoring

To add or modify GUI themes, follow the dedicated guide:

- `components/gui/docs/themes.md`
- `components/gui/docs/gui_project_map.md`

## Current Screen

The active screen contains:

- a vertical navigation bar on the left
- a `BME280` panel showing temperature, humidity, and pressure
- a `Settings` panel placeholder

If no live `sensor_data` snapshot is available yet, the BME280 panel waits for the first published reading instead of generating placeholder measurements.

## Public API

The public header is `include/gui_module.h`.

Current API:

- `gui_init(gui_ctx_t *self)`
- `gui_run(gui_ctx_t *self)`
- `gui_deinit(gui_ctx_t *self)`

The module is still effectively single-instance. `gui_ctx_t` carries readiness state plus internal module-owned storage.

The public API also exposes callback bindings plus setters/getters for GUI-owned Wi-Fi state.

## Data Flow

Current data path:

`selected BME280 HAL backend -> bme280_hal -> sensor_data -> gui_state -> gui_screen`

The GUI does not talk to the BME280 directly. At the moment the BME280 values are intentionally left at zero until a real GUI-side sensor integration path is wired in.

## Measurement Format

Rendered values use the scaled integers published by `sensor_data`:

- temperature: deci-C
- humidity: deci-percent
- pressure: deci-hPa

Example:

- `112` is rendered as `11.2 C`
