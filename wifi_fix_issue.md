# Wi-Fi driver initialization fails after BLUFI startup

## Summary

On the ESP32-S3 target, saved-network autoconnect consistently fails during the
first `esp_wifi_init()` call. The failure occurs before Wi-Fi starts, scans, or
attempts association.

The strongest current hypothesis is insufficient or fragmented internal-capability
RAM when Wi-Fi initializes after the Bluetooth/BLUFI stack and the rest of the
application. This is not yet proven, so the fix branch should first add memory
instrumentation and run the isolation tests below.

## Observed Behavior

Two captured boots, built from commits `6f7fd8f` and `16ad16a`, fail with the
same sequence:

```text
I (...) BLUFI_EXAMPLE: BLUFI init finish
I (...) MAIN: Startup complete
I (...) wifi:Init data frame dynamic rx buffer num: 32
I (...) wifi:Init static rx mgmt buffer num: 5
I (...) wifi:Init management short buffer num: 32
W (...) wifi:esf_buf_setup_static: alloc eb fail(1)
W (...) wifi_init: Failed to unregister Rx callbacks
E (...) wifi_init: Failed to deinit Wi-Fi driver (0x3001)
E (...) wifi_init: Failed to deinit Wi-Fi (0x3001)
E (...) WIFI: esp_wifi_init failed
E (...) NAC: nac_connect_to_saved_wifi: hw online failed
```

Afterward, network-dependent jobs repeatedly report that they are waiting for
Wi-Fi. No scan, association, authentication, DHCP, or IP event occurs.

## Expected Behavior

`esp_wifi_init()` succeeds while BLUFI is active, saved-network autoconnect scans
and connects, and network-dependent jobs can run.

## Reproduction

1. Flash the current `dev` build to the ESP32-S3 board.
2. Ensure valid `wifi_ssid` and `wifi_pass` values exist in NVS.
3. Reboot and monitor serial output.
4. Observe `alloc eb fail(1)` during `esp_wifi_init()` and no connection attempt.

The supplied log reproduced this on application versions `6f7fd8f` and
`16ad16a`.

## Confirmed Evidence

- The failure is inside the first `esp_wifi_init()` call:
  - `main/main.c:164` calls `nac_connect_to_saved_wifi()` after startup.
  - `components/nac/src/nac.c:681` calls `wifi_bring_hw_online()`.
  - `components/nac/src/nac.c:281` calls `esp_wifi_init(&cfg)`.
- The failure happens before `esp_wifi_start()`, scan, credential use, or AP
  association. Router settings and passwords therefore cannot explain this
  specific failure.
- `0x3001` is `ESP_ERR_WIFI_NOT_INIT`, not the original cause. It is emitted by
  ESP-IDF cleanup after the driver only partially initialized.
- ESP-IDF logs the first meaningful failure as
  `wifi:esf_buf_setup_static: alloc eb fail(1)`, which is an allocation failure
  while setting up static Wi-Fi buffers.
- BLUFI is initialized before Wi-Fi:
  - `main/main.c:83` calls `blufi_main()`.
  - `main/main.c:148` reports startup complete.
  - `main/main.c:164` only then attempts saved-network autoconnect.
- Bluetooth uses Bluedroid and is not configured to allocate from PSRAM first:
  - `CONFIG_BT_BLUEDROID_ENABLED=y`
  - `# CONFIG_BT_ALLOCATION_FROM_SPIRAM_FIRST is not set`
- Current Wi-Fi configuration requests:
  - `CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=16`
  - `CONFIG_ESP_WIFI_STATIC_TX_BUFFER_NUM=16`
  - ESP-IDF documents each static RX/TX buffer as approximately 1.6 KiB, so
    these two pools alone request approximately 51.2 KiB during
    `esp_wifi_init()`, excluding driver metadata and other pools.
- PSRAM is enabled and Wi-Fi/LwIP prefer PSRAM, but internal-only allocations
  are still required:
  - `CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP=y`
  - `CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=32768`
  - `CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=16384`
- `idf.py size` reports:
  - DIRAM: 219,060 bytes used, 122,700 bytes remaining before runtime allocation.
  - IRAM: 16,384 bytes used, 0 bytes remaining.
- Boot-time heap reporting shows about 228 KiB total internal heap before
  application modules, BLUFI, GUI, and Wi-Fi allocate runtime memory.
- Commit `16ad16a` is unlikely to be the initiating regression. Its changes only
  affect behavior after Wi-Fi initialization, while both `6f7fd8f` and
  `16ad16a` fail during the first `esp_wifi_init()`.
- BLUFI was added to application startup in commit `83f657b` on May 13, 2026.
  This makes Wi-Fi/Bluetooth coexistence memory pressure a stronger lead than
  the latest NAC state-machine change.

## Leading Hypothesis

Wi-Fi cannot allocate a required internal/DMA-capable static buffer because
BLUFI/Bluetooth and other modules have already consumed or fragmented the
available internal heap.

Confidence: **medium-high**. The allocation-failure log, initialization order,
buffer configuration, and memory configuration all support this. The exact
failed allocation and the responsible consumer are not visible in the current
logs.

## Secondary Issue

When boot autoconnect fails in `nac_connect_to_saved_wifi()`, NAC logs the error
and returns without setting `WIFI_STATE_ERROR`. This leaves callers with a
disconnected state and no actionable failure status. Fix this after the driver
initialization problem is understood so the GUI and diagnostics report future
failures correctly.

## Investigation Plan For Fix Branch

Run these experiments one at a time and record free internal heap, largest
internal block, and minimum-ever internal heap immediately before and after each
major initialization step and immediately around `esp_wifi_init()`:

1. Add temporary `heap_caps_get_free_size(MALLOC_CAP_INTERNAL)`,
   `heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)`, and
   `heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL)` logging.
2. Initialize Wi-Fi before `blufi_main()`. If Wi-Fi succeeds, initialization
   order/internal-memory pressure is confirmed.
3. Temporarily disable BLUFI/Bluetooth. If Wi-Fi succeeds, Bluetooth memory use
   is confirmed as a necessary contributor.
4. Enable `CONFIG_BT_ALLOCATION_FROM_SPIRAM_FIRST`. If coexistence succeeds,
   evaluate this as the preferred configuration fix.
5. If needed, test smaller Wi-Fi static RX/TX buffer counts and measure
   throughput/stability rather than assuming the defaults are required.
6. If total free internal heap appears sufficient but the largest block is too
   small, identify which startup allocations fragment internal RAM and adjust
   allocation order or placement.
7. Test clean boot, saved-network boot, GUI scan/connect, disconnect/reconnect,
   and BLUFI provisioning with Wi-Fi active.

## Suggested Scope

- Diagnose and fix the `esp_wifi_init()` allocation failure while retaining
  required Wi-Fi and BLUFI functionality.
- Add targeted memory diagnostics or a reproducible coexistence test.
- Make NAC expose boot-autoconnect initialization failure as an error state.
- Avoid unrelated Wi-Fi state-machine or GUI refactoring.

## Acceptance Criteria

- Ten consecutive saved-network boots complete without
  `esf_buf_setup_static: alloc eb fail(1)` or `esp_wifi_init failed`.
- Wi-Fi obtains an IP while BLUFI is initialized and advertising.
- GUI scan, connect, disconnect, and reconnect work.
- BLUFI provisioning still works before and while Wi-Fi is active.
- Network-dependent forecast and LEOP jobs run after connection.
- Internal heap and largest-block measurements retain a documented safety
  margin after both Wi-Fi and Bluetooth are initialized.
- A Wi-Fi initialization failure is surfaced as `NAC_WIFI_ERROR`.

## Relevant Files

- `main/main.c`
- `components/nac/src/nac.c`
- `components/nac/include/nac.h`
- `components/blufi/src/blufi_main.c`
- `components/blufi/src/blufi_init.c`
- `sdkconfig`
- `sdkconfig.defaults`

