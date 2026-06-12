# Snapshot Patterns

This note compares three common ways to let readers safely observe shared data:

- mutex-protected access
- versioned snapshot
- double buffer

The examples use a small sensor snapshot so the core synchronization idea stays easy to see.

## Shared Example Type

```c
#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    float temperature;
    float humidity;
    float pressure;
    uint32_t timestamp_ms;
} sensor_snapshot_t;
```

## 1. Mutex-Protected Snapshot

This is the simplest and safest first pattern.

The idea is:

- one shared struct exists
- writer locks, updates, unlocks
- reader locks, copies out, unlocks

### Code

```c
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static sensor_snapshot_t g_snapshot = {0};
static SemaphoreHandle_t g_snapshot_mutex;

void snapshot_init(void)
{
    g_snapshot_mutex = xSemaphoreCreateMutex();
}

void snapshot_write(float temp, float hum, float press, uint32_t ts)
{
    if (xSemaphoreTake(g_snapshot_mutex, portMAX_DELAY) == pdTRUE) {
        g_snapshot.temperature = temp;
        g_snapshot.humidity = hum;
        g_snapshot.pressure = press;
        g_snapshot.timestamp_ms = ts;
        xSemaphoreGive(g_snapshot_mutex);
    }
}

bool snapshot_read(sensor_snapshot_t *out)
{
    if (xSemaphoreTake(g_snapshot_mutex, portMAX_DELAY) != pdTRUE) {
        return false;
    }

    *out = g_snapshot;

    xSemaphoreGive(g_snapshot_mutex);
    return true;
}
```

### What It Gives You

- easy to understand
- easy to reason about
- one stable copy-out read
- good default for small shared state

### Costs

- readers and writers block each other
- readers cannot run in parallel
- lock misuse can cause deadlocks or long stalls

### Best Fit

- small data
- moderate access rate
- simple systems
- first implementation

## 2. Versioned Snapshot

This pattern is often used when:

- reads are frequent
- the shared data is small enough to copy quickly
- you want readers to avoid taking a mutex

The idea is:

- writer increments a version before writing
- writer updates the data
- writer increments the version again after writing
- readers copy the data and verify the version stayed stable

If the version changed during the read, the reader retries.

This is a seqlock-style read pattern.

### Code

```c
typedef struct
{
    volatile uint32_t version;
    sensor_snapshot_t data;
} shared_sensor_data_t;

static shared_sensor_data_t g_sensor_data = {0};

void sensor_data_write(float temp, float hum, float press, uint32_t ts)
{
    // Odd version means "write in progress"
    g_sensor_data.version++;

    g_sensor_data.data.temperature = temp;
    g_sensor_data.data.humidity = hum;
    g_sensor_data.data.pressure = press;
    g_sensor_data.data.timestamp_ms = ts;

    // Even version means "stable snapshot"
    g_sensor_data.version++;
}

bool sensor_data_read_snapshot(sensor_snapshot_t *out)
{
    uint32_t v1;
    uint32_t v2;

    do {
        v1 = g_sensor_data.version;

        if (v1 & 1) {
            continue;
        }

        *out = g_sensor_data.data;

        v2 = g_sensor_data.version;
    } while (v1 != v2);

    return true;
}
```

### What It Gives You

- readers do not take a mutex
- readers get a stable copy or retry
- good for small snapshots that are read often

### Costs

- more subtle than a mutex
- readers may spin/retry
- not a good fit for large data
- production use may need atomics or memory barriers depending on platform

### Best Fit

- small structs
- many readers
- rare or fast writes
- performance-sensitive snapshot reads

## 3. Double Buffer

This pattern is useful when readers need a stable snapshot and you do not want them to ever see half-written data.

The idea is:

- one buffer is currently published for readers
- writer updates the other buffer
- when done, writer swaps which buffer is active
- readers copy from the currently published buffer

### Code

```c
typedef struct
{
    sensor_snapshot_t buffers[2];
    volatile uint32_t active_index;
} double_buffer_snapshot_t;

static double_buffer_snapshot_t g_db = {0};

void db_snapshot_write(float temp, float hum, float press, uint32_t ts)
{
    uint32_t next = (g_db.active_index == 0) ? 1 : 0;

    g_db.buffers[next].temperature = temp;
    g_db.buffers[next].humidity = hum;
    g_db.buffers[next].pressure = press;
    g_db.buffers[next].timestamp_ms = ts;

    // Publish the fully prepared snapshot
    g_db.active_index = next;
}

bool db_snapshot_read(sensor_snapshot_t *out)
{
    uint32_t index = g_db.active_index;
    *out = g_db.buffers[index];
    return true;
}
```

### What It Gives You

- readers always see a complete published snapshot
- writer never edits the currently published snapshot
- simple mental model

### Costs

- 2x buffer memory
- publication/swap correctness matters
- may still need atomic publication rules depending on platform

### Best Fit

- stable published snapshots
- frequent reads
- moderate data size
- when duplicate storage is acceptable

## Comparison

| Pattern | Reader Cost | Writer Cost | RAM Cost | Complexity | Best For |
|---|---|---|---|---|---|
| Mutex | lock + copy | lock + write | low | low | simplest safe design |
| Versioned snapshot | copy + maybe retry | version bump + write | low | medium | small frequent snapshots |
| Double buffer | copy from published buffer | write other buffer + swap | medium | medium | stable snapshots with frequent reads |

## How To Choose

Start with a mutex when:

- the data is small
- correctness matters more than squeezing performance
- you want the clearest first implementation

Use versioned snapshots when:

- the data is small
- reads are frequent
- you want readers to avoid taking a mutex

Use double buffering when:

- readers must never see half-written data
- 2x storage is acceptable
- the published snapshot idea matches your design

## Important Teaching Note

These examples are meant to teach the synchronization patterns clearly.

For production code, details like:

- atomics
- memory ordering
- ISR vs task context
- multi-core behavior
- cache coherence

can matter a lot.

So the right way to read this file is:

- first learn the pattern
- then tighten the implementation for your platform and requirements
