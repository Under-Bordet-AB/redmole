# uart_client — PC-Side UART Diagnostic Client

This document explains how the `uart_client` program works: how it opens a serial port, sends commands, receives binary packets, and how the C object-oriented design pattern is used to make the code extensible. It is written to teach, not just to describe.

---

## Overview

The `uart_client` is a Linux command-line program that communicates with the `uart_mole` module on the ESP32-S3 over a USB-to-UART bridge. The user selects a serial device, picks a command from a menu, and the program sends a single command byte, waits for a binary response, validates it, and prints the decoded data.

The program is split into four layers:

```
main.c          entry point, creates and runs the app
app.c           lifecycle: init → work loop → deinit
uart_client.c   device discovery, menu, factory, vtable dispatch
protocol.c      framing, CRC, per-type decoders
serial.c        raw POSIX serial port I/O
```

Each layer only knows about the layer below it.

---

## 1. The Serial Port on Linux

### Opening the port

On Linux, serial devices appear as files under `/dev/` — typically `/dev/ttyUSB0` for USB-to-UART chips (CH340, FTDI) or `/dev/ttyACM0` for USB CDC-ACM devices (like the ESP32-S3's built-in USB-JTAG). The client uses `glob()` to find all devices matching `/dev/ttyUSB*` and `/dev/ttyACM*` and lets the user pick.

Opening the file:

```c
int fd = open(path, O_RDWR | O_NOCTTY | O_SYNC);
```

- `O_RDWR` — we need to both write (commands) and read (responses)
- `O_NOCTTY` — prevents the terminal from becoming the controlling terminal for the process, which would let it send signals like SIGINT through the port
- `O_SYNC` — writes block until data is handed to the kernel (not until it reaches the hardware)

### Configuring the port with termios

After opening, we configure the port using the POSIX `termios` API:

```c
struct termios tty;
tcgetattr(fd, &tty);   // read current settings
cfmakeraw(&tty);       // disable all special character processing
cfsetispeed(&tty, B115200);
cfsetospeed(&tty, B115200);
tty.c_cc[VMIN]  = 0;  // do not block waiting for a minimum number of bytes
tty.c_cc[VTIME] = 0;  // do not apply a per-character timeout
tcsetattr(fd, TCSANOW, &tty);
```

`cfmakeraw` is the critical call. Without it, the kernel would interpret incoming bytes as terminal control sequences — newlines would be converted, Ctrl+C would raise a signal, and your binary data would be mangled. Raw mode disables all of that.

`VMIN=0` and `VTIME=0` together mean: return from `read()` immediately with however many bytes are available, even zero. This is non-blocking read mode. We implement our own deadline logic using `timerfd` (see below).

### Flushing the receive buffer

The ESP32-S3 ROM bootloader always outputs its startup messages to UART0 (the hardware pins) regardless of which console is configured in the application. Those messages look like:

```
rst:0x1 (POWERON),boot:0x2b (SPI_FAST_FLASH_BOOT)
SPIWP:0xee mode:DIO, clock div:1
load:0x3fce3818,len:0x16a4
...
```

These bytes accumulate in the Linux kernel's TTY receive buffer for the USB-UART bridge. If the client opens the port minutes after the chip booted, all that ASCII text is still in the buffer waiting to be read. The client would then read those bytes as the start of a binary response packet and get garbage `data_len` values.

The fix is to discard the buffer immediately after configuring the port:

```c
tcflush(fd, TCIFLUSH);   // discard any accumulated RX bytes
```

`TCIFLUSH` discards all bytes in the kernel's receive buffer for this file descriptor. After this call, the next `read()` will only see bytes that arrived after our open.

---

## 2. Deadline Reads with timerfd

The `serial_read_exact` function reads exactly `len` bytes or fails with a timeout error. The challenge is that `VMIN=0` / `VTIME=0` means `read()` returns immediately even with zero bytes. We need a way to say "keep reading until you have all the bytes or 2 seconds pass."

The solution uses two Linux file descriptors together with `poll()`:

1. **The serial fd** — readable when bytes arrive from the UART
2. **A timerfd** — a file descriptor that becomes readable after a configured delay

```c
int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);

struct itimerspec ts = {
    .it_value = { .tv_sec = timeout_ms / 1000,
                  .tv_nsec = (timeout_ms % 1000) * 1000000L }
};
timerfd_settime(tfd, 0, &ts, NULL);

struct pollfd fds[2] = {
    { .fd = fd,  .events = POLLIN },
    { .fd = tfd, .events = POLLIN },
};
```

The loop calls `poll(fds, 2, -1)` — blocking indefinitely — and then checks which fd became readable:

- If the **serial fd** fires: call `read()`, advance the pointer, loop if bytes are still needed.
- If the **timerfd** fires: the deadline expired, return -1.

This approach is more robust than `select()` with a timeout because the deadline is computed once and is unaffected by how many times the loop iterates. It also composes cleanly — two fds, one `poll`, no global timeout state.

---

## 3. The Binary Protocol

### Sending a command

The client sends a single byte — the tag value identifying which package it wants:

```c
write(fd, &tag, 1);
```

That is the entire command. The ESP32 listener reads this byte, builds the response, and transmits it.

### Receiving a response

The response format is:

```
┌──────────┬───────────────────┬──────────────────────────────────────────┐
│  tag     │  data_len         │  payload                                  │
│  1 byte  │  2 bytes (LE)     │  data_len bytes, ends with crc16 (2 B)   │
└──────────┴───────────────────┴──────────────────────────────────────────┘
```

`protocol_recv_packet` implements this in three steps:

**Step 1** — read the 3-byte header:
```c
uint8_t hdr[3];
serial_read_exact(fd, hdr, 3, RECV_TIMEOUT_MS);
uint8_t  tag      = hdr[0];
uint16_t data_len = (uint16_t)(hdr[1] | ((uint16_t)hdr[2] << 8));
```

The `data_len` reconstruction reads the low byte first — this is **little-endian** byte order, which matches how the ESP32 (Xtensa, a little-endian architecture) stores `uint16_t` in memory.

**Step 2** — read the payload:
```c
serial_read_exact(fd, payload_out, data_len, RECV_TIMEOUT_MS);
```

**Step 3** — validate the CRC:
```c
uint16_t computed = (uint16_t)~protocol_crc16_update(0xFFFF, payload_out, data_len - 2);
uint16_t received = (uint16_t)(payload_out[data_len - 2] |
                               ((uint16_t)payload_out[data_len - 1] << 8));
if (computed != received) { /* error */ }
```

The last two bytes of the payload are the CRC. The CRC covers everything before them.

### CRC-16/X25

The ESP32 uses `esp_crc16_le(0, data, len)`. From the ESP-IDF ROM source, this function internally computes `~crc_raw(~init, data, len)`. Passing `init = 0` means it runs with starting value `~0 = 0xFFFF` and complements the result. This is the **CRC-16/X25** algorithm:

| Parameter | Value |
|-----------|-------|
| Polynomial | 0x1021 |
| Right-shifting (reflected) polynomial | 0x8408 |
| Starting value | 0xFFFF |
| Output XOR | 0xFFFF (the final `~`) |

The client builds a lookup table at startup using the reflected polynomial 0x8408:

```c
for (int i = 0; i < 256; i++) {
    uint16_t crc = (uint16_t)i;
    for (int j = 0; j < 8; j++)
        crc = (crc & 1) ? ((crc >> 1) ^ 0x8408u) : (crc >> 1);
    s_crc16_table[i] = crc;
}
```

Then to match `esp_crc16_le(0, data, len)` exactly:

```c
uint16_t result = (uint16_t)~protocol_crc16_update(0xFFFF, data, len);
```

The `(uint16_t)` cast is necessary because `~` on a `uint16_t` promotes to `int` in C (integer promotion rules), and the complement of a 16-bit value in a 32-bit int would have the top 16 bits set.

---

## 4. Object-Oriented Design in C

The client uses a vtable pattern to allow different package types to share a common interface without `switch` statements scattered everywhere. If you have studied C++ or Java, this is essentially a manual version of virtual functions or interfaces.

### The problem it solves

Without polymorphism you would write:

```c
if (tag == TAG_STATUS)      decode_status(payload, data_len);
else if (tag == TAG_SENSOR) decode_sensor(payload, data_len);
// etc.
```

This works for five types, but every time you add a type you must update every `if/else` chain in the program. The vtable approach centralises that mapping in one place — the factory.

### How the vtable works

A **vtable** (virtual function table) is a struct of function pointers. Each concrete package type has its own vtable with its specific implementation of the interface:

```c
struct uart_api {
    int8_t (*read_package)(uart_package_t self);
    int8_t (*write_package)(uart_package_t self, uint8_t *data, size_t len);
};
```

Every concrete package type embeds `uart_base_t` as its first member:

```c
typedef struct uart_base {
    struct uart_api *package_api;  // pointer to this type's vtable
} uart_base_t;

typedef struct { uart_base_t base; } pkg_status_t;
typedef struct { uart_base_t base; } pkg_diag_t;
// etc.
```

`uart_base_t` holds a pointer to the vtable. Because it is always the first member, a pointer to any concrete package type can be safely cast to `uart_base_t *`.

### The handle type

```c
typedef struct uart_api** uart_package_t;
```

`uart_package_t` is a pointer to the `package_api` field inside `uart_base_t`. Why point at the field and not the struct? Because `container_of` (see below) can then recover the full concrete object from the handle.

### container_of

```c
#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))
```

`container_of` is a standard embedded C technique. Given a pointer to a field inside a struct, it computes the address of the enclosing struct by subtracting the field's byte offset.

For example, if `pkg` is a `uart_package_t` (a pointer to `base.package_api`):

```c
uart_base_t *base = CONTAINER_OF(pkg, uart_base_t, package_api);
```

This subtracts `offsetof(uart_base_t, package_api)` from the pointer, yielding the address of the `uart_base_t` itself. For a concrete type you would go one level further.

The pattern makes handles safe and opaque — the caller only holds a `uart_package_t` and has no direct access to the struct internals.

### Calling through the vtable

```c
int8_t uart_factory_read_package(uart_package_t pkg)
{
    return (*pkg)->read_package(pkg);
}
```

Breaking this down:
- `pkg` is `struct uart_api**` — a pointer to the `package_api` field
- `*pkg` dereferences it to get `struct uart_api*` — the vtable pointer
- `(*pkg)->read_package` is the function pointer in the vtable
- The call passes `pkg` (the handle) as `self` so the implementation can use `container_of` if needed

### The factory

`uart_client_create_package(tag)` is the factory function. It allocates memory for the correct concrete type and wires up the vtable pointer:

```c
uart_package_t uart_client_create_package(uart_pkg_tag_t tag)
{
    size_t sz;
    struct uart_api *vtable;

    switch (tag) {
        case TAG_STATUS: sz = sizeof(pkg_status_t); vtable = &s_vtable_status; break;
        case TAG_DIAG:   sz = sizeof(pkg_diag_t);   vtable = &s_vtable_diag;   break;
        // ...
    }

    uart_base_t *base = malloc(sz);
    base->package_api = vtable;
    return &base->package_api;   // return pointer to the field, not the struct
}
```

The `switch` appears only here. The rest of the code calls `uart_factory_read_package(pkg)` without knowing which concrete type it holds.

### Static file descriptor

Because `uart_base_t` does not carry the file descriptor (it only knows about the vtable), the fd is stored as a module-level static:

```c
static int s_fd = -1;
```

This is safe because the work loop is fully sequential — only one command is in flight at a time — so there is no concurrent access risk.

---

## 5. The Work Loop

`uart_client_work` runs until the user types `q`:

```c
for (;;) {
    // print menu, read a character
    int ch = fgetc(stdin);
    // drain the rest of the line (the '\n' fgetc left behind)
    while ((c = fgetc(stdin)) != '\n' && c != EOF);

    // map character to tag
    uart_pkg_tag_t tag = ...;

    // send command
    protocol_send_cmd(s_fd, (uint8_t)tag);

    // create package object (allocates, wires vtable)
    uart_package_t pkg = uart_client_create_package(tag);

    // dispatch through vtable — reads and decodes the response
    uart_factory_read_package(pkg);

    // free the allocation
    uart_client_destroy_package(pkg);
}
```

The stdin drain after `fgetc` is necessary because `fgetc` reads one character but the user pressed Enter, leaving `'\n'` in the input buffer. Without the drain, the next iteration would immediately read `'\n'` and see it as a blank input.

---

## 6. Putting It Together — A Full Transaction

Here is what happens when you press `0` (STATUS) in the menu:

| Step | Where | What happens |
|------|-------|--------------|
| 1 | `uart_client_work` | Reads `'0'`, drains `'\n'`, maps to `TAG_STATUS` |
| 2 | `protocol_send_cmd` | Writes byte `0x00` to the serial fd |
| 3 | Linux kernel | Byte travels through CH34x USB bridge to GPIO44 (RX) on ESP32 |
| 4 | ESP32 hardware FIFO | Byte lands in 128-byte silicon FIFO |
| 5 | ESP-IDF UART ISR | ISR fires, drains FIFO into software ring buffer |
| 6 | `uart_mole_listener_task` | `xQueueReceive` unblocks, reads byte `0x00`, logs `rx cmd 0x00` |
| 7 | Listener | Fills `pkg_buf.status` with WiFi flags, uptime, timestamp, CRC |
| 8 | `task_scheduler_add` | Registers `uart_mole_work` to run immediately |
| 9 | `task_scheduler_work` (main loop) | Calls `uart_mole_work`, logs `tx pkg tag=0x00` |
| 10 | `uart_mole_work` | Writes `[00][0B][00]` (header) then 11 payload bytes to UART TX |
| 11 | ESP32 TX FIFO | Hardware clocks bytes out to GPIO43 (TX) |
| 12 | CH34x bridge | Converts to USB, sends to Linux |
| 13 | `serial_read_exact` | Reads 3-byte header, then 11-byte payload |
| 14 | `protocol_recv_packet` | Validates CRC, returns payload to caller |
| 15 | `protocol_decode_status` | Extracts fields byte-by-byte, prints to stdout |

---

## 7. Building

```bash
cd software/uart_client
make
./rmuc
```

The Makefile compiles with `-std=c99` and `-D_GNU_SOURCE` (needed for `cfmakeraw` and `timerfd_create`, which are Linux/glibc extensions not in the C99 standard). Include paths point to `include/` for all headers.
