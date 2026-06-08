# uart_client — Host-Side UART Client

A Linux/POSIX command-line client for communicating with the RedMole ESP32 firmware over a serial port. Sends typed command bytes and receives, validates, and decodes the corresponding response packets.

## Architecture

The client is layered into three modules:

```
app
 └── uart_client        (vtable-based packet factory)
      └── protocol      (framing, CRC-16/X25, encode/decode)
           └── serial   (POSIX serial port I/O)
```

### `serial`

Thin wrapper around a POSIX file descriptor. Configures the port for 8N1, no flow control, at the baud rate expected by the firmware.

```c
int  serial_open(const char *path);         // open and configure
void serial_close(int fd);
int  serial_read_exact(int fd, void *buf, size_t len, int timeout_ms);
int  serial_write_exact(int fd, const void *buf, size_t len);
```

`serial_read_exact()` blocks until exactly `len` bytes arrive or `timeout_ms` elapses (`RECV_TIMEOUT_MS` = 2000 ms default). `serial_write_exact()` retries partial writes.

### `protocol`

Implements the wire format shared with the ESP32 firmware: CRC-16/X25, command sending, packet receiving, and per-tag payload decoders.

```c
void     protocol_crc16_init(void);
uint16_t protocol_crc16_update(uint16_t crc, const uint8_t *data, size_t len);

int protocol_send_cmd(int fd, uint8_t tag);
int protocol_recv_packet(int fd, uint8_t *tag_out, uint8_t *payload_out, uint16_t *data_len_out);

void protocol_decode_status (const uint8_t *payload, uint16_t data_len);
void protocol_decode_server (const uint8_t *payload, uint16_t data_len);
void protocol_decode_sensor (const uint8_t *payload, uint16_t data_len);
void protocol_decode_restart(const uint8_t *payload, uint16_t data_len);
void protocol_decode_diag   (const uint8_t *payload, uint16_t data_len);
void protocol_decode_test   (const uint8_t *payload, uint16_t data_len);
```

Call `protocol_crc16_init()` once before any CRC operations. The CRC algorithm must match the firmware exactly: polynomial `0x8408` (reflected `0x1021`), init `0xFFFF`, xorout `0xFFFF`.

`protocol_recv_packet()` receives a framed packet, validates the CRC, and writes the decoded payload into `payload_out` (must be at least `MAX_PACKET_PAYLOAD` = 12 288 bytes).

### `uart_client`

Vtable-based packet factory. Each packet type is a concrete struct with `uart_base_t` embedded first, giving it a `uart_api` vtable pointer. Callers obtain an opaque `uart_package_t` handle and interact through the factory functions without knowing the concrete type.

```c
int  uart_client_init(uart_client_t *self);
void uart_client_work(uart_client_t *self);
void uart_client_deinit(uart_client_t *self);

uart_package_t uart_client_create_package(uart_pkg_tag_t tag);
void           uart_client_destroy_package(uart_package_t pkg);

int8_t uart_factory_read_package (uart_package_t pkg);
int8_t uart_factory_write_package(uart_package_t pkg, uint8_t *data, size_t len);
```

`uart_client_work()` sends the next queued command, receives the response, and dispatches it to the appropriate `protocol_decode_*` function.

### `app`

Top-level entry point. Holds a single `uart_client_t` and calls `app_run()`, which blocks in the main loop until the application exits.

## Packet types

| Tag | Name | Description |
|---|---|---|
| 0 | `TAG_STATUS` | Subsystem online flags, uptime, timestamp. |
| 1 | `TAG_SERVER` | JSON server data, timestamp. |
| 2 | `TAG_SENSOR` | Temperature, humidity, pressure (×100), timestamp. |
| 3 | `TAG_RESTART` | Restart acknowledgement, timestamp. |
| 4 | `TAG_DIAG` | FreeRTOS diagnostics (task count, stack usage), timestamp. |
| 5 | `TAG_TEST` | Loopback / test packet. |

## Constants

| Macro | Value | Description |
|---|---|---|
| `RECV_TIMEOUT_MS` | 2000 | Receive timeout in milliseconds. |
| `MAX_PACKET_PAYLOAD` | 12288 | Maximum deframed payload buffer size in bytes. |

## Building

```bash
cd software/uart_client
make
./rmuc
```

## Dependencies

- POSIX: `termios`, `unistd`, `fcntl`, `errno`.
- No external libraries required beyond the C standard library.
