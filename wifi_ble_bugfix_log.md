# Wi-Fi and BLE Bugfix Work Log

## Purpose

Track experiments, measurements, decisions, and temporary changes made while
fixing Wi-Fi and BLUFI coexistence under issue #95.

Temporary diagnostic changes on this branch are expected to be replaced or
rolled back during the final implementation.

---

## 2026-06-10 - Initial Coexistence Experiment

### Goal

Determine whether Wi-Fi and BLUFI can run simultaneously on the ESP32-S3, or
whether the final architecture must make them mutually exclusive.

### Original Failure

When BLUFI and the GUI initialized before Wi-Fi, the first `esp_wifi_init()`
failed:

```text
W (...) wifi:esf_buf_setup_static: alloc eb fail(1)
E (...) WIFI: esp_wifi_init failed
E (...) NAC: nac_connect_to_saved_wifi: hw online failed
```

The failure occurred before Wi-Fi started, scanned, or attempted association.

### Temporary Changes

- Initialized the Wi-Fi driver during `nac_init()`, before BLUFI and GUI
  initialization.
- Enabled `CONFIG_BT_ALLOCATION_FROM_SPIRAM_FIRST=y`.
- Kept the initialized Wi-Fi driver online when the saved SSID was not visible.
- Allowed Wi-Fi teardown to tolerate `ESP_ERR_WIFI_NOT_STARTED`.
- Added internal, DMA-capable, and PSRAM heap measurements around:
  - NAC initialization
  - `esp_wifi_init()`
  - Bluetooth controller initialization
  - BLUFI host initialization
  - GUI/runtime initialization
  - Saved-network scan
- Fixed `main.c` to return the actual `nac_init()` error instead of a stale
  return value.

### Hardware Validation

Target:

```text
ESP32-S3 revision v0.2
8 MB PSRAM
ESP-IDF v6.0
Port COM4
```

Validation performed:

- Built with `idf.py build`.
- Flashed to hardware.
- Captured two boot runs.
- Confirmed `esp_wifi_init()` succeeds before BLUFI startup.
- Confirmed Bluetooth controller and BLUFI host initialize successfully.
- Confirmed active Wi-Fi scans complete while BLUFI remains initialized.
- Confirmed the previous `alloc eb fail(1)` does not occur.

### Memory Measurements

| Stage | Internal free | Largest internal block | DMA free | Largest DMA block | PSRAM free |
|---|---:|---:|---:|---:|---:|
| Before early `esp_wifi_init()` | 206,523 | 122,880 | 198,735 | 122,880 | 4,392,192 |
| After `esp_wifi_init()` | 139,215 | 65,536 | 131,427 | 65,536 | 4,328,768 |
| Before BLUFI | 133,775 | 59,392 | 125,987 | 59,392 | 4,304,132 |
| After BLUFI | 81,927 | 31,744 | 74,139 | 31,744 | 4,300,868 |
| After GUI/runtime modules | 4,427 | 3,072 | 3,283 | 3,072 | 562,640 |
| After saved-network scan | 4,131 | 2,816 | 2,987 | 2,816 | 561,024 |

### Findings

- Wi-Fi and BLUFI can run simultaneously.
- Initialization order matters because `esp_wifi_init()` requires a large
  contiguous internal/DMA-capable allocation.
- Initializing Wi-Fi before BLUFI reserves the required driver memory and avoids
  the original failure.
- Bluetooth still consumes approximately 51 KiB of internal memory after Wi-Fi
  initializes, even with PSRAM-first allocation enabled.
- GUI/runtime initialization consumes most remaining internal RAM and PSRAM.
- The current steady-state memory margin is unsafe:
  - Approximately 4 KiB internal RAM remains.
  - The largest internal/DMA block is approximately 3 KiB.
- Wi-Fi should not be repeatedly deinitialized and reinitialized after GUI
  startup. Reinitialization would likely fail because no sufficiently large
  contiguous internal block remains.

### Current Temporary State

The diagnostic firmware currently:

- Initializes and retains the Wi-Fi driver before BLUFI.
- Leaves BLUFI initialized concurrently.
- Keeps Wi-Fi initialized after an unsuccessful saved-network scan.
- Emits `RADIO_HEAP` diagnostic logs during startup.

### Next Investigation Steps

1. Measure task stack high-water marks under worst-case operation.
2. Identify the internal-memory cost of each GUI/runtime module.
3. Identify large PSRAM consumers, especially the GUI and LVGL buffers.
4. Disable unused Bluetooth features and measure the difference.
5. Measure BLUFI negotiation memory usage while a client is connected.
6. Test Wi-Fi association, DHCP, HTTP/TLS, BLUFI, GUI, and SD operations
   concurrently.
7. Define required internal, DMA, and PSRAM safety margins.
8. Replace temporary diagnostics with the final radio lifecycle and memory
   budget implementation.

### Verification Status

- `idf.py build`: passed
- Hardware flash: passed
- Wi-Fi driver initialization: passed
- Bluetooth controller initialization: passed
- BLUFI initialization: passed
- Concurrent BLUFI and Wi-Fi scan: passed
- `git diff --check`: passed

---

## 2026-06-10 - Real Wi-Fi Connection Path Correction

### Correction To Prior Result

The earlier coexistence stress run proved only that Wi-Fi scans and BLE could
run concurrently. It did not prove working Wi-Fi connectivity because the
device never associated, received an IP address, synchronized time, or fetched
a forecast.

### Root Cause

Startup connection was gated on finding the saved SSID in a blocking scan.
Hardware logs showed:

- Saved SSID: `GoGoGo`
- The AP was offline and absent from scan results.
- NAC remained idle instead of attempting association.
- SNTP and forecast correctly remained disabled because no IP address existed.

The scan gate was architecturally incorrect because hidden or temporarily
missed APs are still valid connection targets.

### Implementation

- Removed the scan-before-connect gate for saved credentials.
- Saved credentials now use the normal asynchronous NAC connection state
  machine directly.
- Removed the temporary automated scan/GUI stress runner so it cannot interfere
  with association testing.
- Stopped logging Wi-Fi passwords.
- Changed exhausted association retries from unrecoverable `WIFI_STATE_ERROR`
  to disconnected `WIFI_STATE_IDLE`.

The last change keeps BLUFI provisioning available when a saved AP is offline.
`WIFI_STATE_ERROR` remains reserved for actual driver initialization failure.

### Hardware Result With AP Offline

With Wi-Fi and BLUFI initialized in the same flash:

- NAC attempted association with `GoGoGo` five times.
- ESP-IDF reported disconnect reason `201` (`WIFI_REASON_NO_AP_FOUND`), expected
  because the AP was confirmed offline.
- Wi-Fi retries used stop/start without deinitializing either radio.
- After retry exhaustion, NAC returned to disconnected/provisionable state.
- No OOM, reset, or scheduler task error occurred.

### Remaining Acceptance Test

Turn on `GoGoGo` or provision another reachable AP through BLUFI, then verify
the complete chain in one flash:

1. BLE/BLUFI initialized.
2. Wi-Fi association and DHCP complete.
3. `IP_EVENT_STA_GOT_IP` received.
4. SNTP synchronization callback received.
5. Forecast HTTPS request and GUI update succeed.

---

## 2026-06-10 - Centralized Task Stack High-Water Reporting

### Goal

Measure minimum-ever free stack space for every FreeRTOS task without modifying
each component or wrapping every task-creation call.

### Implementation

- Enabled `CONFIG_FREERTOS_USE_TRACE_FACILITY=y`.
- Added one centralized task reporter in `main.c`.
- Used `uxTaskGetSystemState()` to enumerate application and ESP-IDF tasks.
- Allocated the temporary `TaskStatus_t` snapshot array from PSRAM.
- Logged task stack high-water marks after startup and every 60 seconds.

The reported high-water value is the minimum amount of free stack, in bytes,
observed since each task was created. Lower values indicate less remaining
margin.

### Result

The centralized reporter captured all 17 active tasks, including Wi-Fi,
Bluetooth, ESP-IDF system tasks, and application tasks. Individual component
changes are not required.

Measurements after approximately one minute:

| Task | Minimum free stack bytes |
|---|---:|
| `main` | 4,584 |
| `IDLE0` | 756 |
| `IDLE1` | 864 |
| `environment` | 2,540 |
| `tcpip` | 2,444 |
| `lvgl` | 5,092 |
| `uart_mole_liste` | 3,340 |
| `ipc0` | 464 |
| `ipc1` | 604 |
| `hciT` | 1,384 |
| `BTU_TASK` | 3,388 |
| `BTC_TASK` | 1,872 |
| `sys_evt` | 1,368 |
| `wifi` | 5,016 |
| `btController` | 2,560 |
| `esp_timer` | 2,896 |
| `Tmr Svc` | 1,388 |

### Findings

- Centralized FreeRTOS task enumeration is preferable to component wrappers for
  diagnostics because it also covers tasks created inside ESP-IDF.
- Several tasks retain substantial unused stack and are candidates for later
  right-sizing:
  - `main`
  - `lvgl`
  - `uart_mole_liste`
  - `environment`
- ESP-IDF-owned stacks should not be changed until their worst-case behavior and
  relevant Kconfig options are understood.
- Stack sizes must not be reduced from startup measurements alone. Measurements
  must also include:
  - Wi-Fi association and reconnect
  - HTTP/TLS requests
  - BLUFI client connection and security negotiation
  - GUI interaction and theme switching
  - SD-card logging when available
- Enabling the trace facility and reporter did not prevent Wi-Fi/BLUFI
  coexistence. After runtime initialization, internal free memory was 4,263
  bytes with a 3,072-byte largest internal block.

### Next Step

Exercise worst-case concurrent workloads while collecting periodic stack
reports. After minimum values stabilize, define per-task safety margins and
reduce only clearly oversized application-owned stacks.

---

## 2026-06-10 - Bounded Automated Coexistence Stress Run

### Goal

Exercise repeatable Wi-Fi and GUI activity while Wi-Fi and BLUFI remain
initialized concurrently, then capture heap and task stack minimums before
changing any stack sizes.

### Temporary Diagnostic

Added a bounded two-minute stress runner to `main.c`. It:

- Uses the existing main task and public component APIs; it creates no task or
  long-lived allocation.
- Cycles all four GUI panels and opens/closes the Wi-Fi dialogs every 750 ms.
- Requests a Wi-Fi scan every 10 seconds when the NAC is disconnected.
- Reports heap state and all task stack high-water marks every 30 seconds.
- Stops automatically and restores the BME280 panel.
- Does not switch themes because the normal settings binding would persist each
  change to NVS.

### Hardware Result

The ESP32-S3 completed the run with:

- 148 GUI steps
- 12 Wi-Fi scans
- BLUFI initialized throughout
- No reset, OOM, assertion, or scan-request failure

Heap state was stable at every 30-second report:

| Metric | Result |
|---|---:|
| Internal free | 2,971 bytes |
| Internal largest block | 2,816 bytes |
| Internal minimum-ever free | 2,711 bytes |
| DMA free | 2,931 bytes |
| DMA largest block | 2,816 bytes |
| PSRAM free | 557,260 bytes |
| PSRAM largest block | 540,672 bytes |

Final stack high-water measurements:

| Task | Minimum free stack bytes |
|---|---:|
| `main` | 4,616 |
| `IDLE0` | 756 |
| `IDLE1` | 864 |
| `environment` | 2,528 |
| `tcpip` | 2,428 |
| `lvgl` | 4,484 |
| `uart_mole_liste` | 3,340 |
| `ipc0` | 464 |
| `ipc1` | 604 |
| `hciT` | 1,384 |
| `BTU_TASK` | 3,388 |
| `BTC_TASK` | 1,920 |
| `sys_evt` | 1,368 |
| `wifi` | 5,016 |
| `btController` | 2,768 |
| `esp_timer` | 2,896 |
| `Tmr Svc` | 1,388 |

### Findings

- Repeated scans and GUI transitions work while BLUFI remains initialized.
- Heap values remained identical from 30 seconds through completion, so this
  test found no repeated-scan or repeated-GUI-transition leak.
- GUI activity reduced the `lvgl` minimum free stack from 5,092 to 4,484 bytes.
- The test reduced internal free memory from the prior post-runtime value of
  approximately 4,263 bytes to 2,971 bytes. The allocation remained retained
  after dialogs were hidden, consistent with lazy runtime allocation or
  retained GUI state.
- The 2,711-byte internal minimum and 2,816-byte largest block are not an
  acceptable production safety margin. Stack right-sizing may recover useful
  internal memory, but memory-budget work remains necessary.
- Startup-only stack measurements were sufficient to identify oversized
  application tasks, but not sufficient to choose final stack sizes.

### Remaining External Workloads

The automated run cannot establish full worst-case behavior. These still
require external dependencies or working hardware:

- Wi-Fi association, DHCP, reconnect, and authenticated traffic
- HTTP/TLS requests while BLE is active
- BLUFI client connection and security negotiation
- SD-card logging; the current SD mount fails
- Theme switching, tested in a way that avoids repeated NVS persistence

### Verification Status

- `idf.py build`: passed
- Hardware flash on COM4: passed
- Bounded two-minute coexistence run: passed
- `git diff --check`: passed
