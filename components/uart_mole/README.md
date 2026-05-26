# uart_mole — ESP32-S3 UART Diagnostic Server

This document explains how the `uart_diag` component works: what it does, why it is structured the way it is, and what you need to understand about the ESP32-S3 hardware and FreeRTOS to follow the code.

---

## Overview

The `uart_mole` module turns the ESP32-S3 into a **request/response diagnostic server** over a physical UART connection. A PC client sends a single command byte. The ESP32 receives it, builds a response package containing live system data, and transmits it back in a compact binary format.

The system is designed around three concerns that are kept deliberately separate:

- **Receiving** — a FreeRTOS task that blocks on a UART event queue waiting for incoming bytes
- **Building** — filling a package buffer with data and computing a checksum
- **Transmitting** — a task scheduler callback that writes the bytes to UART

Keeping these three steps apart means the listener never blocks waiting for UART TX to complete, and the transmit path never races with a new incoming command.

---

## 1. UART Hardware on the ESP32-S3

### The hardware FIFO

The ESP32-S3 has three independent UART peripherals: UART0, UART1, and UART2. This module uses **UART0**, whose default IOMUX pins are GPIO43 (TX) and GPIO44 (RX).

Each UART peripheral contains a **hardware FIFO** of exactly 128 bytes baked into silicon. When bytes arrive on the RX pin, the hardware shifts them into this FIFO automatically — no firmware code runs during that process. Similarly, bytes written to the TX FIFO are clocked out by the hardware without CPU involvement.

The FIFO is the boundary between the physical wire and the software world.

### The software ring buffer

The ESP-IDF UART driver installs an ISR (interrupt service routine) that fires when the hardware FIFO reaches a threshold. The ISR drains the FIFO into a **software ring buffer in RAM**, which is where your application code reads from.

```
Wire → RX pin → hardware FIFO (128 B, silicon) → ISR drains → software ring buffer (RAM)
```

The ring buffer must be **strictly larger than the hardware FIFO**. If it were the same size or smaller, the ISR could not guarantee it would always have room to drain a full FIFO load. The minimum meaningful size is 129 bytes; by convention we use 256 (the next power of two) because power-of-two sizes allow the ring buffer index to wrap cheaply using a bitmask instead of a division.

This is why `uart_driver_install` enforces `rx_buffer_size > UART_HW_FIFO_LEN`:

```c
#define UART_MOLE_RX_BUF_SIZE 256   // must be > 128 (UART_HW_FIFO_LEN)
```

Setting it to 128 or less causes `uart_driver_install` to return an error at line 2004 in the driver source.

### The event queue

`uart_driver_install` creates a FreeRTOS queue that the driver posts events to whenever something notable happens — data arrived, a framing error occurred, a buffer overflow happened. The queue handle is passed back to the caller:

```c
uart_driver_install(UART_MOLE_PORT, UART_MOLE_RX_BUF_SIZE, UART_MOLE_TX_BUF_SIZE,
                    20,                    // queue depth
                    &s_uart_mole.queue,    // queue handle returned here
                    0);
```

The listener task blocks on this queue with `xQueueReceive(..., portMAX_DELAY)`. This means the task consumes zero CPU until the driver signals that something happened. This is much better than polling: the FreeRTOS scheduler simply does not schedule the listener until there is work to do.

---

## 2. Module Initialization

`uart_mole_init()` is called once from `main.c` during startup. It does the following in order:

1. **Zeroes the context struct** — `memset` ensures no stale data from a previous boot.
2. **Allocates the JSON buffer on PSRAM** — server responses can be up to 10 KB, which is too large for internal SRAM. `heap_caps_malloc(MALLOC_CAP_SPIRAM)` places the buffer in the external SPI RAM.
3. **Configures UART parameters** — baud rate 115200, 8 data bits, no parity, 1 stop bit, no hardware flow control.
4. **Assigns GPIO pins** — `uart_set_pin` maps UART0 to GPIO43 (TX) and GPIO44 (RX).
5. **Installs the driver** — `uart_driver_install` sets up the ring buffer, event queue, and ISR.
6. **Spawns the listener task** — `xTaskCreate` creates the FreeRTOS task that will wait for commands.

If any step fails, all previously allocated resources are freed before returning the error code. This avoids resource leaks during a failed startup.

---

## 3. The Listener Task

```c
static void uart_mole_listener_task(void *pvParameters)
```

This FreeRTOS task runs forever. Its loop is:

```
wait for UART event → check event type → read command byte → build package → hand to scheduler
```

### Waiting for events

```c
xQueueReceive(s_uart_mole.queue, &event, portMAX_DELAY)
```

`portMAX_DELAY` means the task will sleep indefinitely — it will not be scheduled at all until the UART driver posts an event. This is the correct approach for an I/O-driven task: it does not consume CPU when there is nothing to do.

When the queue receives an event, the task checks `event.type == UART_DATA` and ignores anything else (framing errors, FIFO overflow notifications, etc.).

### Reading the command byte

```c
uart_read_bytes(UART_MOLE_PORT, &cmd, 1, pdMS_TO_TICKS(20))
```

This reads exactly one byte from the software ring buffer with a 20 ms timeout. The command byte is the tag value that identifies which package the client wants.

### Dropping commands while TX is pending

```c
if (s_uart_mole.task_node.active)
{
    ESP_LOGW(TAG, "TX pending, dropping cmd 0x%02x", cmd);
    continue;
}
```

The module uses a **single shared package buffer** (`pkg_buf`). If a previous response has been handed to the scheduler but not yet transmitted, overwriting the buffer would corrupt the in-flight data. The `active` flag on the task node indicates that the scheduler still holds a reference. New commands are dropped until the transmit completes.

This is a deliberate simplicity choice: a more complex design would queue commands, but for a diagnostic tool a dropped command is acceptable — the client can simply retry.

---

## 4. Package Types

Each package type corresponds to a command byte. The mapping is defined by `uart_pkg_tag_t`:

| Tag | Value | Description |
|-----|-------|-------------|
| `UART_STATUS_PKG`  | 0 | WiFi, server, sensor, GUI, SD card online flags + uptime + timestamp |
| `UART_SERVER_PKG`  | 1 | Last HTTP response body as JSON + timestamp |
| `UART_SENSOR_PKG`  | 2 | Temperature, humidity, pressure (×100 integer) + timestamp |
| `UART_RESTART_PKG` | 3 | Restart acknowledgement — ACK is sent, FIFO is drained, then `esp_restart()` is called |
| `UART_DIAG_PKG`    | 4 | Task count, stack high-water mark, stack used + timestamp |
| `UART_TEST_PKG`    | 5 | Fixed magic bytes `0xDEADBEEF` + CRC (used for wire testing) |

All fixed-layout packages are declared as packed structs:

```c
typedef struct __attribute__ ((packed))
{
    uint8_t  tag_bit;
    uint16_t data_len;
    uint8_t  status;
    uint32_t uptime_s;
    uint32_t timestamp_s;
    uint16_t crc16;
} uart_status_pkg_t;
```

`__attribute__ ((packed))` tells the compiler to place fields end-to-end with no padding bytes. Without it, the compiler might insert padding before `data_len` to keep it aligned on a 2-byte boundary, corrupting the wire format.

### The `pkg_buf` union

All package types share a single memory region via a union:

```c
union {
    uart_status_pkg_t  status;
    uart_sensor_pkg_t  sensor;
    uart_restart_pkg_t restart;
    uart_diag_pkg_t    diag;
    uart_server_pkg_t  server;
} pkg_buf;
```

A union allocates space equal to its largest member. At any moment only one package is being prepared, so sharing the memory is safe. This avoids allocating six separate structs that are never in use simultaneously.

### The SERVER package exception

The server package is different from the others because its JSON payload is variable-length and comes from the HTTP client's heap buffer. The struct stores a **pointer** to the JSON data rather than embedding it inline:

```c
typedef struct __attribute__ ((packed))
{
    uint8_t  tag_bit;
    uint16_t data_len;
    char     *json_data;      // pointer, not inline bytes
    uint32_t timestamp_s;
    uint16_t crc16;
} uart_server_pkg_t;
```

This means `sizeof(uart_server_pkg_t)` counts the pointer width (4 or 8 bytes), not the actual JSON length. The `data_len` field for the server package must therefore be computed manually:

```c
pkg->data_len = (uint16_t)(json_len + sizeof(pkg->timestamp_s) + sizeof(pkg->crc16));
```

And the transmit path in `uart_mole_work` must handle it specially — writing the JSON from the pointer, then writing `timestamp_s` and `crc16` as a second write, rather than writing a contiguous struct.

---

## 5. The Wire Format

Every response follows the same framing:

```
┌──────────┬───────────────────┬──────────────────────────┐
│  tag     │  data_len         │  payload (data_len bytes) │
│  1 byte  │  2 bytes (LE)     │                           │
└──────────┴───────────────────┴──────────────────────────┘
```

- **tag** — the same value as the command byte that triggered this response
- **data_len** — number of bytes that follow, including the CRC at the end (little-endian uint16)
- **payload** — the package-specific fields followed by the 2-byte CRC

For a STATUS package the payload is `status(1) + uptime_s(4) + timestamp_s(4) + crc16(2)` = 11 bytes, so `data_len = 11`.

The receiver reads the 3-byte header first to learn how many payload bytes to expect, then reads exactly that many bytes.

### Computing `data_len` for fixed-layout packages

```c
pkg->data_len = sizeof(*pkg) - sizeof(pkg->tag_bit) - sizeof(pkg->data_len);
```

This subtracts the 3 header bytes from the total struct size, giving the number of bytes the receiver must read after the header.

---

## 6. CRC-16/X25

Every package ends with a 16-bit checksum so the receiver can detect transmission errors or misalignment.

### What function is used

```c
pkg->crc16 = esp_crc16_le(0, &pkg->status, payload_byte_count);
```

`esp_crc16_le` is an ESP-IDF wrapper around a ROM function. According to the ESP-IDF source (`esp_rom_crc.h`), the ROM CRC functions apply a bitwise NOT (`~`) to both the initial value and the final result internally. This means:

```
esp_crc16_le(init, buf, len)  ≡  ~crc_raw(~init, buf, len)
```

Calling it with `init = 0` therefore runs the raw CRC with a starting value of `~0 = 0xFFFF`, then complements the result. This is exactly the **CRC-16/X25** standard:

| Parameter | Value |
|-----------|-------|
| Polynomial | 0x1021 |
| Reflected polynomial (for right-shifting) | 0x8408 |
| Initial value | 0xFFFF |
| Output XOR | 0xFFFF |

### What the CRC covers

The CRC covers all payload bytes **except the CRC field itself**. For STATUS:

```c
pkg->crc16 = esp_crc16_le(0, &pkg->status,
                           pkg->data_len - sizeof(pkg->crc16));
```

This starts at the first data field (`status`) and covers everything up to but not including the 2-byte CRC at the end.

### Why CRC matters

Without a checksum, a receiver that reads 3 header bytes out of sync (for example because it read a stale byte from a previous session) would interpret a garbage `data_len`, try to read thousands of bytes, and time out. The CRC provides a second layer of defence: even if framing is correct, a bit error in transit will be caught.

---

## 7. The Task Scheduler

The ESP-IDF UART driver handles the TX path asynchronously through an internal DMA buffer, but the module still needs a place to call `uart_write_bytes` from. Rather than transmitting directly from the listener task (which would block it from receiving new commands), the listener hands the work to a **task scheduler** — a lightweight cooperative scheduler that runs callbacks from the main task loop.

The pattern:

```c
// In listener task:
s_uart_mole.pkg_tag = UART_STATUS_PKG;
task_scheduler_add(&s_uart_mole.task_node, 0);   // schedule immediately

// In main loop:
task_scheduler_work();   // fires uart_mole_work when it is due
```

`uart_mole_work` is the callback registered on `s_uart_mole.task_node`. When the scheduler calls it, it writes the bytes for whichever package type is currently staged in `pkg_buf`, then zeroes the buffer.

The `task_node.active` flag is set by the scheduler while the node is queued or running, which is how the listener knows to drop new commands.

---

## 8. The RESTART Package

`UART_RESTART_PKG` (tag 3) causes the ESP32 to reset itself cleanly after acknowledging the command. The sequencing is critical: the chip must not reset before the client has received the response, otherwise the client gets a timeout instead of a confirmation.

The listener builds the ACK normally and hands it to the scheduler:

```c
pkg->tag_bit     = UART_RESTART_PKG;
pkg->data_len    = sizeof(*pkg) - sizeof(pkg->tag_bit) - sizeof(pkg->data_len);
pkg->rslt_bit    = SET_RESTART_OK;
pkg->timestamp_s = (uint32_t)time(NULL);
pkg->crc16       = esp_crc16_le(0, &pkg->rslt_bit, pkg->data_len - sizeof(pkg->crc16));
task_scheduler_add(&s_uart_mole.task_node, 0);
```

`uart_mole_work` sends the bytes and then drains the hardware TX FIFO before resetting:

```c
uart_mole_uart_write_wrapper(port, pkg, hdr_len);
uart_mole_uart_write_wrapper(port, payload, pkg->data_len);
uart_wait_tx_done(port, pdMS_TO_TICKS(200));   // block until FIFO is empty
ESP_LOGI(TAG, "restarting now");
esp_restart();
```

`uart_wait_tx_done` is essential. `uart_write_bytes` copies bytes into the driver's DMA buffer and returns immediately — the bytes have not been clocked out yet. Calling `esp_restart()` at that point would cut the packet short. With `uart_wait_tx_done`, the function blocks until the UART peripheral confirms the TX FIFO is empty and the last bit has left the GPIO pin. At 115200 baud the 9-byte ACK takes under 1 ms; the 200 ms timeout is purely defensive.

---

## 9. The TEST Package

`UART_TEST_PKG` (tag 5) is a hardcoded diagnostic packet that does not use the scheduler or `pkg_buf`. It is built inline and sent directly from the listener task:

```c
static const uint8_t magic[4] = {0xDE, 0xAD, 0xBE, 0xEF};
uint16_t crc = esp_crc16_le(0, magic, sizeof(magic));
uint8_t pkt[9] = { UART_TEST_PKG, 6, 0, 0xDE, 0xAD, 0xBE, 0xEF,
                   crc & 0xFF, crc >> 8 };
uart_mole_uart_write_wrapper(UART_MOLE_PORT, pkt, sizeof(pkt));
```

Its purpose is to verify the complete wire path — baud rate, framing, CRC algorithm — without depending on any live application data. If the TEST packet decodes correctly on the client, every other packet will too.

---

## 10. Logging

The module logs key events at `ESP_LOGI` level:

| Log | Meaning |
|-----|---------|
| `rx cmd 0x%02x` | A command byte was successfully read from the ring buffer |
| `tx pkg tag=0x%02x` | `uart_mole_work` is about to write bytes to UART |
| `tx test pkg crc=0x%04x` | TEST packet sent, CRC value shown for verification |

If you see `rx cmd` but never `tx pkg`, the task scheduler is not calling the work callback — check that `task_scheduler_work()` is being called in the main loop. If you see neither, the command never reached UART0 — check baud rate and physical wiring.
