# Sensor Data Component

`sensor_data` is a single-instance application data owner.

It is not a HAL.
It does not talk to hardware directly.

## Responsibility

This module owns the latest published local sensor sample and exposes copy-out read APIs.

Current responsibilities:

- own the latest local sample
- support single-writer / many-reader access
- provide copy-out snapshot reads
- track freshness and update count
- store measurements as scaled integers

## Intended Write Path

Current intended path:

1. `bme280_hal` produces a measurement
2. the sensor task translates it into `sensor_data_sample`
3. the task calls `sensor_data_submit_local(...)`
4. readers call `sensor_data_get_latest_local(...)`

That keeps ownership centralized inside this module.

## Measurement Format

The local sample uses scaled integers:

- temperature: deci-C
- humidity: deci-percent
- pressure: deci-hPa

Examples:

- `112` means `11.2 C`
- `503` means `50.3 %`
- `10132` means `1013.2 hPa`

## Snapshot Pattern

Reads use a versioned snapshot pattern:

1. reader loads version
2. if version is odd, a write is in progress, so retry
3. reader copies the sample out
4. reader loads version again
5. if the version stayed the same, the snapshot is valid

Meaning:

- odd = write in progress
- even = stable published snapshot

## What Does Not Belong Here

This module should not directly own:

- BME280 bus or chip access
- Wi-Fi or HTTP state
- GUI rendering
- history buffers or graph data
- unrelated application state
