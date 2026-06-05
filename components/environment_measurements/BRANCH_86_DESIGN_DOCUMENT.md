# Branch 86 Environment Measurements Design

## Goal

`environment_measurements` owns the product's environmental information.

The rest of the application asks the module for environmental values. It does
not know which physical sensors exist, how they communicate, whether values came
from hardware or simulation, or how those values are stored.

The initial implementation uses temperature, humidity, and pressure. The
capability model is pre-filled with likely future indoor and outdoor
environmental measurements so their names and units are established before
additional sensors are integrated.

The initial location is indoor. The design must support an outdoor sensor
connected through a dedicated port.

## Responsibilities

The module owns:

- supported physical sensor objects
- simulated environment sources
- source initialization and detection
- source-to-location bindings
- the polling task
- measurement selection and fallback policy
- latest values by location and capability
- rolling history in RAM
- synchronization for stored data
- the public C API

Concrete source implementations own:

- device-specific initialization
- hardware communication and register protocols
- physical device reads
- conversion from raw values into environment measurements

`board_i2c` continues to own the shared physical I2C bus.

## Core Model

The atomic unit inside the module is one environmental measurement:

```cpp
enum class EnvironmentCapability : uint8_t {
    Temperature,
    Humidity,
    Pressure,
    CarbonDioxide,
    VolatileOrganicCompounds,
    ParticulateMatter1,
    ParticulateMatter2_5,
    ParticulateMatter10,
    Illuminance,
    SoundLevelDeciDbA,
    RainfallSinceMidnight,
    WindSpeed,
    WindDirection,
    UltravioletIndex,
    SoilMoistureRelative,
    Count,
};

union EnvironmentValue {
    int32_t temperature_deci_c;
    int32_t humidity_deci_pct;
    int32_t pressure_deci_hpa;
    uint32_t carbon_dioxide_ppm;
    uint32_t volatile_organic_compounds_ppb;
    uint32_t particulate_matter_1_ug_m3;
    uint32_t particulate_matter_2_5_ug_m3;
    uint32_t particulate_matter_10_ug_m3;
    uint32_t illuminance_lux;
    uint32_t sound_level_deci_dba;
    uint32_t rainfall_since_midnight_deci_mm;
    uint32_t wind_speed_deci_m_s;
    uint16_t wind_direction_degrees;
    uint16_t ultraviolet_index_deci;
    uint16_t soil_moisture_relative_deci_pct;
};

struct EnvironmentMeasurement {
    EnvironmentCapability capability;
    EnvironmentValue value;
    int64_t timestamp_ms;
};

bool make_temperature(
    int32_t deci_c,
    int64_t timestamp_ms,
    EnvironmentMeasurement& out);
bool make_humidity(
    int32_t deci_pct,
    int64_t timestamp_ms,
    EnvironmentMeasurement& out);
bool make_pressure(
    int32_t deci_hpa,
    int64_t timestamp_ms,
    EnvironmentMeasurement& out);
```

`capability` is the tag that determines which union member is valid.
Capability-specific construction helpers must be used instead of directly
constructing tagged values throughout source code. This centralizes union member
selection and basic range validation. `Count` is an internal array-size sentinel
and is never a reportable capability.

Construction helpers return `false` and leave `out` unchanged when a value is
outside the capability-specific range or the timestamp is negative. Sources
must propagate construction failure as an error from `poll()`.
`MeasurementBatch` validates again when accepting a measurement so manually
constructed or test-provided measurements cannot bypass structural validation.
The manager performs validation that depends on the current time before commit.

### Capability Units

| Capability | Stored unit | Example stored value |
|---|---|---|
| Temperature | 0.1 degrees Celsius | `231` = 23.1 C |
| Humidity | 0.1 percent relative humidity | `453` = 45.3% |
| Pressure | 0.1 hectopascals | `10134` = 1013.4 hPa |
| Carbon dioxide | parts per million | `800` = 800 ppm |
| Volatile organic compounds | parts per billion | `120` = 120 ppb |
| PM1 | micrograms per cubic meter | `8` = 8 ug/m3 |
| PM2.5 | micrograms per cubic meter | `12` = 12 ug/m3 |
| PM10 | micrograms per cubic meter | `20` = 20 ug/m3 |
| Illuminance | lux | `500` = 500 lux |
| A-weighted sound level | 0.1 dBA | `425` = 42.5 dBA |
| Rainfall since midnight | 0.1 millimeters | `127` = 12.7 mm |
| Wind speed | 0.1 meters per second | `53` = 5.3 m/s |
| Wind direction | degrees from north | `270` = west |
| Ultraviolet index | 0.1 UV index | `35` = UV index 3.5 |
| Relative soil moisture | 0.1 sensor-relative percent | `684` = 68.4% |

These are canonical module units. A source converts its native sensor units
before reporting a measurement.

Capabilities describe values that a source may directly report. The environment
manager accepts, stores, selects, and exposes those reported values. It does not
derive one capability from others as part of the source-reporting path.

When another capability is added, its units, range, signedness, and semantic
meaning must be defined explicitly before adding it to
`EnvironmentCapability` and `EnvironmentValue`.

## Sources Report Measurements

An environment source represents one physical device, simulator, or test fake.
The manager polls each source once. The source reports every measurement
produced by that poll.

```cpp
constexpr size_t kMaxMeasurementsPerPoll = 8U;

class MeasurementBatch {
public:
    void clear();
    bool report(const EnvironmentMeasurement& measurement);
    size_t size() const;
    const EnvironmentMeasurement& operator[](size_t index) const;

private:
    std::array<EnvironmentMeasurement, kMaxMeasurementsPerPoll> measurements_{};
    size_t count_ = 0U;
};

class EnvironmentSource {
public:
    virtual ~EnvironmentSource() = default;

    // Called once to create permanent resources. May allocate.
    virtual esp_err_t init() = 0;

    // Checks whether the configured hardware is currently available.
    // Must not allocate.
    virtual bool probe() = 0;

    // Configures detected hardware for use. Must not allocate.
    virtual esp_err_t activate() = 0;

    // Performs one logical device read and reports produced measurements.
    // Must not allocate.
    virtual esp_err_t poll(MeasurementBatch& batch) = 0;
};
```

`MeasurementBatch::report()` returns `false` when:

- the fixed batch capacity is full
- the batch already contains the reported capability
- the measurement fails capability-specific validation

Sources must propagate reporting failure as an error from `poll()`.
`kMaxMeasurementsPerPoll` must be at least as large as the largest supported
source's maximum output count.

One poll is committed only when `poll()` returns `ESP_OK`. Measurements reported
before a failed poll are discarded together, preventing partially updated
multi-capability readings.

A BME280 performs one physical read and reports temperature, humidity, and
pressure:

```cpp
esp_err_t Bme280Source::poll(MeasurementBatch& batch) {
    Bme280Reading reading = read_device_once();
    EnvironmentMeasurement temperature{};
    EnvironmentMeasurement humidity{};
    EnvironmentMeasurement pressure{};

    if (!make_temperature(reading.temperature_deci_c, reading.timestamp_ms, temperature) ||
        !make_humidity(reading.humidity_deci_pct, reading.timestamp_ms, humidity) ||
        !make_pressure(reading.pressure_deci_hpa, reading.timestamp_ms, pressure) ||
        !batch.report(temperature) ||
        !batch.report(humidity) ||
        !batch.report(pressure)) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}
```

A temperature-only source performs one physical read and reports one
measurement:

```cpp
esp_err_t TemperatureSource::poll(MeasurementBatch& batch) {
    const TemperatureReading reading = read_temperature();
    EnvironmentMeasurement temperature{};

    return make_temperature(reading.deci_c, reading.timestamp_ms, temperature) &&
                   batch.report(temperature)
               ? ESP_OK
               : ESP_ERR_INVALID_RESPONSE;
}
```

Poll contract:

- sources assign the acquisition timestamp
- all measurements from one physical acquisition should use the same timestamp
- duplicate capabilities reject the complete poll
- `ESP_OK` with an empty batch is treated as a source failure
- the manager validates timestamps and capability-specific value ranges before
  commit
- the manager commits a successful batch as one coherent update

All timestamps use monotonic milliseconds since boot from the same clock domain
as `esp_timer_get_time() / 1000`. Wall-clock time is not used for source
measurements, freshness, or history scheduling.

At commit time, the manager rejects a batch when any timestamp:

- is negative
- is later than the manager's current monotonic time
- is older than the configured maximum accepted acquisition age

The maximum accepted acquisition age must be at least the stale timeout and must
be configured before polling starts. A rejected timestamp rejects the complete
batch.

The environment manager does not know:

- which hardware model produced a measurement
- which communication protocol the source uses
- whether a source reads values together or separately
- whether the source internally uses `get_all()` or individual operations

## Source Lifecycle

Every supported source object is constructed before or during module
initialization and remains alive for the module lifetime.

The source lifecycle methods have distinct responsibilities:

### `init()`

- called once during module initialization
- creates or registers all permanent resources
- may allocate memory because polling has not started
- does not require the physical sensor to be connected
- failure means permanent source setup failed and the source becomes `Disabled`

For an I2C source, `init()` registers and retains the configured device handle.
The handle represents bus configuration and remains valid while the physical
sensor is unplugged.

### `probe()`

- checks whether the configured physical device is currently available
- performs no allocation
- does not perform full device configuration

### `activate()`

- verifies and configures detected hardware
- performs no allocation
- may verify chip identity, reset the device, load calibration, and configure
  measurement settings
- is called again after a disconnected source reconnects

### `poll()`

- performs one logical device measurement
- reports one or more atomic environment measurements on success
- performs no allocation
- failure makes the manager treat the source as unavailable

## Source States

The manager owns runtime source state:

```cpp
enum class SourceState : uint8_t {
    Disabled,
    Unavailable,
    Activating,
    Active,
    Failed,
};
```

State meanings:

- `Disabled`: permanent source setup failed during initialization; do not retry
  because retrying may require forbidden runtime allocation
- `Unavailable`: configured source exists, but hardware is not currently
  detected
- `Activating`: hardware was detected and must be configured
- `Active`: source is configured and can be polled
- `Failed`: activation or operation failed; retry after a configured delay

Runtime behavior:

```cpp
switch (binding.state) {
case SourceState::Disabled:
    break;

case SourceState::Unavailable:
case SourceState::Failed:
    if (binding.source->probe()) {
        binding.state = SourceState::Activating;
    }
    break;

case SourceState::Activating:
    binding.state = binding.source->activate() == ESP_OK
        ? SourceState::Active
        : SourceState::Failed;
    break;

case SourceState::Active:
    poll_batch_.clear();
    if (binding.source->poll(poll_batch_) != ESP_OK ||
        poll_batch_.size() == 0U) {
        binding.state = SourceState::Failed;
    } else {
        commit(binding, poll_batch_);
    }
    break;
}
```

`Failed` is not permanently disabled. The manager retries it after a delay.

## Locations And Bindings

Location belongs to product composition, not the physical sensor class.

```cpp
enum class EnvironmentLocation : uint8_t {
    Indoor,
    Outdoor,
    Count,
};

enum class SourceKind : uint8_t {
    Hardware,
    Simulation,
};

struct SourceBinding {
    EnvironmentSource* source;
    const char* name;
    EnvironmentLocation location;
    SourceKind kind;
    uint8_t priority;
    SourceState state;
};
```

Examples:

- the onboard sensor binding assigns `Indoor`
- a sensor connected to the dedicated external port assigns `Outdoor`
- simulated sources are bound to the location for which they provide fallback

The manager associates a successful batch with the binding that produced it.
Sources do not choose their own location. `Count` is an internal array-size
sentinel and is never assigned to a binding.

## Product Composition

Supported sensors, addresses, ports, locations, source kinds, and priorities are
hard-coded product configuration.

Keep that configuration in one composition file or table:

```cpp
Bme280Source onboard_bme280_{0x76};
Bme280Source external_bme280_{0x77};
SimulatedEnvironmentSource simulated_indoor_{};
SimulatedEnvironmentSource simulated_outdoor_{};

std::array<SourceBinding, 4> bindings_{{
    {&onboard_bme280_, "onboard BME280", Indoor, Hardware, 10, Disabled},
    {&external_bme280_, "external BME280", Outdoor, Hardware, 10, Disabled},
    {&simulated_indoor_, "simulated indoor", Indoor, Simulation, 100, Disabled},
    {&simulated_outdoor_, "simulated outdoor", Outdoor, Simulation, 100, Disabled},
}};
```

During module initialization:

```cpp
ESP_RETURN_ON_ERROR(board_i2c_init(), TAG, "required board I2C initialization failed");

for (SourceBinding& binding : bindings_) {
    if (binding.source->init() != ESP_OK) {
        binding.state = SourceState::Disabled;
        continue;
    }

    if (binding.source->probe()) {
        binding.state = SourceState::Activating;
    } else {
        binding.state = SourceState::Unavailable;
    }
}
```

Physical sensor absence is not a module initialization failure. Failure to
initialize required shared infrastructure is a module initialization failure.
Failure to create permanent resources for one optional source disables that
source while the module continues with other hardware or simulation.

The composition table is the single place changed when the product gains
another configured source or when wiring changes.

## Manager Behavior

The manager owns a fixed collection of source bindings.

For every due polling cycle:

1. poll each active source, including compiled-in simulators, once
2. receive one complete fixed-capacity batch from each successful source
3. commit the complete batch to that binding's candidate values
4. recompute selection for affected location/capability pairs after candidate or
   source-state changes
5. update the latest stored value
6. append selected values to rolling history when the history interval is due
7. select simulated candidates only when preferred hardware candidates are not
   acceptable

The manager stores and exposes measurements. It does not contain
sensor-specific behavior.

## Selection And Fallback

Multiple sources may report the same capability for the same location.

The selection policy uses binding metadata:

1. prefer available hardware over simulation
2. prefer the source with the lowest configured priority value
3. consider only candidates whose source is `Active` and whose value is fresh
4. use simulation when no acceptable hardware candidate is available

The exact stale timeout remains configurable.

Simulation fallback must be clearly logged so missing hardware is visible during
development.

Polling simulators on the normal cadence keeps fresh fallback candidates ready
without selecting or storing their values in graph history while hardware is
preferred.

Selection is a maintained invariant, not only a result of successful polling.
The manager recomputes affected selected values:

- after a successful candidate batch commit
- whenever a source enters or leaves `Active`
- whenever the current time crosses a candidate's freshness deadline
- before copying selected values through a public API
- before appending selected values to history

If no eligible candidate exists, the selected value becomes invalid immediately.
When a selected hardware source fails or becomes stale, an already-fresh
simulated candidate can therefore become selected without waiting for another
simulator poll.

The production scheduler includes the nearest selected-candidate freshness
deadline when calculating its next wake time. Staleness therefore triggers
reselection even when no poll, probe, or public API call occurs at that moment.

## Candidate And Selected State

The manager stores the latest candidate value produced by every source binding:

```text
candidates[binding][capability]
```

Each candidate contains:

```cpp
struct CandidateMeasurement {
    EnvironmentValue value;
    int64_t timestamp_ms;
    bool valid;
};
```

The binding itself provides the candidate's location, source kind, priority,
name, and runtime state. This provenance allows the selection policy to compare
candidates correctly.

After committing a successful source batch or changing source eligibility, the
manager recomputes selected values for affected capabilities:

```text
selected[location][capability]
```

Selected values contain the measurement plus the selected binding index:

```cpp
struct SelectedMeasurement {
    EnvironmentValue value;
    int64_t timestamp_ms;
    size_t binding_index;
    bool valid;
};
```

The selected binding index is retained for diagnostics and to determine when
selection changes. Public APIs do not expose internal binding pointers.

Only selected values are appended to graph history. Candidate values that lose
selection are retained only as latest fallback candidates.

A successful batch replaces that binding's complete candidate set. Previously
reported capabilities omitted from the new successful batch are invalidated.
When a source leaves `Active`, its retained candidates become ineligible until
the source activates and successfully polls again.

## Simulation And Testing

### Development Simulation

`SimulatedEnvironmentSource` allows the complete firmware to run without
physical environment sensors.

It reports plausible changing environmental measurements. It simulates
environment information, not BME280 registers or I2C behavior.

### Automated-Test Fake

`FakeEnvironmentSource` allows tests to control:

- reported measurements
- capabilities
- initialization results
- probe results
- polling failures
- unplug and reconnect behavior

The fake reports through the same `MeasurementBatch` contract as production
sources. Tests bind a fake as `Hardware` or `Simulation` according to the policy
scenario being tested; test implementation type is not part of production
selection policy.

### Testable Controller

Environment policy must be testable without starting FreeRTOS tasks or requiring
physical hardware.

The production task schedules manager operations. The manager and its source
bindings can be instantiated independently in tests.

Manager policy methods receive the current monotonic time explicitly:

```cpp
void process_sources(int64_t now_ms);
bool copy_latest(
    EnvironmentLocation location,
    EnvironmentCapability capability,
    int64_t now_ms,
    EnvironmentMeasurement& out);
void retain_due_history(int64_t now_ms);
```

Production passes `esp_timer_get_time() / 1000`. Tests pass controlled values.
Sources still assign acquisition timestamps, while the manager's explicit
`now_ms` drives timestamp validation, freshness, retry deadlines, selection, and
history cadence. Policy tests therefore do not sleep or depend on FreeRTOS
timing.

## Allocation And Lifetime

The module has a strict post-initialization memory invariant:

> After successful initialization, the module's allocated memory footprint never
> changes.

Heap allocation is permitted during module initialization when required. Before
initialization returns successfully, the module must allocate or construct
everything it may use during operation.

After initialization succeeds, normal operation must not allocate or free heap
memory:

- no `new`, `delete`, `malloc`, or `free` from the polling task
- no containers that grow during operation
- no dynamically growing history
- no source, simulator, synchronization object, queue, or permanent device
  resource is created or destroyed
- runtime plug and unplug changes source state; it does not change object
  lifetime

The module preallocates:

- every supported physical source object
- every compiled-in simulated source
- all source bindings
- latest-value storage
- all rolling-history buffers
- synchronization objects
- any bounded queues
- polling scratch buffers
- the polling task stack and task control block

Source objects remain alive while unavailable. Unplugging a sensor changes its
state to `Unavailable`. Reconnecting activates the same source object. A
compiled-in simulator also remains alive at all times; fallback changes
selection state rather than constructing a simulator.

Permanent device resources, including configured I2C device handles, are retained
through disconnect and reconnect. Runtime recovery reuses the existing source
object and resources, then calls `activate()` again.

The FreeRTOS task must use static task allocation:

```cpp
constexpr size_t kTaskStackBytes = 4096U;

StaticTask_t task_control_block_{};
StackType_t task_stack_[kTaskStackBytes]{};

task_ = xTaskCreateStatic(
    task_entry,
    "environment",
    kTaskStackBytes,
    this,
    kTaskPriority,
    task_stack_,
    &task_control_block_);
```

This ensures the task control block and stack are part of the module's
preallocated storage rather than allocated when polling starts. This definition
uses ESP-IDF's byte-sized task-stack API and byte-sized Xtensa/RISC-V
`StackType_t`; retain a compile-time assertion if portability beyond ESP-IDF is
introduced.

Short-lived `EnvironmentMeasurement` transport objects may remain stack-local.
A stack-local object performs no heap allocation because the complete task stack
is already allocated. The manager should reuse one module-owned
`MeasurementBatch` as polling scratch because sources are polled serially.
Sources may also reuse preallocated member transport objects when useful, but
`MeasurementBatch::report()` must copy each measurement and never retain a
pointer or reference to it.

The task's allocated stack size never changes. Its high-water mark may change
when a rarely used path, such as reconnect or error handling, uses more of the
already allocated stack. This does not represent a new allocation.

After initialization:

```text
heap allocations by environment_measurements: zero
heap frees by environment_measurements: zero
source and simulator objects created or destroyed: zero
history capacity changes: zero
task stack allocation changes: zero
```

The production ownership model must not prevent isolated manager instances in
tests.

The no-allocation contract must be verified, not only documented:

- audit every source call path used by `probe()`, `activate()`, and `poll()`
- measure free heap before and after repeated polling, fallback, and reconnect
  scenarios
- fail debug/test builds when the module causes post-initialization heap changes
  where practical
- record and inspect task stack high-water marks after normal, error, and
  reconnect paths

## Stop And Shutdown Policy

Permanent environment resources have process lifetime in production:

- source objects remain constructed
- I2C device handles remain registered
- mutexes and history storage remain available
- task stack and control-block storage remain allocated

The public API may stop polling, but normal production shutdown does not destroy
or free permanent resources. Replace teardown-oriented `deinit()` semantics with
an idempotent `stop()` operation if a stop API is required:

```c
void environment_measurements_stop(void);
```

`stop()` is cooperative. It signals the polling task to stop, wakes it if
necessary, and waits for the task to acknowledge that it has exited its polling
loop. The task finishes its current source operation and releases every mutex
before acknowledging the stop. Only then may the static task be deleted or left
suspended.

`stop()` must not externally delete or suspend the task while it may hold a
mutex, perform I2C communication, or mutate manager state. It does not unregister
device handles or free module storage. Restarting reuses the same preallocated
resources.

The stop signal and acknowledgement use preallocated synchronization objects.
Repeated `stop()` calls are idempotent. A concurrent `start()` or `stop()` is
serialized by a lifecycle mutex, and public data-copy APIs remain usable while
polling is stopped.

A true teardown path that unregisters handles and frees initialization-time heap
allocations is outside the fixed-footprint production lifecycle and should exist
only if a concrete shutdown or test requirement needs it.

## Synchronization

Physical source polling and device communication happen without holding a data
store mutex.

Source runtime state, candidates, and selected values are protected by the
latest-state mutex. Source polling and probing occur without that mutex; their
results are applied afterward while holding it.

After a source operation completes, the manager takes the latest-state mutex and
performs one coherent state update:

1. apply any source-state transition
2. replace that binding's complete candidate set after a successful poll
3. recompute affected selected values
4. update compatibility latest state
5. release the mutex

Latest-value and compatibility APIs receive the current monotonic time, hold the
same mutex while refreshing affected selection state, and then copy data into
caller-owned storage. They do not poll sources or perform device communication.

History has a separate mutex because copying many history samples can take
longer than copying latest state. History append and history copy-out never hold
the latest-state mutex at the same time, preventing nested-lock ordering issues.

## Latest State

The selected state stores values independently by location and capability.
Independent timestamps and validity allow one location to combine values from
different sources:

```text
indoor temperature <- precision temperature source
indoor humidity    <- BME280 source
indoor pressure    <- BME280 source
```

## Rolling History

The module owns bounded circular history buffers in RAM for configured graph
channels.

Conceptually:

```text
configured history channels:
    Indoor + Temperature
    Indoor + Humidity
    Indoor + Pressure
```

Pre-filling `EnvironmentCapability` does not reserve rolling history for every
capability and location. Only explicitly configured graph channels receive
history storage:

```cpp
struct HistoryChannelDefinition {
    EnvironmentLocation location;
    EnvironmentCapability capability;
};

constexpr std::array history_channel_definitions{
    HistoryChannelDefinition{Indoor, Temperature},
    HistoryChannelDefinition{Indoor, Humidity},
    HistoryChannelDefinition{Indoor, Pressure},
};
```

Each configured channel owns one fixed-capacity circular buffer. A history
sample does not repeat location, capability, or validity because those are
defined by the containing channel and only valid selected values are appended:

```cpp
struct HistorySample {
    // Time at which this selected value was retained for graphing.
    int64_t timestamp_ms;
    EnvironmentValue value;
};
```

Required properties:

- channels and capacities are chosen before polling starts
- no allocation while pushing samples
- oldest samples are overwritten when full
- callers receive copies, never pointers into mutable internal storage
- writes and copy-out operations are synchronized
- only fresh selected values are appended

History timestamps represent the graph-retention time, not necessarily the
source acquisition time. This permits a regular graph interval even when source
polling and history retention use different cadences.

The retained-history interval may differ from the source polling interval. For
example, sources may be polled every second while graph history stores one value
per minute.

History capacity is calculated from:

```text
history capacity = desired graph duration / retained-history interval
```

RAM usage must be calculated before selecting channels and capacities:

```text
history RAM =
    configured channel count
    x history capacity
    x sizeof(HistorySample)
```

## Public API

The public API remains a C API and exposes environment information, not source
objects.

### Compatibility API

Existing GUI and UART call sites should remain unchanged during this branch.

```c
bool environment_measurements_get_latest(
    environment_measurement_sample_t* out);
```

This function returns a compatibility payload containing the latest preferred
indoor temperature, humidity, and pressure.

`environment_measurement_sample_t` is an export payload. It does not dictate the
module's internal storage model.

Compatibility payload semantics:

- temperature, humidity, and pressure are taken from selected indoor values
- `valid` is true only when all three values exist and are fresh
- `timestamp_ms` is the oldest timestamp among the three exported values
- when `valid` is false, callers must not use the payload values

Using the oldest timestamp makes compatibility freshness checks conservative and
truthful when the three exported values came from different sources or polling
times.

### Location And Capability APIs

The module can additionally expose:

```c
bool environment_measurements_get_latest_for_location(
    environment_location_t location,
    environment_location_snapshot_t* out);

bool environment_measurements_get_value(
    environment_location_t location,
    environment_capability_t capability,
    environment_value_t* out);

size_t environment_measurements_copy_history(
    environment_location_t location,
    environment_capability_t capability,
    environment_history_sample_t* out,
    size_t max_count);
```

All APIs copy data into caller-owned storage. They do not allocate memory or
expose pointers into internal storage.

## Adding Sources And Capabilities

### Add Another Instance Of An Existing Sensor

Example: add a second BME280 for outdoors.

1. construct another `Bme280Source`
2. bind it to `Outdoor`
3. add the binding to the source collection

No manager, storage, or driver behavior changes are required.

### Add A New Sensor Using Existing Capabilities

Example: add a temperature-only sensor.

1. implement `EnvironmentSource`
2. initialize and poll the physical device
3. report a temperature measurement
4. add its source binding and priority

No manager, history, or public API changes are required.

### Add A Sensor Using Predefined Additional Capabilities

Example: add a device reporting an already-defined set of CO2, VOC, PM2.5, and
PM10 capabilities.

1. confirm the sensor's units can be converted to the canonical module units
2. add a graph-history channel only if the product needs a graph for that
   location and capability
3. implement the source and report its measurements
4. expose new public API values only when another module needs them

Polling and reporting flow remains unchanged.

### Add A Genuinely New Capability

1. define the capability's semantic meaning, units, range, and signedness
2. add its enum value before `EnvironmentCapability::Count`
3. add its tagged union member and safe construction helper
4. add capability-specific validation
5. implement a source that reports it
6. add a graph-history channel only if the product needs one
7. expose it through the public API only when another module needs it

Candidate and selected storage automatically include the new capability because
their fixed arrays are sized using `EnvironmentCapability::Count`. Polling,
selection, and reporting flow remain unchanged.

## Deferred SD-Card Persistence

Writing environment readings to the SD card is desirable but is outside Branch
86.

The RAM state and public API must not depend on SD-card availability. A future
persistence worker should consume completed measurements through a bounded queue
without blocking sensor polling.

## Required Tests

The design requires focused tests for:

- one successful multi-capability batch commits coherently
- a failed or overflowing batch commits nothing
- duplicate capabilities reject the batch
- capabilities omitted from a later successful batch are invalidated
- hardware wins over simulation for the same location and capability
- lower numeric priority wins between equivalent source kinds
- stale and inactive candidates are not selected
- selection is recomputed when a selected source fails without another source
  committing
- selection is recomputed when a candidate becomes stale without another source
  committing
- fallback selects an already-fresh simulated candidate
- reconnect reuses the same source object and permanent resources
- optional source initialization failure disables only that source
- required infrastructure initialization failure fails the module
- future, negative, and excessively old acquisition timestamps reject the
  complete batch
- manager timing tests use controlled monotonic time and do not sleep
- compatibility payload validity requires fresh indoor temperature, humidity,
  and pressure
- compatibility timestamp uses the oldest exported measurement timestamp
- only configured graph channels consume history storage
- history buffers overwrite their oldest samples at capacity
- cooperative stop waits for an in-progress source operation and releases all
  locks before acknowledging completion
- repeated and concurrent start/stop calls preserve lifecycle invariants
- repeated runtime polling, failure, fallback, and reconnect cause no heap
  allocation or freeing

## Implementation Stages

Branch 86 implementation is split into independently reviewable stages. A stage
must satisfy its tests and preserve the compatibility API before the next stage
begins.

### Stage 1: Core Types And Testable Manager

- implement capability values, construction helpers, validation, batches,
  source lifecycle, bindings, explicit manager time, and fake sources
- implement candidate storage, source-state transitions, selection, freshness,
  fallback, and compatibility latest-state generation
- keep polling task integration and rolling history out of this stage

### Stage 2: Production Sources And Polling Lifecycle

- adapt BME280 and simulation to `EnvironmentSource`
- add product composition
- add the statically allocated polling task
- add cooperative stop/restart and post-initialization allocation verification
- preserve existing GUI and UART call sites

### Stage 3: Rolling History

- configure initial history channels, interval, duration, and fixed capacities
- implement synchronized append and copy-out
- verify calculated RAM usage before enabling the configured capacities

### Stage 4: Additional Public APIs

- add only the location/capability APIs required by a concrete consumer
- keep internal binding pointers and candidate storage private

## Decisions Required Before Stage 1

The reporting architecture and ownership boundaries are decided. Implementation
must not start until these Stage 1 constants and policies are selected:

1. capability-specific valid ranges
2. the fixed maximum measurement count per source poll
3. fixed source-binding capacity
4. source priority values, stale timeout, and maximum accepted acquisition age
5. retry policy used by manager state-transition tests

## Decisions Deferred Until Their Stage

These decisions do not block Stage 1:

1. Stage 2: production probe cadence and whether simulation is automatic,
   configurable, or both
2. Stage 3: history channels, graph duration, retained-history interval, and
   fixed capacities
3. Stage 4: the first location/capability APIs beyond the compatibility API
