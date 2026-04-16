# NVS Module

## Overview

`rm_nvs` is a single-instance wrapper around ESP-IDF NVS.

The module:
- owns all internal state
- uses one default namespace configured by `rm_nvs_init()`
- exposes plain `rm_nvs_*` functions with no public context pointer
- commits writes immediately in each setter

## Initialization

Call `rm_nvs_init("app")` once before any module that depends on NVS.

This matters for Wi-Fi too, because ESP-IDF Wi-Fi uses NVS internally. If Wi-Fi starts before NVS is initialized, startup can fail with `ESP_ERR_NVS_NOT_INITIALIZED`.

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

## Temporary Main Test

`main.c` currently contains a temporary persistence test controlled by:

```c
#define NVS_TEST_WRITE 1
```

Workflow:
1. Set `NVS_TEST_WRITE` to `1`
2. Boot once to write the value
3. Change `NVS_TEST_WRITE` to `0`
4. Rebuild and reboot
5. Confirm the stored value is read back

This is temporary application-level test code and can be removed once persistence is verified on hardware.

## Notes

- NVS key names and namespace names must stay short. ESP-IDF uses a 15-character limit.
- Immediate read-after-write only proves the API works in the current boot.
- Persistence is only proven after a reboot and successful readback.
