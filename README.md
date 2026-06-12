# RedMole

## About the project

RedMole is a smart home display that helps you make better decisions about energy use and daily life. It shows live spot-prices and a weather forecast from your LEOP server, alongside indoor environmental readings — temperature, relative humidity, and barometric pressure.

The goal is to help you:
- Choose the right times to run high-consumption appliances based on spot-prices
- Know when outdoor conditions are suitable for drying clothes or whether to bring an umbrella
- Monitor and regulate your indoor environment

**Hardware:**
- MCU: ESP32-S3 (dual core, up to 240 MHz, BLE 5)
- Flash: 16 MB (quad), PSRAM: 8 MB embedded
- Display: RGB LCD with capacitive touch
- Sensor: BME280 over I2C

## Pre-requisites and dependencies

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/) v5.1 or later
- Python 3.8+ (required by ESP-IDF tooling)

Third-party dependencies are declared in `main/idf_component.yml` and fetched automatically by the ESP-IDF component manager on first build:

| Dependency | Version |
|---|---|
| lvgl/lvgl | `>= 8.3.9, < 9` |
| espressif/cjson | `^1.7.19` |

LVGL is vendored locally under `components/lvgl_port/` and does not require a network fetch.

## Building and running

### Setting configuration

A `sdkconfig.defaults` is included in the repo as a starting point and will be picked up automatically on first build. To review or adjust settings before building:

```bash
idf.py menuconfig
```

### Build and flash

Set the target, build, flash, and monitor:

```bash
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

If the board is unresponsive, hold `BOOT`, reconnect or reset, flash, then release `BOOT`.

## Tests

Component-level tests live under each component's `test/` directory (e.g. `components/nac/test/`). Each test suite is a self-contained ESP-IDF project that flashes directly to the device. Refer to the `README.md` inside each `test/` directory for build and run instructions.

## License

MIT License — open school project. See `LICENSE` for details.
