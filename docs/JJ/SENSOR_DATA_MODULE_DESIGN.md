# SensorData Module Design

This document focuses only on memory, data ownership, and access patterns for a `sensorData` module.

The `sensorData` module owns all data produced by the sensor system.
It does **not** own the physical sensor driver or sampling hardware.

That separation makes sense.

A good mental split is:

- sensor driver: talks to hardware and produces raw measurements
- `sensorData` module: stores, organizes, exposes, and persists measurement data

That is a clean boundary.

--------------------------
q: Why is that actually good? Why not just let the sensor module also own the data?
--------------------------
a: Because "produce measurements" and "own the application's measurement data" are different responsibilities.

The sensor side is about hardware concerns:
- bus communication
- configuration
- retries
- calibration
- timing
- driver errors

The `sensorData` side is about data concerns:
- latest snapshot
- recent history
- archive
- reader APIs
- aggregation
- persistence/export access

If you fuse them, hardware logic and memory/history logic get tangled together. That makes both harder to change.

Keeping them separate means:
- sensor hardware can change without redesigning the data model
- storage/history policy can change without touching sensor driver code
- fake samples can be injected for testing without real hardware
- UI and persistence depend on the stable data owner, not the hardware producer

So the short answer is:
the sensor knows how to get data, `sensorData` knows how to own data.
--------------------------

## 1. Purpose

The purpose of `sensorData` is to be the single owner of:

- current live sensor snapshot
- short-term high-rate recent history
- longer-term historical archive
- APIs for readers to obtain safe views or copies

It should not:

- configure the physical sensor
- own sampling timing policy unless explicitly asked
- directly render UI
- directly own storage drivers like SD card

It may coordinate with those systems, but it should remain the owner of the data itself.

## 2. Requirements We Know So Far

The system has:

- temperature
- humidity
- pressure

Sampling/update behavior:

- live snapshot may update as fast as 60 times per second
- minimum update rate is 1 time per second
- longer-term storage should usually keep 1-minute interval history

Readers may want data for:

- UI drawing
- dumping to SD card
- persisting to flash
- inspection/debugging

Important constraint:

history may grow large and its final size is unknown

That means this module must be designed around bounded RAM and controlled access.

## 3. First Big Design Decision

Do **not** model all sensor data as one giant shared structure.

Instead split it into distinct classes of data:

1. live snapshot
2. recent high-rate window
3. long-term archive

These three have different memory and access needs.

That is the single most important design decision in this document.

## 4. Ownership Model

The `sensorData` module is the only owner of internal sensor data buffers.

Other modules do not:

- write directly into its structs
- keep long-lived raw pointers into internal buffers
- decide internal memory layout

Other modules interact through APIs only.

This gives:

- one place to reason about correctness
- freedom to change storage layout later
- better protection from race conditions

In short:

the module owns the bytes, other code owns requests.

--------------------------
q: But if readers only want data, why not let them just listen to the sensor directly for update events?
--------------------------
a: Because the sensor is the wrong public dependency for most of the system.

If the UI or persistence layer listens directly to the sensor, then those modules become tied to:
- sensor timing
- sensor availability
- sensor-specific payload shapes
- sensor driver behavior

That makes the producer into the public authority for application data, which is usually the wrong ownership boundary.

If readers instead depend on `sensorData`, then:
- the sensor implementation can change
- multiple sensors can feed the same data owner
- aggregation/downsampling can change
- caching/history policy can change

and the rest of the system still talks to one stable data module.

So the stronger architecture is:
- sensor produces
- `sensorData` owns
- readers consume from `sensorData`
--------------------------

## 5. Data Model

At the lowest level, a sample can be represented as one timestamp plus three values.

Example:

```c
typedef struct {
    int64_t timestamp_ms;
    float temperature_c;
    float humidity_pct;
    float pressure_hpa;
} sensor_sample_t;
```

This is the natural unit of data for:

- recent history
- archive
- copy-out snapshots

For the live snapshot, you may still use the same struct, which is nice because it keeps the data model consistent.

## 6. Proposed Internal Architecture

Use three storage layers inside `sensorData`.

### A. Live Snapshot

One small struct representing the most recent reading.

Example:

```c
static sensor_sample_t s_latest;
```

Purpose:

- fast UI reads
- “what is the current temperature right now?”
- safe cheap copy-out

This data is small, so copying it is cheap.

That means the safest API style here is usually copy-out:

```c
bool sensor_data_get_latest(sensor_sample_t *out);
```

This avoids pointer lifetime problems entirely for the live snapshot.

--------------------------
q: Why not publish the latest snapshot in an event payload instead of making readers call an API?
--------------------------
a: You can do that for a small struct, but it is usually not the best default.

The latest snapshot is small enough that event payload transport is technically fine. But architecturally, it is often cleaner to use:
- event for notification
- API for actual data access

Why?

Because then:
- `sensorData` stays the owner of the latest snapshot
- listeners can ignore updates they do not care about
- GUI can redraw on its own schedule
- events stay lightweight
- you avoid turning the event system into your main data transport layer

So the preferred pattern is:
- `sensorData` updates latest sample
- posts "latest updated" event
- reader decides whether to fetch latest via API

That keeps event traffic small and keeps ownership centralized.
--------------------------

### B. Recent High-Rate Window

Keep a bounded RAM ring buffer for recent samples at the high update rate.

Example idea:

- last 10 seconds at 60 Hz
- or last 60 seconds at 1 Hz minimum

This buffer supports:

- UI graphs
- recent trends
- quick short-range inspection

Because this is bounded, RAM use stays predictable.

Example:

```c
static sensor_sample_t s_recent_ring[RECENT_CAPACITY];
static size_t s_recent_head;
static size_t s_recent_count;
```

This is usually a better fit than keeping an ever-growing RAM history.

--------------------------
q: Why not just keep one giant history buffer and let everyone borrow a pointer into it?
--------------------------
a: Because that turns memory ownership, lifetime, and synchronization into a mess.

With one giant shared mutable history buffer, you immediately get hard questions:
- who can write?
- who can read while writes happen?
- how long may readers keep pointers?
- how do you avoid half-written reads?
- how do you avoid unbounded RAM growth?

For small systems, a giant shared buffer feels tempting because it looks simple. In practice it usually becomes the opposite.

The bounded recent ring solves several problems at once:
- RAM cost is known in advance
- UI gets the short window it actually needs
- the public API can expose a stable abstraction instead of raw internals
- archive strategy can be different from recent-view strategy
--------------------------

### C. Long-Term Archive

Do **not** keep unlimited historical data in RAM.

Instead:

- aggregate/downsample to 1-minute samples
- store those in a second structure
- optionally flush/archive them to flash or SD

This can be:

- another ring buffer if you only need bounded history in RAM
- an append-only persistence log if you need long retention

This is the layer readers should use for:

- full export
- persistence
- coarse long-term graphing

--------------------------
q: If archive readers want a lot of data, doesn't copy-out waste RAM?
--------------------------
a: It would if you tried to copy the entire archive at once. That is exactly why the design recommends chunked or iterator-style reads.

The point is not:
"always duplicate big data"

The point is:
"do not expose raw long-lived internal pointers by default"

For large history, the right compromise is:
- bounded chunks
- iteration
- streaming to the consumer

That gives safety without needing one giant duplicate copy in RAM.
--------------------------

## 7. Why This Split Is Better

Without the split, you run into conflicting requirements:

- UI wants frequent fresh updates
- archive wants compact long retention
- RAM is limited
- large shared mutable buffers are hard to keep safe

By splitting, you get:

- live data is cheap and simple
- recent history is bounded and fast
- archive is compact and scalable

This reduces both RAM pressure and concurrency complexity.

## 8. Writer Model

There should be exactly one write path into `sensorData`.

That does **not** mean there must be one FreeRTOS task.
It means there must be one clear API boundary for writes.

Something like:

```c
bool sensor_data_push_sample(const sensor_sample_t *sample);
```

or:

```c
bool sensor_data_push_raw_measurement(...);
```

Then inside that function, the module:

- updates the live snapshot
- appends to the recent ring
- decides whether a 1-minute aggregate/archive sample should be updated

This is how the module keeps ownership of data evolution.

--------------------------
q: We might have many sensors later. Does this design still make sense?
--------------------------
a: Yes, and that is one of its strengths.

Because `sensorData` is not fused to one specific sensor task, many producers can still feed it through controlled write APIs.

Examples:
- one environmental sensor task pushes all three values
- separate tasks push different sensor families
- a future virtual/computed sensor pushes derived values

The important point is that many producers may exist, but the write path into the data owner remains controlled and explicit.

So the architecture scales better if sensors multiply.
--------------------------

## 9. Reader Model

Readers fall into different categories, and each category should get a different API.

That is critical.

Do not expose one giant generic “borrow any internal pointer” API unless you absolutely must.

### Reader Type 1: Current Value Readers

Example:

- status UI
- telemetry screen

Best API:

```c
bool sensor_data_get_latest(sensor_sample_t *out);
```

This is safe and cheap because the struct is small.

### Reader Type 2: Recent Window Readers

Example:

- graph widgets
- sparkline drawing

Best API style:

- copy out a bounded range into caller buffer
- or iterate through a callback/chunk API

Example:

```c
size_t sensor_data_copy_recent(sensor_sample_t *out, size_t max_count);
```

This avoids exposing internal ring-buffer layout.

--------------------------
q: For the GUI, what is actually best: event payload or API call?
--------------------------
a: Best default: event for notification, API for data.

For a GUI, the most robust pattern is:
- `sensorData` posts a small "latest updated" event
- GUI marks itself dirty
- GUI render/update code calls `sensor_data_get_latest(...)`
- GUI renders from a local copied snapshot

That is better than rendering directly from a borrowed internal pointer because:
- no lock must be held while rendering
- no temporary pointer lifetime issues
- the GUI gets one stable snapshot for the frame

So for the GUI specifically:
- event says "something changed"
- API gives the data snapshot
--------------------------

### Reader Type 3: Long-Term Export Readers

Example:

- dump to SD card
- flush to persistence

Best API style:

- chunked iteration
- range-based copy
- streaming callback

Example:

```c
typedef bool (*sensor_data_iter_cb_t)(const sensor_sample_t *sample, void *ctx);

bool sensor_data_iter_archive(sensor_data_iter_cb_t cb, void *ctx);
```

This prevents huge RAM copies and keeps archive ownership inside the module.

--------------------------
q: Why not let long-term readers just hold a pointer and version number to internal archive storage?
--------------------------
a: Because versioning helps detect staleness, but it does not solve ownership and lifetime by itself.

A `const` pointer plus version can tell a reader:
- what version it saw
- whether newer data exists

But it does not guarantee:
- the underlying buffer will remain valid forever
- the owner will not recycle that storage
- the reader can safely keep that pointer indefinitely

So versioning is useful, but it is not a full replacement for copy-out, chunking, or explicit borrowing rules.

That is why this design keeps the archive APIs ownership-safe by default.
--------------------------

## 10. Memory Strategy

The module should optimize memory by following these rules:

Keep the live snapshot tiny.

Bound the recent window.

Do not keep unbounded history in RAM.

Prefer streaming/chunking for large historical reads.

Downsample aggressively for long-term retention.

This is the memory story in one sentence:

small hot data stays in RAM, large cold data is bounded or streamed.

--------------------------
q: Isn't this paying for safety with extra copies?
--------------------------
a: Yes, sometimes, but the design tries to pay that cost only where it is cheap.

That is why the design splits the problem:
- latest snapshot is tiny, so copy-out is cheap
- recent window is bounded, so copy-out is controlled
- large archive reads use chunks/iteration, not full duplication

So the real memory strategy is:
copy at safe boundaries, not everywhere by default.
--------------------------

## 11. Recommended Synchronization Strategy

There are several choices, but for this module the best default is probably:

- single writer through module API
- short critical sections for internal state mutation
- copy-out for small reads
- chunk/callback APIs for large reads

That is simpler than handing out borrowed pointers to large internal buffers.

### Option A: Mutex Around Internal State

Simple and likely good enough at first.

Example model:

- `sensor_data_push_sample()` takes lock, updates internal state, releases
- `sensor_data_get_latest()` takes lock, copies one struct, releases
- `sensor_data_copy_recent()` takes lock, copies requested range, releases

Pros:

- simple
- easy to reason about
- low implementation complexity

Cons:

- long reads can block writers if copy sizes get large

This means:

use mutex + copy-out for small or moderate reads

but do not hold a mutex while doing huge export work.

### Option B: Owner Task

A stronger architecture is:

- one task owns all `sensorData` mutable state
- writer requests go through a queue
- readers request snapshots/ranges through APIs or messages

Pros:

- very clean ownership
- avoids direct concurrent mutation

Cons:

- more complex request/response design
- more moving parts for simple cases

For this module, I would not start with an owner task unless concurrency becomes genuinely hard.

### Option C: Borrowed Pointers to Large Buffers

Not recommended as the default here.

It saves copies but creates lifetime problems and makes APIs harder to use correctly.

Use only when a specific performance bottleneck proves you need it.

## 12. Recommended First Implementation

A very reasonable first version is:

- `sensor_sample_t` as the core data record
- one latest snapshot
- one bounded recent ring buffer
- one bounded 1-minute archive ring buffer
- one mutex protecting module state
- copy-out APIs only

That gives:

- simple ownership
- safe read patterns
- bounded RAM
- good enough performance for this sensor workload

It also leaves room to evolve later.

## 13. Example Public API

This is one good direction.

```c
#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int64_t timestamp_ms;
    float temperature_c;
    float humidity_pct;
    float pressure_hpa;
} sensor_sample_t;

bool sensor_data_init(void);

bool sensor_data_push_sample(const sensor_sample_t *sample);

bool sensor_data_get_latest(sensor_sample_t *out);

size_t sensor_data_copy_recent(sensor_sample_t *out, size_t max_count);

size_t sensor_data_copy_archive(sensor_sample_t *out, size_t max_count);

typedef bool (*sensor_data_iter_cb_t)(const sensor_sample_t *sample, void *ctx);

bool sensor_data_iter_archive(sensor_data_iter_cb_t cb, void *ctx);
```

Notice what this API does **not** do:

- no raw internal buffer pointer returned
- no caller-managed write access
- no exposure of ring-buffer internals

That is deliberate.

## 14. Example Internal Layout

```c
typedef struct {
    sensor_sample_t latest;

    sensor_sample_t recent_ring[RECENT_CAPACITY];
    size_t recent_head;
    size_t recent_count;

    sensor_sample_t archive_ring[ARCHIVE_CAPACITY];
    size_t archive_head;
    size_t archive_count;

    bool latest_valid;
} sensor_data_state_t;
```

And then:

```c
static sensor_data_state_t s_state;
static SemaphoreHandle_t s_lock;
```

That gives a single owned internal state object.

## 15. Why Copy-Out Is Good Here

For this module, copy-out is a very good default because:

- each sample is small
- UI usually needs only recent bounded data
- latest snapshot copy is trivial
- archive readers can work in chunks

The main cost of copy-out is RAM and CPU.

But in this workload:

- copying one sample is cheap
- copying a bounded recent window is acceptable
- archive export should be chunked anyway

So the safety-to-cost ratio is favorable.

## 16. How To Handle 1-Minute History

Do not store every 60 Hz sample forever.

Instead, maintain an aggregator.

For example, over one minute:

- track latest
- track min
- track max
- track average

Then every minute produce one archive sample or one archive record.

This dramatically reduces long-term storage pressure.

You may even decide the archive record should be richer than one raw sample:

```c
typedef struct {
    int64_t minute_timestamp_ms;
    float avg_temperature_c;
    float min_temperature_c;
    float max_temperature_c;
    float avg_humidity_pct;
    float min_humidity_pct;
    float max_humidity_pct;
    float avg_pressure_hpa;
    float min_pressure_hpa;
    float max_pressure_hpa;
} sensor_minute_summary_t;
```

That is often far more useful than one arbitrary sample every minute.

## 17. Persistence Design

The `sensorData` module owns the historical data, but that does not mean it must directly own file systems or flash drivers.

A clean model is:

- `sensorData` owns archive records
- persistence layer asks `sensorData` for chunks or iteration
- persistence layer writes them out

This preserves the ownership split:

- `sensorData` owns data semantics
- persistence module owns storage mechanics

That separation makes your architecture more reusable.

## 18. What This Module Should Not Expose

Avoid APIs like:

```c
const sensor_sample_t *sensor_data_get_internal_buffer(void);
```

or:

```c
sensor_data_state_t *sensor_data_get_state(void);
```

These destroy the ownership boundary and make future changes painful.

The more your public API exposes raw internals, the less the module really owns anything.

## 19. Recommended Direction

If I had to choose one design direction for your current project, it would be:

- `sensorData` owns all sample/history memory
- writer enters only through `sensor_data_push_sample`
- live snapshot is copied out
- recent history is provided via bounded copy-out
- archive is stored as minute summaries
- archive is exposed via chunked copy or iterator APIs
- internal state is protected by one mutex to start with

This is not the fanciest design.
That is exactly why it is a good starting design.

It is:

- memory-conscious
- safe
- easy to reason about
- easy to evolve later

## 20. Final Design Summary

Yes, it makes sense for `sensorData` to own all produced data while not owning the sensor itself.

That is a strong boundary.

The best memory/data design is:

- one owner module
- split live vs recent vs archive
- bounded RAM for recent data
- downsampled archive for long-term retention
- copy-out for small data
- chunk/iterator APIs for large history
- no raw long-lived pointer borrowing by default

That gives you a module that is both safe and realistic on an ESP32-class system.
