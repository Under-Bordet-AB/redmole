# uart_mole — UART Protocol Layer (ESP32)

Firmware-side UART driver and protocol layer. Listens for command bytes from a host, assembles typed response packets, and transmits them over UART. Each packet carries a tag byte, a 16-bit payload length, a payload, and a CRC-16/X25 checksum. Payload bytes that collide with framing values are escaped before transmission.

## Architecture

```
UART driver (ISR / event queue)
        │
        ▼
  Listener task          ← spawned by uart_mole_init()
        │  receives command byte
        ▼
  Packet assembly        ← fills pkg_buf union in uart_ctx_t
        │
        ▼
  Task scheduler         ← work node posts packet over UART
```

The listener task blocks on the UART driver event queue. When a command arrives it assembles the appropriate response packet in the `pkg_buf` staging union and posts the scheduler node. The scheduler then sends the packet, keeping UART I/O off the listener task.

## Packet types

| Tag | Enum | Payload contents |
|---|---|---|
| 0 | `UART_STATUS_PKG` | Online-state bitmask, uptime (s), Unix timestamp. |
| 1 | `UART_SERVER_PKG` | JSON server data (PSRAM buffer), Unix timestamp. |
| 2 | `UART_SENSOR_PKG` | Temperature, humidity, pressure (all ×100), Unix timestamp. |
| 3 | `UART_RESTART_PKG` | Restart result bitmask, Unix timestamp. |
| 4 | `UART_DIAG_PKG` | FreeRTOS task count, listener stack HWM and current usage, Unix timestamp. |
| 5 | `UART_TEST_PKG` | Test/loopback packet. |

Sensor values are fixed-point integers scaled by 100 (e.g. 2150 = 21.50 °C).

## Status flags

The STATUS packet's `status` byte is a bitmask:

| Macro | Bit | Meaning |
|---|---|---|
| `SET_WIFI_ONLINE` | 0 | WiFi is connected. |
| `SET_LEOP_SERVER_ONLINE` | 1 | LEOP server is reachable. |
| `SET_BME280_SENSOR_ONLINE` | 2 | BME280 sensor is present. |
| `SET_GUI_ONLINE` | 3 | GUI is running. |
| `SET_SDCARD_ONLINE` | 4 | SD card is mounted. |

These flags mirror the event group bits (`UART_MOLE_*_BIT`) used for inter-module signalling.

## CRC

CRC-16/X25: polynomial `0x8408` (reflected `0x1021`), init `0xFFFF`, xorout `0xFFFF`. The final checksum is `~protocol_crc16_update(0xFFFF, buf, len)`. **The host client must use the identical algorithm.** Several incompatible CRC-16 variants exist; verify before changing either side.

## API

```c
esp_err_t uart_mole_init(EventGroupHandle_t *event_group);
esp_err_t uart_mole_deinit(void);
```

`uart_mole_init()` installs the UART driver and ISR via `uart_driver_install()`, allocates a persistent JSON buffer on PSRAM, and spawns the listener FreeRTOS task. The `event_group` pointer is injected from `main` and used to read subsystem state when assembling STATUS packets.

`uart_mole_deinit()` deletes the listener task, uninstalls the driver and ISR, and frees the JSON buffer.

## Configuration (in `uart_ctx_t`)

| Field | Description |
|---|---|
| `uart_mole_port` | UART peripheral number (e.g. `UART_NUM_1`). |
| `uart_mole_rx_pin` / `uart_mole_tx_pin` | GPIO pin numbers. |
| `uart_mole_baud_rate` | Baud rate in bps. |
| `uart_mole_rx_buf_size` / `uart_mole_tx_buf_size` | Driver ring-buffer sizes in bytes. |

## Dependencies

- `task_scheduler` — packet transmission is queued through the scheduler.
- ESP-IDF: `driver/uart`, `driver/gpio`, FreeRTOS event groups and tasks, `esp_crc`.
