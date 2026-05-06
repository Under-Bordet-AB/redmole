# Sensor Data Component

`sensor_data` is a single-instance application data store.

It stores the latest local environmental sample. It does not talk to hardware,
own physical sensor status, or know which driver produced the sample.

## Responsibility

This module owns:

- the latest published local sample
- single-writer / many-reader copy-out access
- versioned snapshot reads
- freshness checks based on sample age
- a monotonic local update counter
- scaled integer measurement storage

## Intended Write Path

Current local sensor path:

```text
bme280 -> local_sensor_service -> sensor_data -> consumers
```

The service layer translates a driver measurement into `sensor_data_sample` and
then calls `sensor_data_submit_local(...)`.

Readers call `sensor_data_get_latest_local(...)` to copy out a stable snapshot.

## Measurement Format

The local sample uses scaled integers:

- temperature: deci-C
- humidity: deci-percent
- pressure: deci-hPa

Examples:

- `112` means `11.2 C`
- `503` means `50.3 %`
- `10132` means `1013.2 hPa`

Pressure is stored as local/station pressure. Sea-level correction belongs in a
separate application/calibration layer.

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

## Freshness

`sensor_data_get_latest_local(...)` answers:

```text
Do we have a valid sample?
```

`sensor_data_is_local_fresh(...)` answers:

```text
Is the valid sample recent enough for this consumer?
```

Those are intentionally separate. A sample can be valid but stale if the sensor
was unplugged after a successful reading.

## What Does Not Belong Here

This module should not directly own:

- BME280 bus or chip access
- BME280 presence/missing state
- service health or driver error counters
- Wi-Fi or HTTP state
- GUI rendering
- history buffers or graph data
- sea-level pressure correction
- user calibration offsets
- unrelated application state
