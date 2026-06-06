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

The active build is focused on local environment measurement to GUI bring-up.

Current runtime behavior:

- console output uses `USB Serial/JTAG`
- PSRAM is enabled because the RGB panel frame buffers need it
- `environment_measurements` owns the configured environment sensor, polling task, and latest data
- `main.c` owns app-layer orchestration
- the Wi-Fi module code is still present in the repo
- Wi-Fi behavior is driven through the current NAC/app binding path

## Active Runtime Flow

Current active path:

`environment_measurements -> app_gui_bindings -> gui`

Startup flow:

1. `main.c` initializes single-instance modules:
   `rm_nvs`, `task_scheduler`, `nac`, `http_client`, `environment_measurements`
2. `main.c` initializes runtime modules:
   `gui`
3. `main.c` starts `environment_measurements`
4. `main.c` runs the GUI bindings and task scheduler from the app loop

## Module Notes

- `rm_nvs` is a single-instance NVS wrapper with a fixed default namespace.
- `board_i2c` owns the shared ESP32-S3 I2C master bus configuration.
- `environment_measurements` owns board-local environment readings and fallback simulation.
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
