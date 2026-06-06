# Environment Measurements Redesign Plan

## Status

This document records the intended direction for `environment_measurements`.
It is an implementation plan, not a description of the current code.

The redesign should remain incremental. The current public C API must continue
to work while the internal representation changes.

## Problem

The current internal data path mirrors the BME280 and the public C API:

```text
Bme280Sensor
    -> TemperatureHumidityPressureReading
    -> EnvironmentMeasurements
    -> environment_measurement_sample_t
```

This works for one BME280, but it makes the complete module assume that every
producer returns temperature, humidity, and pressure together.

Future producers may return different sets of measurements. For example:

- a BME280 returns temperature, relative humidity, and pressure
- a precise temperature sensor returns only temperature
- another combined sensor returns a different set of values

The internal representation should therefore describe logical measurements,
not mirror one physical sensor or one public API response.

## Design Goals

- Keep hardware-specific knowledge inside the component.
- Make adding another local sensor producer straightforward.
- Preserve coherent physical acquisitions: one BME280 read produces one batch.
- Use one polling manager and one FreeRTOS task for all current producers.
- Store measurements independently after acquisition.
- Keep the public C API independent from concrete hardware.
- Allow public APIs to combine or select stored measurements as needed.
- Keep memory bounded and avoid runtime allocation.
- Keep producers injectable so manager and application behavior can be tested
  with fakes.

## Non-Goals

- Runtime producer registration.
- Supporting unknown third-party plugins.
- Designing asynchronous network or wireless producers.
- Persisting history or defining the SD-card format.
- Automatically choosing between multiple producers for the same purpose.
- Exposing hardware identity outside `environment_measurements`.
- Making units or numeric storage dynamically configurable per sample.

These can be addressed later if concrete requirements appear.

## Core Design

The module has four internal responsibilities:

```text
Concrete producers
    -> measurement batches
    -> MeasurementsManager
    -> MeasurementStore
    -> public C API adapters
```

### Concrete Producers

A producer owns the knowledge required to acquire data from one physical
device. It may use I2C, SPI, or another local mechanism.

Examples:

- `Bme280Producer`
- `PreciseTemperatureProducer`
- fake producer used by tests

A producer performs one physical acquisition and returns a non-empty batch of
logical measurements on success. The producer decides how the device must be
read; it does not own the application's latest-value store.

### Measurements Manager

The manager owns one polling task and a fixed list of producers. It:

- initializes every configured producer
- polls producers sequentially
- validates returned batches
- timestamps and atomically publishes successful batches into the store
- tracks and logs producer failure and recovery

The manager knows each producer's allowed logical channels through its
registration, but it contains no producer-specific behavior or BME280-specific
branches.

### Measurement Store

The store owns the latest independently addressable logical measurements. A
batch from a combined sensor is split into separate stored channels after one
coherent acquisition.

For example:

```text
BME280 acquisition
    -> indoor ambient temperature
    -> indoor relative humidity
    -> indoor pressure
```

The store provides synchronized atomic batch publication and multi-channel
copy-out access. It does not acquire hardware data or define public API
response shapes.

Publishing a batch holds the store mutex once while updating every included
channel. Copying a combined response also holds the store mutex once while
copying every requested channel. This prevents consumers from observing a mix
of old and new values from one physical acquisition.

### Public C API Adapters

The public API reads logical channels from the store and presents the
application-facing response.

The existing `environment_measurements_get_latest()` combines the configured
indoor temperature, humidity, and pressure channels into one
`environment_measurement_sample_t`.

A future precise-temperature API could expose another channel without changing
the BME280 producer, manager, or store.

## Internal Data Model

### Logical Channels

A channel identifies the semantic meaning of a stored value. It must not expose
hardware identity.

Initial channels:

```cpp
enum class MeasurementChannel : uint8_t {
    IndoorAmbientTemperature,
    IndoorRelativeHumidity,
    IndoorPressure,
    Count,
};
```

A future precise sensor could add:

```cpp
IndoorPreciseTemperature,
```

Channel names intentionally include semantic purpose and location. A generic
`Temperature` channel would become ambiguous when multiple temperature sources
exist.

### Numeric Representation

Every internal measurement value uses `int64_t`.

```cpp
struct Measurement {
    MeasurementChannel channel;
    int64_t value;
};
```

Each channel has one canonical unit and scale defined as metadata rather than
repeated in every sample.

Example canonical representations:

| Channel | Canonical representation |
|---|---|
| `IndoorAmbientTemperature` | milli-degrees Celsius |
| `IndoorRelativeHumidity` | milli-percent RH |
| `IndoorPressure` | pascals |

These canonical scales are used by the implementation. Conversion to the
existing public deci-unit fields happens at the public API boundary.

Using `int64_t` creates a simple homogeneous internal path and leaves room for
more precise future sensors. The additional cost is negligible for the small
number and low rate of current measurements.

There is no per-sample `size` field because every value is `int64_t`. There is
no per-sample unit field because a channel has exactly one canonical unit.

### Measurement Batch

Producers populate caller-owned fixed-capacity batches:

```cpp
constexpr size_t kMaxMeasurementsPerBatch = 4U;

struct MeasurementBatch {
    std::array<Measurement, kMaxMeasurementsPerBatch> measurements;
    size_t count;
};
```

The capacity should cover known local combined sensors with a small margin. It
can be changed centrally if a concrete producer later needs more channels.

A producer must:

- clear or set `count` on every read
- never exceed batch capacity
- emit at most one value for a channel in one batch
- return only complete, valid values

For the current synchronous local producers, `ESP_OK` with `count == 0` is an
invalid response. A successful read must publish at least one measurement.

The manager assigns one timestamp immediately after a successful producer read
and uses it for every measurement in that batch. This keeps clocks out of
producers and ensures values from one physical acquisition have one timestamp.
A future producer should supply its own acquisition timestamp only if a
concrete hardware requirement makes that timestamp more meaningful.

### Stored Sample

Runtime validity belongs in the store, not in successful producer output:

```cpp
struct MeasurementRecord {
    int64_t timestamp_ms;
    int64_t value;
    uint64_t publication_version;
    bool valid;
};
```

The store can use a fixed array indexed by `MeasurementChannel`:

```cpp
std::array<MeasurementRecord,
           static_cast<size_t>(MeasurementChannel::Count)> latest;
```

The store operations must support:

```cpp
publish_batch(const MeasurementBatch& batch, int64_t timestamp_ms);
invalidate_channels(const MeasurementChannel* channels, size_t count);
copy_channels(const MeasurementChannel* channels, size_t count,
              MeasurementRecord* out);
```

Each operation is atomic across all supplied channels and holds the store mutex
once. `publish_batch()` validates the complete batch before changing the store,
so an invalid item cannot cause a partial update.

The store increments one monotonic `uint64_t` publication version for each
successfully published batch and assigns that same version to every channel in
the batch. Consumers compare versions for change; they do not interpret the
numeric difference as a number of updates.

## Producer Contract

The first producer interface should remain narrow:

```cpp
class MeasurementProducer {
  public:
    virtual ~MeasurementProducer() = default;

    virtual esp_err_t init() = 0;
    virtual esp_err_t read(MeasurementBatch& out) = 0;
};
```

Constraints:

- `read()` is synchronous and bounded.
- Producers do not allocate runtime memory.
- Producers do not publish directly into the store.
- Producers do not expose their hardware identity in returned measurements.
- A failed read returns an error and no publishable measurements.
- `ESP_OK` requires a non-empty valid batch.
- Recovery remains the producer's responsibility when it requires
  hardware-specific behavior.

The interface exists to support heterogeneous producers in one fixed list and
to allow fakes in tests. It should not grow methods for location, simulation,
probing, or concrete sensor type.

## Product Composition

The production composition file is expected to contain hardware knowledge:

```cpp
Bme280Producer s_indoor_bme280(kIndoorAddress, kIndoorSettings);

constexpr MeasurementChannel kIndoorBme280Channels[] = {
    MeasurementChannel::IndoorAmbientTemperature,
    MeasurementChannel::IndoorRelativeHumidity,
    MeasurementChannel::IndoorPressure,
};

std::array<ProducerRegistration, 1> s_producers = {{
    {
        "indoor BME280",
        s_indoor_bme280,
        kIndoorBme280Channels,
        std::size(kIndoorBme280Channels),
    },
}};

MeasurementStore s_store;
MeasurementsManager s_manager(
    s_producers.data(),
    s_producers.size(),
    s_store,
    now_ms);
```

The composition supplies concrete producers, exclusive channel ownership,
diagnostic names, and the manager clock. Registrations and referenced channel
arrays have process lifetime. No runtime registration mechanism is introduced.

BME280 names, settings, I2C addresses, and Kconfig mapping belong in this file
or in the BME280 producer. The generic manager and store must not contain
BME280-specific types.

Adding a producer should normally require:

1. Implementing `MeasurementProducer`.
2. Defining the logical channel or channels it emits.
3. Adding its build-time configuration.
4. Constructing it and adding its registration to the fixed producer list.
5. Adding or adapting a public API only if application consumers need it.

Existing producers and manager logic should not require changes.

## Scheduling And Failure Behavior

The manager uses one task and polls producers sequentially. This is suitable
for the current small number of local, bounded-time sensor reads.

Initial scheduling policy:

- every producer is polled at the configured environment interval
- one slow producer delays later producers in the same cycle
- failed producers are retried on later cycles
- first failure and first recovery are logged per producer

Different producer intervals or asynchronous acquisition are deferred until a
real requirement exists.

On producer failure, the manager atomically invalidates every channel owned by
that producer. This preserves the current fail-fast behavior rather than
serving a previously valid reading until it becomes stale.

The manager therefore needs a way to know which channels belong to a producer.
The preferred initial solution is fixed producer configuration supplied during
composition, not extra introspection methods on the producer interface.

Example:

```cpp
struct ProducerRegistration {
    const char* diagnostic_name;
    MeasurementProducer& producer;
    const MeasurementChannel* channels;
    size_t channel_count;
};
```

The diagnostic name remains internal and is used only for logs.

Registration and batch validation must enforce:

- every channel is owned by exactly one registered producer
- a producer returns only channels listed in its registration
- a producer returns each owned channel at most once per batch
- invalidating a producer affects only its registered channels

The manager validates registrations before polling starts. Duplicate channel
ownership is a manager initialization error.

Module initialization distinguishes manager failures from producer failures:

- failure to initialize manager-owned storage or synchronization fails module
  initialization
- invalid producer registrations fail module initialization
- producer `init()` failures are logged and mark that producer failed, but do
  not prevent module startup
- later polling retries the producer, allowing missing hardware to recover

## Public API Compatibility

The first redesign must preserve:

```c
bool environment_measurements_get_latest(environment_measurement_sample_t* out);
bool environment_measurements_is_fresh(uint32_t max_age_ms);
uint32_t environment_measurements_get_update_count(void);
```

`environment_measurements_get_latest()` succeeds only when all three required
indoor channels are valid and satisfy the module freshness policy.

When combining channels, the API must define the output timestamp. The initial
recommendation is the oldest timestamp among the required channels because it
conservatively represents the age of the complete response.

The API copies all three required channels atomically through one store
operation. It cannot observe a partially published BME280 batch.

The existing public fixed-point units remain unchanged:

- temperature: deci-degrees Celsius
- humidity: deci-percent RH
- pressure: deci-hectopascals

Conversions from canonical internal values must define rounding and overflow
behavior.

Update versions are tracked per channel in the store. An application-facing
combined API derives its change token from the maximum publication version
among the channels it exposes. Therefore, the value changes when any exposed
channel is successfully published but does not change for unrelated channels.

For compatibility, `environment_measurements_get_update_count()` returns this
combined indoor change token truncated to `uint32_t`. Despite the existing
function name, consumers must use it only to detect change. Values can skip
because unrelated batches may consume intermediate store publication versions,
and the public token may eventually wrap. A future precise-temperature update
must not change the returned indoor token.

## Proposed File Responsibilities

The exact filenames may change during implementation, but responsibilities
should remain separated:

| File or area | Responsibility |
|---|---|
| `measurement_types.hpp` | Channels, batches, canonical values, and stored sample types |
| `measurement_producer.hpp` | Narrow producer interface |
| `measurement_store.hpp/.cpp` | Synchronized latest-value storage |
| `measurements_manager.hpp/.cpp` | One task, producer polling, failure handling, publication |
| `bme280/bme280_producer.hpp/.cpp` | BME280 acquisition and conversion into a measurement batch |
| `environment_measurements.cpp` | Product composition, Kconfig mapping, and public C API adapters |
| `include/environment_measurements.h` | Stable application-facing C API |

Do not create a generic top-level `Sensor` object that mixes identity,
location, hardware behavior, scheduling, storage, and public reporting.

## Implementation Plan

### Phase 1: Introduce The Generic Middle

1. Add `MeasurementChannel`, `Measurement`, `MeasurementBatch`, and
   `MeasurementRecord`.
2. Finalize canonical unit and scale for each initial channel.
3. Add the narrow `MeasurementProducer` interface.
4. Add a fixed-size `MeasurementStore` with synchronized publish, invalidate,
   and multi-channel copy-out operations.
5. Make batch publication and multi-channel copy-out atomic.
6. Test channel indexing, atomic publication, invalidation, and freshness
   behavior.

The current BME280 path and public API can remain active during this phase.

### Phase 2: Adapt The BME280

1. Adapt or wrap `Bme280Sensor` as a `MeasurementProducer`.
2. Keep register access, calibration, compensation, settings, and recovery
   inside the BME280 implementation.
3. Convert one successful BME280 acquisition into a three-item batch.
4. Let the manager assign one timestamp to the complete batch.
5. Test successful batches, failed reads, and recovery.

Use a `Bme280Producer` adapter around the current working driver. The driver
retains BME280 protocol and compensation behavior while the adapter maps one
coherent reading onto configured logical channels.

### Phase 3: Introduce The Manager

1. Add `ProducerRegistration`.
2. Validate exclusive channel ownership before polling starts.
3. Preserve recoverable producer initialization behavior.
4. Move the single polling task into `MeasurementsManager`.
5. Poll the fixed producer list sequentially.
6. Validate, timestamp, and atomically publish returned batches.
7. Atomically invalidate registered channels on producer failure.
8. Track and log failure and recovery per producer.
9. Test one producer, multiple fake producers, partial failure, and recovery.

### Phase 4: Adapt The Existing Public API

1. Change the C API forwarding layer to read the three indoor channels from the
   store.
2. Convert canonical internal values to current public units.
3. Preserve current freshness and update-count behavior or explicitly document
   intentional changes.
4. Verify GUI and UART consumers continue to work unchanged.

### Phase 5: Remove Superseded Types

After the new path is verified:

1. Remove `TemperatureHumidityPressureSource`.
2. Remove `TemperatureHumidityPressureReading`.
3. Remove old single-stream polling and storage code.
4. Update the component README to describe the implemented architecture.

## Test Plan

Minimum automated tests:

- a fake producer returns one measurement
- a fake producer returns several measurements in one batch
- manager gives all values in one acquisition the same timestamp
- batch publication is atomic
- multi-channel copy-out is atomic
- manager publishes values to the correct channels
- manager polls multiple producers using one manager
- failure invalidates only the failing producer's registered channels
- recovery republishes and restores validity
- duplicate channel ownership prevents manager initialization
- a producer cannot publish an unregistered channel
- duplicate channels in one batch are rejected
- successful empty batches are rejected
- oversized batches are rejected
- public API combines the three required indoor channels
- public API rejects missing, invalid, or stale required channels
- unrelated channel updates do not change the combined indoor change token
- canonical-to-public conversion handles rounding and limits

The fake producer should implement the same narrow `MeasurementProducer`
interface. It does not need to model a fake BME280 unless a test specifically
targets BME280 behavior.

## Decisions Locked By This Plan

- The generic middle represents logical measurements.
- Physical producers return fixed-capacity batches.
- One physical acquisition may publish several logical channels.
- Internal values use `int64_t`.
- Units and scales are canonical per channel, not repeated per sample.
- The manager owns one task and polls a fixed producer list.
- The manager timestamps successful batches.
- Channel ownership is exclusive and validated during manager initialization.
- The store owns independently addressable latest channel values.
- Batch publication, invalidation, and multi-channel copy-out are atomic.
- Hardware identity remains internal to producers and composition.
- Public APIs adapt stored channels into application-facing response shapes.
- Temperature uses milli-degrees Celsius, relative humidity uses milli-percent
  RH, and pressure uses pascals internally.
- `Bme280Producer` adapts `Bme280Sensor` into the generic producer contract.

## Decisions To Finalize Before Implementation

No architectural decisions remain open for the initial implementation.
