# Environment Module C++ Design

This document sketches the next branch direction for environment sensing.

The goal is to move from implementation-detail components toward one
responsibility-area module:

`environment_measurements` owns board-local environment sensing.

That means this module should eventually own:

- sensor discovery
- sensor object lifetime
- sensor polling task
- latest environment data
- sensor metadata such as location
- simulation fallback
- the public C API used by the rest of the application

The old components do not need to be deleted first. The safer branch plan is to
keep them available while the new module grows behind the existing
`environment_measurements_*` API.

## Short Decision Summary

We choose a C++ virtual-interface design for environment sensors.

What we gain:

- one controller can own a simple collection of mixed sensor objects
- sensors can be discovered at runtime from the actual I2C devices present
- real and simulated sensors can use the same polling and data path
- the public application API can stay stable while sensor internals change
- sensor location metadata can be handled consistently for every sensor type

What it costs:

- each concrete sensor class has a small vtable
- each sensor call has one virtual dispatch indirection
- object lifetime must be owned carefully by the environment module
- the design is slightly more abstract than a single C HAL context

Why that cost is acceptable:

- BME280 reads are slow I2C transactions, so virtual dispatch overhead is
  irrelevant compared with bus latency
- ESP32 has enough RAM/flash that the small vtable cost is not meaningful here
- the code is easier to extend to multiple sensors, locations, and simulation
  fallback
- avoiding compile-time simulator switches gives one firmware image that works
  both with and without physical sensors

The main trade is: we accept a tiny runtime and memory cost to get a cleaner
runtime architecture that matches the requirement of dynamically managing
multiple environment sensors.

Memory policy:

- sensor objects are stored in fixed module-owned storage
- startup initializes the objects the firmware supports
- runtime plug/unplug changes active/inactive state only
- no runtime sensor `new`/`delete`
- no runtime sensor list growth

This keeps the C++ design deterministic enough for embedded use while still
giving us the polymorphic interface.

UI migration rule:

- keep `environment_measurement_sample_t` unchanged for the first
  implementation
- the UI should only need to change which function it calls to get the latest
  sample
- richer metadata stays internal until a caller genuinely needs a public API
  change

## Current Shape

Relevant components today:

- `components/environment_measurements`
  - C++ wrapper with the public `environment_measurements_*` C API
  - currently owns one `bme280_hal` context
  - currently owns one polling task and latest-sample snapshot
- `components/bme280`
  - C HAL for BME280 hardware or compile-time simulator backend
  - currently selects simulator vs hardware through build configuration
- `components/sensor_data`
  - separate C latest-sample store
  - still used by GUI bindings and UART paths
- `components/local_sensor_service`
  - older C task that bridges `bme280_hal` into `sensor_data`
  - overlaps with `environment_measurements`
- `components/board_i2c`
  - shared board I2C bus owner
  - used by touch, IO extension, and sensors

The new design should make `environment_measurements` the responsibility-area
owner for environment sensing while leaving `board_i2c` as the owner of the
physical I2C bus.

## Main Design Choice

Use dynamic polymorphism for sensor objects:

```cpp
class EnvironmentSensor {
public:
    virtual ~EnvironmentSensor() = default;
    virtual esp_err_t init() = 0;
    virtual esp_err_t read(EnvironmentReading& out) = 0;
    virtual const SensorIdentity& identity() const = 0;
};
```

The controller can then store a small fixed collection of base pointers to
preallocated objects:

```cpp
EnvironmentSensor* sensors_[kMaxEnvironmentSensors];
```

The pointers do not imply heap allocation. They point at objects owned directly
by `EnvironmentMeasurements`, for example:

```cpp
Bme280Sensor real_bme280_0_;
Bme280Sensor real_bme280_1_;
SimulatedBme280Sensor simulated_inside_;
```

The key point is that `EnvironmentMeasurements` loops over
`EnvironmentSensor` objects and does not care whether a specific object is a
real BME280, a simulated BME280, or a future sensor type.

## Why Virtual Functions Fit This Branch

Templates and CRTP are valid C++ tools, but they solve a different problem.
They give compile-time polymorphism and can remove virtual dispatch overhead.

That trade is not useful for slow I2C sensor reads:

- I2C transactions dominate runtime cost
- virtual dispatch cost is tiny compared with a bus transaction
- ESP32 has enough memory that one vtable per class is not a practical issue
- template variants can increase binary size through repeated instantiation

More importantly, template and CRTP sensor types are separate compile-time
types. A `Bme280Sensor<I2cPortA>` and `Bme280Sensor<I2cPortB>` are not naturally
stored in one simple array unless we add another mechanism such as
`std::variant` or a virtual base anyway.

For this branch we want runtime decisions:

- scan expected I2C addresses at boot
- verify chip IDs
- initialize/activate the preallocated sensor objects that match the hardware
  that is actually present
- fall back to simulation when no physical environment sensor is found
- possibly mix real and simulated sensors during development

Those are runtime ownership decisions, so a virtual interface is the most direct
design.

## Responsibility Boundaries

### `environment_measurements` Owns

- the environment sensor controller object
- the list of environment sensors
- mapping discovered sensors to locations
- the polling task
- the latest published environment samples
- simulation fallback policy
- public environment data accessors

This should be thought of as a module that owns a task, not just a task.

The task is only the worker that periodically reads sensors. The module is the
public boundary:

- main initializes and starts the module
- the module creates and owns the polling task
- the module owns the latest data and later history
- UI and other consumers call the module API to read data

That matters because the UI should not talk directly to a FreeRTOS task or a
sensor driver. It should ask the `environment_measurements` module for the
latest environment data.

### Sensor Classes Own

- device-specific initialization
- device-specific register protocol
- compensation/conversion for their own readings
- per-sensor cached state such as calibration data

Example classes:

- `Bme280Sensor`
- `SimulatedBme280Sensor`
- future `Bmp280Sensor` if needed

### `board_i2c` Owns

- the ESP-IDF I2C bus handle
- board-level SDA/SCL/port configuration
- low-level bus operations
- eventual bus-level locking or transaction serialization

Environment sensors should depend on `board_i2c`; they should not create or
delete the board I2C bus.

## I2C Ownership And Concurrency

The environment module cannot own the I2C bus because the touch controller and
other board devices use the same bus.

The correct dependency direction is:

```text
environment_measurements -> board_i2c -> ESP-IDF I2C driver
touch                    -> board_i2c -> ESP-IDF I2C driver
io_extension             -> board_i2c -> ESP-IDF I2C driver
```

The current `board_i2c` helpers already centralize access, but they do not yet
provide a clear bus manager contract. That should be tracked as a separate
follow-up.

Future `board_i2c` improvements:

- add a mutex around bus transactions if ESP-IDF does not already provide the
  required serialization for this usage
- avoid duplicate device handle registration for the same address
- optionally expose scoped transaction helpers
- keep scan/probe diagnostics centralized

For this branch, the design should explicitly avoid each sensor creating its own
bus. Sensor classes should receive an address and use `board_i2c_add_device()`
or a future `board_i2c_get_device()` helper.

## Sensor Discovery

Discovery should be targeted, not a fully generic I2C plug-and-play system.

For BME280, the known addresses are:

- `0x76`
- `0x77`

The BME280 chip ID register is:

- register `0xD0`
- expected BME280 chip ID `0x60`

The startup flow should be:

1. Ask `board_i2c` to initialize the shared bus.
2. Probe known BME280 addresses.
3. For each ACKing address, create a temporary device handle or use a probe
   helper to read chip ID register `0xD0`.
4. If the chip ID is `0x60`, mark the matching preallocated `Bme280Sensor`
   usable.
5. Attach location metadata based on the board wiring/configuration.
6. If no real environment sensors are found, activate the preallocated simulated
   sensor.

This is not globally unique device identification like USB. It is a practical
embedded rule:

```text
address 0x76 + chip ID 0x60 = expected BME280 at configured location
address 0x77 + chip ID 0x60 = expected BME280 at configured location
```

That is good enough because this product controls the expected sensor set.

## BME280 Hardware Configuration

Physical BME280 sensors need explicit configuration during initialization.

The Bosch BME280 datasheet defines the relevant runtime configuration registers:

- `0xF2 ctrl_hum`: humidity oversampling `osrs_h`
- `0xF4 ctrl_meas`: temperature oversampling `osrs_t`, pressure oversampling
  `osrs_p`, and mode
- `0xF5 config`: standby time `t_sb`, IIR filter coefficient `filter`, and
  3-wire SPI enable

The first implementation should expose the useful BME280 options through
Kconfig, but still keep safe defaults.

### Configurable Options

Oversampling options for temperature, pressure, and humidity:

```text
0 = skipped
1 = x1
2 = x2
3 = x4
4 = x8
5 = x16
```

Sensor mode:

```text
0 = sleep
1 = forced
2 = forced
3 = normal
```

IIR filter coefficient:

```text
0 = off
1 = 2
2 = 4
3 = 8
4 = 16
```

Normal-mode standby time:

```text
0 = 0.5 ms
1 = 62.5 ms
2 = 125 ms
3 = 250 ms
4 = 500 ms
5 = 1000 ms
6 = 10 ms
7 = 20 ms
```

SPI 3-wire should stay disabled for this project because the BME280 is used over
I2C.

### First Defaults

Use forced mode by default because our FreeRTOS task already owns the sampling
cadence through `CONFIG_REDMOLE_ENVIRONMENT_READING_INTERVAL_SEC`.

Recommended first defaults:

```text
mode = forced
temperature oversampling = x1
pressure oversampling = x1
humidity oversampling = x1
IIR filter = off
standby time = 1000 ms, ignored by forced mode but configured safely
SPI 3-wire = disabled
```

This matches the product use case: low-rate weather/environment readings, not
high-frequency indoor navigation.

If we later use BME280 normal mode, then `t_sb` becomes important because the
sensor itself cycles measurements internally. In forced mode, the task cadence
is the authoritative interval.

### Register Write Order

When configuring the BME280:

1. verify chip ID `0x60`
2. optionally soft reset with `0xE0 = 0xB6`
3. wait until NVM copy is complete using status register `0xF3`
4. read calibration data
5. write `ctrl_hum` at `0xF2`
6. write `config` at `0xF5` while the device is in sleep mode
7. write `ctrl_meas` at `0xF4` last

`ctrl_hum` changes only become effective after writing `ctrl_meas`, so
`ctrl_meas` must be written after `ctrl_hum`.

### Kconfig Names

Proposed Kconfig symbols:

```text
CONFIG_REDMOLE_BME280_MODE
CONFIG_REDMOLE_BME280_OVERSAMPLING_TEMPERATURE
CONFIG_REDMOLE_BME280_OVERSAMPLING_PRESSURE
CONFIG_REDMOLE_BME280_OVERSAMPLING_HUMIDITY
CONFIG_REDMOLE_BME280_IIR_FILTER
CONFIG_REDMOLE_BME280_STANDBY_TIME
```

The implementation should validate/clamp these values to the datasheet-defined
bit patterns before writing registers.

## Sensor Location

Each sensor needs metadata that explains what the reading means in the product.

Use a small enum instead of strings for internal logic:

```cpp
enum class SensorLocation : uint8_t {
    Inside,
    Outside,
    Unassigned,
};
```

A reading should carry location:

```cpp
struct EnvironmentReading {
    int64_t timestamp_ms;
    int64_t wall_time_unix_ms;
    int32_t temperature_deci_c;
    int32_t humidity_deci_pct;
    int32_t pressure_deci_hpa;
    SensorLocation location;
    bool has_temperature;
    bool has_humidity;
    bool has_pressure;
    bool wall_time_valid;
    bool valid;
};
```

This is an internal C++ model for the environment module. It does not require
changing the first public C sample struct.

Use two timestamp concepts:

- monotonic sample time from `esp_timer`, used for freshness and age checks
- optional wall-clock time, used for logs/history when SNTP or another time
  source is synchronized

The monotonic timestamp should always be present for valid samples. It answers:

```text
How old is this sample compared with now?
```

The wall-clock timestamp may be invalid during startup if the device has not
synced time yet. It answers:

```text
What real date/time did this measurement happen?
```

Those should not be confused. A sample can be fresh and valid even if its
wall-clock timestamp is not valid yet.

## Different Sensor Value Sets

Different environment sensors may not report the same values.

Examples:

- BME280 reports temperature, humidity, and pressure
- BMP280 reports temperature and pressure, but not humidity
- a simple temperature sensor reports only temperature
- a future sensor might report air quality, light, CO2, or something else

Use two layers of data:

1. device-specific readings inside each sensor driver
2. normalized environment readings published by the environment module

For example, a BME280 class can use its own internal type:

```cpp
struct Bme280Reading {
    int32_t temperature_deci_c;
    int32_t humidity_deci_pct;
    int32_t pressure_deci_hpa;
};
```

A future temperature-only class can use a smaller internal type:

```cpp
struct TemperatureReading {
    int32_t temperature_deci_c;
};
```

But the module should publish a common shape so callers do not need to know each
driver's private structs. Internally, the common reading can say which fields
are present.

```cpp
struct EnvironmentReading {
    int64_t timestamp_ms;
    int64_t wall_time_unix_ms;
    int32_t temperature_deci_c;
    int32_t humidity_deci_pct;
    int32_t pressure_deci_hpa;
    SensorLocation location;
    bool has_temperature;
    bool has_humidity;
    bool has_pressure;
    bool wall_time_valid;
    bool valid;
};
```

The rule is:

- if `has_temperature` is true, `temperature_deci_c` is meaningful
- if `has_humidity` is true, `humidity_deci_pct` is meaningful
- if `has_pressure` is true, `pressure_deci_hpa` is meaningful

This avoids fake values. A BMP280 reading should not invent humidity just to fit
the struct. It should publish temperature and pressure, with `has_humidity` set
to false.

If a future sensor reports values that do not fit the current environment data
model, add new fields and presence booleans deliberately. Do not make every
caller depend on device-specific structs unless that caller truly needs
device-specific diagnostics.

For the first public C API, keep the existing `environment_measurement_sample_t`
shape. Because the first real sensor is BME280-compatible, the compatibility
sample can still contain temperature, humidity, and pressure without changing
the GUI data model.

Sensor identity should also be explicit:

```cpp
enum class SensorKind : uint8_t {
    Bme280,
    SimulatedBme280,
};

struct SensorIdentity {
    SensorKind kind;
    SensorLocation location;
    uint8_t i2c_address;
    bool simulated;
};
```

For the first pass, location can be hardcoded from address. The current project
has one environment sensor, and its default location is inside:

```text
0x76 -> Inside
0x77 -> Inside
```

If that becomes too rigid, move the mapping into a small configuration table:

```cpp
struct Bme280DiscoveryRule {
    uint8_t address;
    SensorLocation location;
};
```

## Simulation Fallback

Simulation should become a runtime fallback, not only a compile-time backend.

If a teammate does not have the physical sensor, the same firmware should still
run. At boot:

- scan for real sensors
- initialize preallocated real sensor objects for what is found
- always initialize the preallocated simulated sensor
- choose which source is active for the inside location

The simulated sensor implements the same `EnvironmentSensor` interface. It can
generate plausible values using time-based sine waves:

- temperature around a baseline, for example 22.0 C
- humidity around a baseline, for example 45.0 percent
- pressure around a baseline, for example 1013.0 hPa

The rest of the application does not need `#ifdef SIMULATION` branches. It just
asks `environment_measurements` for environment data.

This is one of the main wins of the virtual design: real and simulated sensors
can live in the same collection.

Runtime plug/unplug behavior is state-based, not allocation-based:

```text
real BME280 present    -> inside active source = real BME280
real BME280 unplugged  -> inside active source = simulated BME280
real BME280 replugged  -> inside active source = real BME280
```

No object is created or destroyed during this transition. The module only marks
the real sensor active/inactive and switches which preallocated source publishes
the inside reading.

## Latest Data Model

The current public API exposes one latest sample:

```c
bool environment_measurements_get_latest(environment_measurement_sample_t* out);
```

With multiple sensors, a single sample becomes ambiguous. We need one of these
API directions:

### Option A: Keep One Aggregate Latest Sample

`environment_measurements_get_latest()` returns the preferred/aggregate
environment sample.

Examples:

- if inside exists, return inside
- if outside exists, return outside
- average all valid sensors
- return first valid sensor

This is backward-compatible but hides sensor locations.

### Option B: Add Location-Based Reads

Keep the old function for compatibility and add:

```c
bool environment_measurements_get_latest_for_location(
    environment_sensor_location_t location,
    environment_measurement_sample_t* out);
```

This is probably the best first public API extension because inside/outside is a
real product concept.

### Option C: Add Enumeration/Copy-Out APIs

For GUI or diagnostics:

```c
size_t environment_measurements_copy_latest(
    environment_measurement_sample_t* out,
    size_t max_count);
```

This gives callers all latest samples without exposing internal storage.

Recommended branch direction:

1. keep the existing `get_latest()` API working
2. keep the public sample struct unchanged
3. add location to the internal C++ reading model
4. later add a location-based C API if callers need it
5. later add a bounded copy-out API if the GUI needs to show all sensors

## Application Integration Points

The rest of the system should only need a few direct changes when
`environment_measurements` becomes the owner.

### Main Initialization

`main.c` should initialize the module during single-instance startup:

```c
rv = environment_measurements_init();
```

This call prepares the shared environment module state, probes hardware, and
marks preallocated sensor objects active or inactive. It should not start
polling yet.

### Main Startup

`main.c` should start the module during runtime startup:

```c
rv = environment_measurements_start();
```

This call starts the environment sensing task owned by the module.

### UI Data Read

The GUI binding should stop reading local temperatures from `sensor_data` once
the migration is done. It should call the environment module instead:

```c
environment_measurement_sample_t sample = {0};
if (environment_measurements_get_latest(&sample) && sample.valid) {
    /* update GUI state */
}
```

Possible future location-based API, not required for the first UI migration:

```c
environment_measurements_get_latest_for_location(
    ENVIRONMENT_SENSOR_LOCATION_OUTSIDE,
    &sample);
```

That keeps the GUI dependent on the environment responsibility area instead of
the old data store or a concrete BME280 implementation.

The UI should prefer one snapshot call over separate value calls.

Good:

```c
environment_measurement_sample_t sample = {0};
if (environment_measurements_get_latest(&sample)) {
    /* render temperature, humidity, pressure from the same sample */
}
```

Avoid as the main UI API:

```c
environment_measurements_get_temperature(...);
environment_measurements_get_humidity(...);
environment_measurements_get_pressure(...);
```

Separate getters can accidentally read different samples if the polling task
publishes between calls. A single copy-out snapshot gives the GUI one coherent
view of the latest environment reading.

## Proposed Internal Layout

One possible first implementation:

```cpp
class EnvironmentMeasurements {
public:
    esp_err_t init();
    esp_err_t start();
    void deinit();

    bool get_latest(environment_measurement_sample_t* out) const;

private:
    esp_err_t discover_sensors();
    void task_loop();
    void publish(const EnvironmentReading& reading);

    Bme280Sensor real_bme280_0_;
    Bme280Sensor real_bme280_1_;
    SimulatedBme280Sensor simulated_inside_;
    EnvironmentSensor* active_inside_;

    EnvironmentReading latest_by_location_[kLocationCount];
    environment_measurement_sample_t compatibility_latest_;
    std::atomic_uint version_;
    std::atomic_uint update_count_;
    TaskHandle_t task_;
    bool initialized_;
};
```

The module owns all sensor objects directly. `active_inside_` is only a selector
for which preallocated sensor currently provides the inside reading.

A location-specific getter can be added later if the GUI or another caller needs
inside/outside separately. It is not required for the first migration.

## Migration Plan

### Step 1: Design And Boundaries

Add this design document and agree on the responsibility boundary:

- `environment_measurements` becomes the new owner
- old components stay temporarily
- `board_i2c` remains the shared bus owner

### Step 2: Introduce C++ Sensor Interfaces Internally

Inside `components/environment_measurements/src`, add internal C++ classes:

- `EnvironmentSensor`
- `Bme280Sensor`
- `SimulatedBme280Sensor`
- small structs/enums for identity, location, readings

Keep the public C header stable at first.

### Step 3: Move BME280 Hardware Logic Behind `Bme280Sensor`

Either:

- wrap the existing `bme280_hal` from the new C++ object first, or
- move the BME280 hardware implementation directly into the new C++ class

Wrapping first is lower risk. Moving later is cleaner.

### Step 4: Add Discovery

Replace the single `bme280_hal_init()` path in `EnvironmentMeasurements::init()`
with targeted discovery:

- probe `0x76` and `0x77`
- read chip ID
- initialize/activate preallocated `Bme280Sensor` objects
- attach configured locations
- activate simulated fallback if none are found

### Step 5: Update Data APIs

Keep:

- `environment_measurements_get_latest()`
- `environment_measurements_is_fresh()`
- `environment_measurements_get_update_count()`

Later additions, not required for the first implementation:

- location enum in the C public API
- `environment_measurements_get_latest_for_location()`
- optional copy-out API for all latest sensor readings
- timestamp validity metadata if wall-clock synchronization matters to readers

### Step 6: Retire Overlap

Once `environment_measurements` is feeding the UI and UART paths, decide what to
do with old components:

- remove `local_sensor_service` if unused
- either remove `sensor_data` or redefine it as a broader application data store
- either remove `bme280` as a separate component or keep it as a low-level driver
  used only by `environment_measurements`

Do this only after callers have moved.

### Step 7: Board I2C Follow-Up

In a separate issue/branch, improve `board_i2c` into a stronger bus manager:

- transaction mutex if needed
- address/device handle registry
- duplicate handle policy
- clear rules for touch and sensors sharing the bus

This is related but should not block the environment module design.

## Implementation Readiness Checklist

These are the concrete choices for the first implementation.

### Public C API Shape

Keep the existing API:

```c
esp_err_t environment_measurements_init(void);
esp_err_t environment_measurements_start(void);
void environment_measurements_deinit(void);
bool environment_measurements_get_latest(environment_measurement_sample_t* out);
bool environment_measurements_is_fresh(uint32_t max_age_ms);
uint32_t environment_measurements_get_update_count(void);
```

Do not add new public getters for the first implementation unless they are
needed by a caller. The first migration goal is:

```c
bool environment_measurements_get_latest(environment_measurement_sample_t* out);
```

The GUI should move from the old data source to this one call and keep using the
same sample shape.

Possible later API for multiple locations:

```c
typedef enum {
    ENVIRONMENT_SENSOR_LOCATION_INSIDE,
    ENVIRONMENT_SENSOR_LOCATION_OUTSIDE,
    ENVIRONMENT_SENSOR_LOCATION_UNASSIGNED,
} environment_sensor_location_t;

bool environment_measurements_get_latest_for_location(
    environment_sensor_location_t location,
    environment_measurement_sample_t* out);
```

Do not add separate public getters for temperature, humidity, or pressure in the
first implementation. The public measurement type is
`environment_measurement_sample_t`, and callers copy one latest sample out of
the module.

Do not change `environment_measurement_sample_t` in the first implementation.

Decision:

- keep the public sample struct stable
- keep `timestamp_ms` as monotonic uptime in milliseconds
- keep the existing temperature, humidity, pressure, and valid fields
- make the UI migration require only one function-call change
- keep richer internal metadata inside C++ for now
- add public location/value-presence/wall-clock fields only in a later API
  version if callers actually need them

### Sensor Capacity And Allocation

Use a fixed first-pass maximum:

```cpp
constexpr size_t kMaxEnvironmentSensors = 2;
```

That matches BME280 addresses `0x76` and `0x77`.

Use fixed storage and no runtime heap allocation for sensor objects.

Decision:

```cpp
Bme280Sensor real_bme280_0_;
Bme280Sensor real_bme280_1_;
SimulatedBme280Sensor simulated_inside_;

EnvironmentSensor* sensors_[kMaxEnvironmentSensors];
size_t sensor_count_;
```

Rules:

- no `new` for sensor objects
- no `delete` for sensor objects
- no creating or destroying sensor objects after startup
- discovery at startup decides which preallocated real sensors are usable
- runtime unplug/replug only changes sensor state, not object lifetime

This avoids heap fragmentation and keeps memory ownership deterministic.

### Discovery Rules

Use this first discovery table:

```cpp
static constexpr Bme280DiscoveryRule kBme280Rules[] = {
    {0x76, SensorLocation::Inside},
    {0x77, SensorLocation::Inside},
};
```

This table is the one place to change if wiring changes.

Decision:

- `0x76` means inside
- `0x77` means inside
- BME280 chip ID must be `0x60`
- unsupported chip IDs are logged and ignored

### Compatibility Latest Policy

The old API returns one latest sample, but multiple sensors may exist.

Decision:

1. prefer inside if a fresh inside sample exists
2. otherwise prefer outside if a fresh outside sample exists
3. otherwise return the most recent valid sample from any location

This keeps old GUI/code paths working while new location-based APIs are added.

### Freshness And Validity

Use separate concepts:

- public `valid`: the compatibility sample contains a successful publishable
  sensor reading
- freshness: computed from monotonic public `timestamp_ms`
- internal `has_temperature`, `has_humidity`, `has_pressure`: which values are
  meaningful before converting to the compatibility sample
- internal `wall_time_valid`: real-world time is synchronized and
  `wall_time_unix_ms` can be trusted

Decision:

- always set monotonic `timestamp_ms` on successful reads
- do not reject a sensor sample just because wall-clock time is not synced
- use `environment_measurements_is_fresh()` for stale-data checks
- use `valid == false` only for empty/unpublishable sample slots, not for
  missing optional fields

### Polling And Publishing

The task should loop over all discovered sensors:

```text
for each sensor:
    read sensor
    if read succeeds:
        publish latest for that sensor location
update compatibility latest
delay polling period
```

Decision:

- use one module-level polling period for all environment sensors in the first
  implementation
- read the polling period from Kconfig as seconds
- if Kconfig is set to `60`, the task should publish roughly once per minute
- revisit per-sensor polling periods only when a second real sensor type needs it

Use a non-drifting task cadence:

```cpp
const TickType_t interval_ticks =
    pdMS_TO_TICKS(CONFIG_REDMOLE_ENVIRONMENT_READING_INTERVAL_SEC * 1000U);
TickType_t last_wake = xTaskGetTickCount();

while (true) {
    read_and_publish_environment();
    vTaskDelayUntil(&last_wake, interval_ticks);
}
```

`vTaskDelayUntil()` is the right FreeRTOS primitive here because it schedules the
next wake relative to the previous wake time. That means sensor execution time is
included in the period. If the interval is 60 seconds and the sensor read takes
200 ms, the task sleeps for about 59.8 seconds instead of drifting to 60.2
seconds per cycle.

Kconfig should clamp the interval to a reasonable range, for example:

```text
config REDMOLE_ENVIRONMENT_READING_INTERVAL_SEC
    int "Environment reading interval in seconds"
    default 60
    range 1 3600
```

### Error Handling

Decision:

- absence of all real sensors is not fatal; create simulated fallback
- simulated fallback creates one inside simulated BME280 by default
- if a previously working physical sensor stops responding, log a clear message:
  "physical environment sensor unplugged; falling back to simulation"
- if a physical sensor appears again later, log that hardware was detected and
  resume physical readings if the first implementation supports re-detection
- one failed read is not fatal; log at warning level when useful
- init failure is fatal only for module-owned resources that are required for
  operation, such as failing to initialize `board_i2c`
- duplicate or unsupported devices are ignored with a diagnostic log

### Thread Safety

The polling task writes samples. GUI/main reads samples.

Decision: keep the existing versioned snapshot pattern already used in
`environment_measurements.cpp`. Extend it to per-location latest samples instead
of introducing a new mutex in the first implementation.

Do not hold an I2C/bus lock while publishing to the data store.

Why this is reasonable here:

- there is one writer, the environment polling task
- readers only need small copy-out snapshots
- reads should not block the sensor polling task
- the existing code already uses this pattern, so extending it is lower risk
- the version counter prevents readers from accepting a half-written sample

This pattern is not a universal replacement for a mutex. If later data grows
into larger history buffers, multi-sample iteration, or complex mutation across
several structures, use a mutex or owner-task message queue for those parts.
For the first latest-sample API, the versioned snapshot pattern is a good fit.

### First Code Change Boundaries

First implementation should touch only:

- `components/environment_measurements/include/environment_measurements.h`
- `components/environment_measurements/src/environment_measurements.cpp`
- `components/environment_measurements/CMakeLists.txt` if sources are split
- `main/Kconfig.projbuild` or a component Kconfig file for the reading interval
  and BME280 hardware configuration
- the GUI binding file that currently reads `sensor_data`
- `main.c` only if the init/start calls or includes need adjustment

Do not delete `bme280`, `sensor_data`, or `local_sensor_service` in the first
implementation unless they become truly unused and the build proves it.

Decision:

- keep implementation inside `environment_measurements.cpp` first
- split into additional `.cpp`/`.hpp` files only if the file becomes difficult
  to navigate
- keep old components in the tree for this branch

### Build Verification

Minimum verification:

```bash
idf.py build
```

If C API structs change, also search for all users:

```bash
rg "environment_measurements_|sensor_data_get_latest_local|sensor_data_sample"
```

Decision:

- run `idf.py build` after the first implementation
- do not run clang-format unless touched C/C++ files are explicitly added to
  `tools/clang-format-files.txt`

## Course Notes Evaluation

These notes are relevant, but not all of them should be implemented in the first
branch. The main architectural decision is to combine sensor task and data store
inside one responsibility-area module, then keep lower-level I2C ownership in
`board_i2c`.

### Adopt Now

Combine sensor task and datastore:

- yes
- the awkward data passing between sensor task and data store is a design smell
- the first implementation should make `environment_measurements` own both
  polling and latest environment data
- this is a responsibility boundary, not an implementation-detail boundary

Protect I2C ownership in the I2C module:

- yes as a design rule
- environment sensors must not own the bus
- sensors call `board_i2c`
- stronger protection, such as mutex or manager, belongs in `board_i2c`

Use copy-out APIs for UI reads:

- yes
- GUI should ask the module for the latest copied snapshot
- do not hand GUI pointers into internal storage

### Reasonable But Not First Step

Use semaphores to wait for new readings:

- useful if a consumer task must sleep until a fresh sample arrives
- not needed for the GUI path if GUI already polls/syncs periodically
- can be added later as `environment_measurements_wait_for_update(timeout_ms)`
  or event notification

Use a queue with `xQueueOverwrite()`:

- good pattern for "latest value only" delivery to exactly one queue consumer
- less useful as the main datastore because multiple consumers may need reads
- could be used internally for event-style notification, but the module should
  still own the latest sample

Use a hardware timer for sensor polling:

- probably unnecessary for BME280
- BME280/I2C sampling is slow and not hard real-time
- FreeRTOS task delay is good enough for periodic environment sensing
- hardware timer may be useful only if the assignment specifically requires
  practicing timers

### Avoid For This Design

Run I2C sensor sampling directly from an ISR:

- do not do this
- I2C transactions are not appropriate ISR work
- sensor reads can block, retry, allocate/use driver locks, and log errors
- ISR should at most notify a task, not perform the sensor transaction

Use implementation-detail module split:

- avoid this
- separating `sensor_task`, `sensor_data`, and `bme280` because they are
  different implementation mechanisms made integration awkward
- the product concept is "environment readings", so that should be the module
  boundary

### Separate Follow-Up

I2C priority and arbitration:

- this belongs in `board_i2c`, not in the environment module
- touch and environment sensors share the bus
- touch likely has higher user-experience priority than slow environment polling
- environment polling can tolerate delay or skipped samples
- first policy can be a simple mutex with short transactions
- later policy could add a bus manager queue with request priorities

Possible first `board_i2c` policy:

```text
touch: high priority, latency-sensitive
environment sensor: low priority, periodic, may wait
io extension: normal priority, short transactions
```

For the first environment implementation, do not solve this inside
`environment_measurements`. Note it as a bus-manager issue and keep all bus
access going through `board_i2c`.

## Open Questions

- Should `sensor_data` remain as the application data owner, or should
  `environment_measurements` fully replace it for local environment readings?
- Does the current ESP-IDF I2C master bus usage need explicit locking between
  touch and sensor transactions, or is the driver already serializing enough for
  this configuration?

Resolved for first implementation:

- `0x76` maps to inside and `0x77` maps to inside
- existing GUI paths can keep using the compatibility latest sample first
- simulated fallback creates one inside simulated BME280
- samples expose both monotonic uptime and wall-clock validity fields, but
  wall-clock validity may stay false until time sync is integrated

## Recommended First Branch Goal

For this branch, aim for a small but meaningful architectural step:

1. keep old components in place
2. make `environment_measurements` internally manage a collection of virtual
   `EnvironmentSensor` objects
3. support discovery of BME280 at `0x76` and `0x77`
4. add simulated fallback when no real BME280 is found
5. keep the existing public API working

That proves the new responsibility boundary without forcing a repo-wide cleanup
in the same change.
