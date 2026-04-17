# Redmole

ESP-IDF firmware project for the Redmole device.

## Confirmed Hardware

Current confirmed hardware from `esptool` on `COM4`:

- MCU: `ESP32-S3` revision `v0.2`
- package: `QFN56`
- CPU: dual core plus LP core, up to `240 MHz`
- wireless: Wi-Fi and Bluetooth LE 5
- crystal: `40 MHz`
- USB mode: `USB-Serial/JTAG`
- flash: `16 MB`
- embedded PSRAM: `8 MB`
- flash bus mode from eFuse: quad
- flash voltage from eFuse: `3.3 V`

Current checked-in defaults match that target:

- `sdkconfig.defaults` targets `16MB` flash
- the project uses the large single-app partition layout
- PSRAM is enabled for the RGB GUI path

## Dependencies

This project uses the ESP-IDF Component Manager for third-party dependencies.

Repo policy:

- commit `idf_component.yml`
- commit `dependencies.lock`
- do not commit `managed_components/`

Typical flow:

1. pull the repo
2. run `idf.py build`
3. let ESP-IDF fetch `managed_components/` locally

## Current Bring-Up State

The active build is focused on local sensor-to-GUI bring-up.

Current runtime behavior:

- console output uses `USB Serial/JTAG`
- PSRAM is enabled because the RGB panel frame buffers need it
- the simulated BME280 backend is the default local sensor path
- `main.c` directly owns the current bring-up flow
- the Wi-Fi module code is still present in the repo
- Wi-Fi startup from `main.c` is currently commented out on purpose
- `rm_event_notify` gates task startup

The old Wi-Fi startup block is kept in `main.c` as commented reference code so it can be re-enabled later without changing the Wi-Fi module itself.

## Active Runtime Flow

Current active path:

`selected BME280 HAL backend -> bme280_hal -> sensor task -> sensor_data -> gui`

Startup flow:

1. `main.c` initializes single-instance modules:
   `rm_nvs`, `sensor_data`, `rm_event_notify`
2. `main.c` initializes runtime modules:
   `bme280_hal`, `gui`
3. `main.c` starts the local sensor task
4. `main.c` starts the GUI task if GUI init succeeded
5. tasks signal readiness through `rm_event_notify`
6. `main.c` releases the startup gate

### Startup Gating Behavior

`main.c` currently uses a simple startup gate:

- the sensor task signals `RM_EVENT_NOTIFY_BIT_SENSOR_READY` before entering its main loop
- the GUI task signals `RM_EVENT_NOTIFY_BIT_GUI_READY` before entering its main loop
- both tasks wait on `RM_EVENT_NOTIFY_BIT_INIT_DONE`
- `main.c` waits until all expected ready bits are set
- `main.c` then signals `RM_EVENT_NOTIFY_BIT_INIT_DONE`

That means tasks are created first, but they do not enter their steady-state loops until the expected startup participants have reached the gate.

## Module Notes

- `rm_nvs` is a single-instance NVS wrapper with a fixed default namespace.
- `sensor_data` owns the latest published local sample.
- `bme280_hal` is the public sensor-facing API above the selected backend.
- `gui` is currently a single-instance module that owns its own internal runtime state.
- `wifi_module` matches the main branch implementation, but its startup path is not active in this build.

## Build

Use the ESP-IDF environment, then run:

```bash
idf.py build
```

Flash with:

```bash
idf.py -p COM4 flash monitor
```

If the board is in a bad boot state, hold `BOOT`, connect or reset, flash, then release `BOOT`.
