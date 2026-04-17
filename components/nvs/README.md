# NVS Module

`rm_nvs` is a single-instance wrapper around ESP-IDF NVS.

## Overview

The module:

- owns all internal state
- uses one default namespace configured by `rm_nvs_init()`
- exposes plain `rm_nvs_*` functions with no public context pointer
- commits writes immediately in each setter

## Initialization

Call `rm_nvs_init("app")` before any module that depends on NVS.

This matters for Wi-Fi too, because ESP-IDF Wi-Fi uses NVS internally.
If Wi-Fi starts before NVS is initialized, startup can fail with `ESP_ERR_NVS_NOT_INITIALIZED`.

`rm_nvs_init()` behavior:

- first call initializes NVS and stores the namespace
- repeated call with the same namespace returns `ESP_OK`
- repeated call with a different namespace returns `ESP_ERR_INVALID_STATE`

## Supported Types

The wrapper currently supports:

- `u8`, `i8`
- `u16`, `i16`
- `u32`, `i32`
- `u64`, `i64`
- `str`
- `blob`

It also supports:

- `rm_nvs_erase_key()`
- `rm_nvs_self_test()`

## Notes

- NVS key names and namespace names must stay short. ESP-IDF uses a 15-character limit.
- Immediate read-after-write only proves the API works in the current boot.
- Persistence is only proven after a reboot and successful readback.

Current active build does initialize `rm_nvs` from `main.c`, even though Wi-Fi startup is commented out in this bring-up configuration.
