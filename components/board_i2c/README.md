# Board I2C Component

`board_i2c` owns the shared I2C master bus on the Waveshare ESP32-S3 display
board, initializes it on demand, and exposes board-level device and transaction
helpers through a thread-safe C API.

The component owns bus configuration and lifecycle only. Device-specific
protocols and behavior remain in their own drivers.

## Responsibilities

The component:

- configures the shared ESP-IDF I2C master bus
- owns SDA/SCL pin selection and controller selection
- creates the bus on the first request
- serializes initialization, wrapper operations, and teardown
- creates caller-owned device handles
- provides raw and register-oriented transaction helpers
- provides targeted probes and diagnostic full-bus scans

## Runtime Flow

```text
IO expander ----\
GT911 touch -----+-> board_i2c -> ESP-IDF I2C master driver -> I2C_NUM_0
BME280 ----------/
```

`board_i2c_init()` creates the shared bus on its first successful call.
Repeated calls return successfully without creating another bus.

The public wrapper APIs use one lifecycle lock so initialization and teardown
cannot race wrapper operations. ESP-IDF's I2C master driver separately
serializes individual bus transactions.

`board_i2c_get_bus()` returns the raw shared bus handle for ESP-IDF integrations
such as the GT911 panel IO driver. Code using that raw handle operates outside
the component lifecycle lock, so `board_i2c_deinit()` must only run during
teardown after raw-handle users and device handles are no longer active.

## Public API

Include:

```c
#include "board_i2c.h"
```

| Function | Purpose |
|---|---|
| `board_i2c_init()` | Initialize the shared bus if needed. |
| `board_i2c_get_bus()` | Return the raw shared bus handle for ESP-IDF integrations. |
| `board_i2c_add_device(address, speed_hz, out)` | Create a caller-owned device handle. |
| `board_i2c_read_reg(dev, reg, data, len)` | Read bytes from an 8-bit register address. |
| `board_i2c_write_reg(dev, reg, value)` | Write one byte to an 8-bit register address. |
| `board_i2c_read_reg_u16_le(dev, reg, out)` | Read one little-endian 16-bit register value. |
| `board_i2c_write(dev, data, len)` | Write a raw byte buffer. |
| `board_i2c_read(dev, data, len)` | Read a raw byte buffer. |
| `board_i2c_probe_address(address)` | Check whether one address acknowledges. |
| `board_i2c_bme280_present()` | Check the two expected BME280 addresses. |
| `board_i2c_scan(out_found_count)` | Scan all normal seven-bit device addresses. |
| `board_i2c_deinit()` | Delete the shared bus during controlled teardown. |

Initialization is idempotent. Normal runtime code should prefer targeted probes
over full-bus scans.

### Example

```c
i2c_master_dev_handle_t device = NULL;

esp_err_t result = board_i2c_add_device(0x24, BOARD_I2C_DEFAULT_SPEED_HZ, &device);
if (result == ESP_OK) {
    uint8_t input = 0;
    result = board_i2c_read_reg(device, 0x00, &input, sizeof(input));
}
```

## Bus Configuration

| Setting | Value |
|---|---|
| SDA | `GPIO8` |
| SCL | `GPIO9` |
| Controller | `I2C_NUM_0` |
| Default device speed | `400 kHz` |
| Wrapper transaction timeout | `100 ms` |

Known devices:

| Address | Device |
|---|---|
| `0x24` | IO expander |
| `0x5d` or `0x14` | GT911 touch controller |
| `0x76` or `0x77` | BME280 on the external I2C header |

## Device Handles

`board_i2c_add_device()` creates a new ESP-IDF device handle and transfers
ownership of that handle to the caller. Callers must avoid repeatedly adding
the same address.

The component currently has no address registry. A future registry could return
an existing handle for repeated requests with the same address and speed, and
reject conflicting speed requests with `ESP_ERR_INVALID_STATE`.

## Thread Safety

The lifecycle lock protects:

- bus creation and shared state
- wrapper-based probes, scans, reads, and writes
- device-handle creation
- bus teardown

The ESP-IDF driver protects each physical transaction with its own bus lock.
Neither lock combines multiple public calls into one atomic device operation.

Raw bus users returned by `board_i2c_get_bus()` are not covered by the lifecycle
lock after that function returns. They remain protected from concurrent physical
transactions by the ESP-IDF driver, but teardown must be coordinated by the
application.

## Implementation

| Path | Responsibility |
|---|---|
| `include/board_i2c.h` | Stable board-level I2C C API, configuration, and known-device identifiers. |
| `src/board_i2c.c` | Shared bus lifecycle, locking, probes, scans, and transaction wrappers. |

The component uses:

- one process-lifetime platform lifecycle lock
- one shared ESP-IDF I2C master bus handle
- ESP-IDF's internal per-bus transaction lock
- no address registry
- no device-specific protocol state

## Boundaries

The component does not own:

- BME280 chip ID checks
- BME280 calibration or compensation
- GT911 touch parsing
- IO expander behavior
- sensor samples
- GUI state

## Logs

Relevant log tag:

- `BOARD_I2C`: bus creation failures, device-registration failures, scan results,
  and teardown failures
