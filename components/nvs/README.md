# NVS Module

`rm_nvs` owns the application's default NVS namespace and exposes immediate,
typed persistent-storage operations through a single-instance C API.

The component wraps ESP-IDF NVS but does not own the complete default NVS
partition. Other ESP-IDF components, including Wi-Fi, may use that partition
independently.

## Responsibilities

The component:

- initializes the default ESP-IDF NVS partition
- recovers the partition after no-free-page or version-mismatch errors
- owns a validated copy of one application namespace
- opens and closes an NVS handle for every operation
- commits every successful write and erase immediately
- provides typed integer, string, and blob accessors
- provides key-existence and single-key erase helpers
- provides a diagnostic self-test using reserved test keys

## Runtime Flow

```text
Application modules -> rm_nvs -> ESP-IDF NVS -> default NVS flash partition
                              \
                               +-> configured application namespace
```

`rm_nvs_init()` initializes the default NVS partition and copies the selected
namespace into module-owned storage. Repeated calls with the same namespace
return `ESP_OK`; a different namespace returns `ESP_ERR_INVALID_STATE`.

Following the standard ESP-IDF recovery flow, initialization erases the default
NVS partition when it reports no free pages or an incompatible stored version.
That recovery removes values owned by this application and other users of the
default partition.

Each read or write opens the configured namespace, performs one operation, and
closes the handle. Successful setters and erases commit before returning.

`rm_nvs_deinit()` disables this wrapper and clears its namespace copy. It does
not call `nvs_flash_deinit()` or erase stored values because other ESP-IDF
components may still depend on the default NVS partition.

## Public API

Include:

```c
#include "rm_nvs.h"
```

| Function family | Purpose |
|---|---|
| `rm_nvs_init(namespace)` | Initialize NVS and select the application namespace. |
| `rm_nvs_deinit()` | Disable the wrapper without erasing or deinitializing flash. |
| `rm_nvs_set_*()` / `rm_nvs_get_*()` | Write or read integer values. |
| `rm_nvs_set_str()` / `rm_nvs_get_str()` | Write or read null-terminated strings. |
| `rm_nvs_set_blob()` / `rm_nvs_get_blob()` | Write or read binary data. |
| `rm_nvs_key_exists()` | Check whether a key exists. |
| `rm_nvs_erase_key()` | Erase and commit one key. |
| `rm_nvs_self_test()` | Exercise all supported types using reserved test keys. |

### Example

```c
esp_err_t result = rm_nvs_init("app");
if (result == ESP_OK) {
    result = rm_nvs_set_u8("brightness", 80U);
}

uint8_t brightness = 0;
if (result == ESP_OK) {
    result = rm_nvs_get_u8("brightness", &brightness);
}
```

## Supported Values

| Type | Accessors |
|---|---|
| Unsigned integers | `u8`, `u16`, `u32`, `u64` |
| Signed integers | `i8`, `i16`, `i32`, `i64` |
| Strings | `str` |
| Binary data | `blob` |

For string and blob reads, `length` is an input/output parameter. Pass a `NULL`
buffer to query the required size before allocating or selecting a buffer.
String lengths include the null terminator.

## Naming And Persistence

ESP-IDF limits namespace and key names to 15 characters, excluding the null
terminator. `rm_nvs_init()` rejects an empty or overlong namespace before
initializing flash. Key validation is performed by ESP-IDF during each
operation.

Immediate read-after-write verifies the current operation but does not prove
cross-boot persistence. Persistence is proven only after reboot and successful
readback.

## Thread Safety

A lifecycle lock serializes initialization, deinitialization, and access to the
configured namespace. ESP-IDF NVS provides synchronization for concurrent NVS
operations after a handle is opened.

The wrapper does not combine multiple public calls into one atomic operation.
Callers that require read-modify-write semantics must provide higher-level
synchronization.

## Self-Test

`rm_nvs_self_test()` writes, reads, and erases reserved `test_*` keys in the
configured namespace. It removes stale test keys before starting and removes
all test keys after a successful run. A failed run may leave reserved test keys,
which the next run removes before testing.

The self-test proves wrapper behavior in the current boot. It does not prove
cross-boot persistence.

## Implementation

| Path | Responsibility |
|---|---|
| `include/rm_nvs.h` | Stable application-facing persistent-storage C API. |
| `src/rm_nvs.c` | Namespace lifecycle, typed operations, commit behavior, and self-test. |

The component uses:

- one process-lifetime lifecycle lock
- one owned namespace buffer
- one short-lived ESP-IDF NVS handle per operation
- immediate commit for every successful write and erase
- no cached application values

## Logs

Relevant log tag:

- `RM_NVS`: self-test progress, self-test failures, and cleanup warnings
