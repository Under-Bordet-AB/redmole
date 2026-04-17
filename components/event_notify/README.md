# Event Notify Module

`rm_event_notify` is a single-instance wrapper around a FreeRTOS event group.

## Current Role

This module is currently used as a simple startup gate for bring-up.

Current flow:

1. `main.c` initializes `rm_event_notify`
2. runtime tasks signal their ready bits
3. `main.c` waits until all expected ready bits are set
4. `main.c` signals `RM_EVENT_NOTIFY_BIT_INIT_DONE`
5. waiting tasks leave the startup gate and enter their main loops

## Public API

The module currently exposes:

- `rm_event_notify_init()`
- `rm_event_notify_deinit()`
- `rm_event_notify_signal(...)`
- `rm_event_notify_wait_all(...)`
- `rm_event_notify_wait(...)`

## Current Bits

Current named bits are:

- `RM_EVENT_NOTIFY_BIT_INIT_DONE`
- `RM_EVENT_NOTIFY_BIT_SENSOR_READY`
- `RM_EVENT_NOTIFY_BIT_GUI_READY`

These are only the currently used startup bits.
The module itself is still generic enough to carry more event bits later.

## Ownership Model

`rm_event_notify` is a singleton module.

- it owns one internal FreeRTOS event group
- callers do not receive a public context pointer
- callers interact with it through plain `rm_event_notify_*` functions

## Scope

Right now this is mainly a startup synchronization helper.

It is not:

- a replacement for `esp_event`
- a queue
- a message bus
- a data store

If the project later needs broader global signaling, this module can either grow carefully or stay limited to startup gating and let another module handle higher-level app events.
