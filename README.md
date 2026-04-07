
## WIP förslag, jimpa
Det finns så klart fortfarande ogenomtänkta och felaktiga delar i detta dokument, så det ska inte läsas som ett finalt förslag.

## Purpose
This proposal aims to continue the good direction of the original proposal while limiting scope, clarifying ownership, and trimming unnecessary complexity. The goal is to stay concrete and close to implementation, with focus on actual technical decisions, ownership, and data flow rather than broad pattern language.

## Strengths Kept From The Original Direction

1. A modular system structure with clear functional areas.

2. FreeRTOS as the runtime foundation, with separate tasks where task separation is justified.

3. A watchdog and supervisor concept for robustness and fault recovery.

4. HAL boundaries for hardware-near parts such as sensor, display, and communication.

5. A focus on diagnostics, runtime visibility, and explainable system behavior.

## Weaknesses This Proposal Resolves

1. The original direction identifies modules and coordination concerns, but it does not define clearly enough where data lives, who owns it, and how history and queries should work.

2. Building one large top-level runtime object out of many smaller module contexts risks turning startup infrastructure into a shared state container. Once many modules depend on pointers into that object, ownership becomes weaker and accidental cross-module mutation becomes much easier.

3. Most state should have one clear owner, and the synchronization strategy should fit the actual problem. Depending on the case, that may mean queues, event groups, double buffering, a ring buffer, a bounded critical section, or a mutex where a mutex is actually the right tool. 

4. State machines should be used where a module genuinely has meaningful states, not as a mandatory pattern inside every task. If they are treated as mandatory, they risk becoming a second control model layered on top of FreeRTOS instead of a tool used where they add clarity.

5. Features and abstractions that are not needed for the first working version, such as BLE or a custom allocator, should stay out of the core architecture until a concrete need appears. Otherwise the design grows in complexity before the core requirements are even stable.

## Course Alignment
We must also make sure the architecture supports the things the course actually examines:

- clear RTOS structure
- HAL boundaries for hardware-near code
- testable modules
- visible debugging and diagnostics
- explainable communication and synchronization
- clean documentation of architecture and data flow

The architecture should therefore avoid hidden global state and should make it possible to test logic outside the full hardware runtime whenever practical.

## Explicit Dependencies And Testability

One of the most important design choices in this proposal is that dependencies should be passed explicitly during initialization instead of being reached indirectly through one large shared application object.

That matters for two reasons.

First, it gives stronger boundaries. A module can only use the things it has actually been given. If the Sensor module is initialized with config, datastore, and a status signal path, then it cannot accidentally start reading WiFi internals, UI state, or unrelated module data because it never receives access to them in the first place.

Second, it makes test harnesses much easier to build. The same module that is initialized with real dependencies in the running system can be initialized with fakes, stubs, or mocks in tests. That means important logic can be tested in isolation instead of needing one large integrated runtime just to exercise a small piece of behavior.

In other words, explicit dependencies help both runtime architecture and testing architecture at the same time.

The important point is that dependencies are passed explicitly at init time. That gives stronger boundaries by construction. A module cannot accidentally read or write unrelated state later if it never received access to that state in the first place.

Example startup shape:

```c
void app_main(void)
{
    Config config = {0};
    StorageBackend storage = {0};
    DataStore dataStore = {0};
    EventGroup systemEvents = event_group_create();
    Queue statusQueue = queue_create();
    HealthRegistry health = {0};

    // Shared services are initialized explicitly at startup.
    // They are long-lived, but not collected into one giant App object.
    storage_init(&storage, &config);
    cfg_init(&config, &storage);
    ds_init(&dataStore, &config, &storage);
    health_init(&health);

    // Each module only receives the dependencies it actually needs.
    // That means a module cannot accidentally reach into unrelated state.
    wifi_init(&config, &systemEvents, &statusQueue);
    sensor_init(&config, &dataStore, &statusQueue);
    energy_fetcher_init(&config, &dataStore, &systemEvents, &statusQueue);
    display_init(&config, &dataStore);
    uart_diag_init(&config, &dataStore, &statusQueue);
    supervisor_init(&config, &health, &systemEvents, &statusQueue);

    // After init, tasks are started and app_main can return.
    wifi_start_task();
    sensor_start_task();
    energy_fetcher_start_task();
    display_start_task();
    uart_diag_start_task();
    supervisor_start_task();
}
```

The same explicit dependency model also makes it much easier to build test harnesses:

```c
void test_sensor_writes_valid_sample_to_datastore(void)
{
    Config testConfig = {0};
    FakeDataStore fakeDataStore = {0};
    FakeQueue fakeStatusQueue = {0};

    sensor_init(&testConfig, &fakeDataStore, &fakeStatusQueue);

    fake_bme280_set_temperature(112); // 11.2 C

    sensor_run_once();

    assert(fakeDataStore.latestTemp == 112);
}
```

Here the important point is not just that the Sensor module is testable with explicit dependencies. The more important point is that a large top-level application object is unnecessary in this design.

The system can be initialized directly in `app_main()`. After that, the runtime lives in tasks and shared services, not in one umbrella object. That means the application object is not needed to keep the system alive, not needed to make modules callable, and not needed to make tests possible.

All the same setup work still has to happen somewhere: queues must be created, storage must be initialized, and each module must receive the collaborators it actually depends on. A large application object does not remove that work. It only hides it behind one pointer.

Because of that, the large application object adds complexity without solving a real problem. It mainly creates a convenient place for state and dependencies to accumulate, and over time that makes shortcuts easier: both in runtime code and in tests.

With explicit initialization and explicit dependency passing, those shortcuts are removed by construction. A module only receives what it actually needs, and nothing more. That gives stronger boundaries, clearer ownership, and a design that stays closer to the real implementation steps instead of wrapping them in an unnecessary top-level container.

## Scope
This proposal is intentionally focused on the first working architecture that satisfies the core course goals.

That means:

- BLE is out of scope for the initial implementation
- a custom allocator is out of scope unless a concrete memory-management problem appears
- fine-grained per-module recovery is out of scope; the initial recovery policy is full system restart on critical unrecoverable fault

This is not because those things are impossible or uninteresting, but because they add complexity without helping us pass the course first.

## Architecture In One Paragraph
Startup should initialize the shared services and RTOS primitives directly in `app_main()`: shared queues and event groups, storage backend, configuration, datastore, and health registry. Then the real modules and tasks are initialized and started. After that, runtime ownership lives inside the modules/tasks themselves. Sensor and remote-data modules produce data. A dedicated `dataStore` module owns stored values, history, cache, and query access. UI and UART do not own the source of truth for application data; they read from `dataStore` and own only their own presentation or parser state. RTOS queues and event groups are used mainly for signaling and coordination.

## Platform Responsibilities
To keep the architecture concrete, we should be explicit about what comes from FreeRTOS, what comes from ESP-IDF, and what we must design ourselves.

### What we use from FreeRTOS
FreeRTOS should provide:

- tasks
- scheduling
- queues
- semaphores and mutexes
- event groups
- timers where needed

So FreeRTOS is our runtime and synchronization layer. We should use it for task execution and coordination, not to hide unclear ownership behind more tasks and signaling.

### What we use from ESP-IDF
ESP-IDF should provide:

- logging
- UART, I2C, and SPI drivers
- WiFi and network stack
- NVS
- watchdog APIs
- system diagnostics and reset information

So ESP-IDF is our platform and driver layer. It gives us the hardware-near services we need on ESP32.

### What we design ourselves
Our architecture should define:

- module boundaries
- HAL boundaries
- ownership of runtime state
- configuration and persistence policy
- `dataStore`
- storage abstraction
- query APIs for latest values and history
- UI data access model
- health policy and recovery policy

We should only build the abstractions we actually need.

## Core Design Decisions
### 1. Startup is direct and explicit

Startup should initialize the shared services and RTOS primitives directly rather than building one umbrella application object that everything later depends on.

That means initializing, in order:

- shared queues and event groups
- storage backend
- configuration
- datastore
- health registry
- modules that depend on those shared services
- tasks

Then `app_main()` can return. The ongoing system behavior lives in the tasks and shared services, not in a central application object.

### 2. Producer modules own production, `dataStore` owns stored data

This is the most important architectural decision.

The sensor module owns:

- the physical sensor
- sensor read timing
- calibration logic
- low-level read state

The remote-data side owns:

- connection state
- fetch timing
- parser state
- retry/backoff logic

The `dataStore` module owns:

- latest stored values
- historical series
- cached reduced series for graphs
- optionally precomputed moving averages or similar derived data

That gives the system a clear source of truth for data access and a cleaner testing story, because producers, storage, and consumers can be tested through narrow interfaces instead of depending on one large shared application context.

### 3. RTOS queues and event groups are for signaling

- "new data available"
- "WiFi connected/disconnected"
- "energy plan fetch done"
- "status changed"
- "refresh UI"

They are not the main storage model for long-lived or historical data.

### 4. UI owns presentation state, not application data

The display/UI task should own:

- current screen/view
- touch/navigation state
- rendering state
- temporary frame-local values

When UI paints a frame, it should read what it needs from `dataStore`. If it needs graph data, it should ask for that view explicitly. After rendering, it can discard temporary references. That keeps UI state small and avoids turning the display task into another data owner.

### 5. Health should be based on progress, not loop frequency

Each critical module should define:

- what meaningful progress means
- how often that progress is expected
- what timeout is acceptable

The supervisor task should own:

- checking those deadlines
- fault logging
- escalation/recovery policy
- watchdog feeding when the system is healthy

The watchdog is the fallback reset mechanism. The supervisor is the software policy layer.

### 6. HAL is required and should be test-friendly

The course explicitly expects HAL usage for hardware-near parts, so this should be part of the architecture from the start rather than treated as a late week 9 cleanup.

That means:

- sensor logic should depend on a sensor HAL, not directly on low-level I2C calls
- display logic should depend on a display HAL, not directly on low-level SPI transactions
- storage or communication logic should expose narrow interfaces instead of leaking driver details upward

This matters because it makes the code easier to explain in the course context and easier to test with stubs or mocks.

## Instance Lifetime And Boundaries

Before defining the modules, it is important to define what kind of thing a module actually is in this architecture.

Some modules are shared services. That means there should be one long-lived instance for the whole running system, and multiple tasks may use that same instance through a controlled API. These are not "global state containers" in the broad sense; they are narrowly defined shared services with clear responsibility.

Some modules are task-local. That means their runtime state belongs only to the task that owns them. Other tasks may interact with them indirectly through events, queues, or service APIs, but they should not read or mutate that task-local state directly.

Some data is only temporary. That means it exists only while a function runs or while one frame/query is being processed. Temporary data should stay local and should not silently grow into shared runtime state.

For this proposal, the important split is:

- one shared instance: `Config`, `DataStore`, `StorageBackend`, `HealthRegistry`, and shared RTOS primitives
- task-local runtime state: Sensor, WiFi, EnergyPlanFetcher, Display, UART Diagnostics, and Supervisor state
- temporary data: frame-local render data, query scratch buffers, and short-lived processing state

## Suggested Modules

### Sensor

The Sensor module owns the physical sensor and the logic required to read and validate it. It should not become a data warehouse for the rest of the system; it produces measurements and hands them off to `DataStore`.

Responsibility:

- read BME280
- apply validation/calibration
- produce new sensor samples
- notify that new data is available

Owns:

- sensor state
- read cadence
- sensor hardware logic
- sensor health state

Depends on:

- sensor HAL

Writes to:

- `dataStore`

Signals through:

- status queue and/or event group

Persists:

- normally nothing directly
- historical sensor data is owned by `dataStore`

### WiFi

The WiFi module owns connectivity only. Its job is to get the device on the network, detect disconnects, and publish connection status. It should not own fetched application data.

Responsibility:

- connect/reconnect WiFi
- maintain connection status
- notify connection changes

Owns:

- WiFi state
- reconnect timing
- internal FSM if needed
- WiFi health state

Depends on:

- WiFi/communication HAL or driver wrapper

This module owns connectivity. It does not own remote application data or fetched values.

Persists:

- nothing directly except through configuration if credentials/settings are updated

### EnergyPlanFetcher

The EnergyPlanFetcher module owns the logic for talking to the remote backend and translating the response into application data. It depends on connectivity, but it does not own connectivity itself and it does not own long-term storage of fetched data.

Responsibility:

- fetch energy-plan data
- parse responses
- write remote data into `dataStore`
- notify that new remote data is available

Owns:

- fetch timing
- parser state
- retry/backoff logic
- fetcher health state

Depends on:

- transport/client abstraction
- WiFi connectivity status

In practice this module should run on top of ESP-IDF networking facilities rather than reimplement TCP or HTTP itself.

Writes to:

- `dataStore`

Signals through:

- status queue and/or event group

Persists:

- nothing directly
- fetched remote data is stored through `dataStore`

This module fetches and parses remote data. It does not own long-term storage of that data.

### DataStore

`DataStore` is a shared service, not a task-local worker. It is the source of truth for stored application data: latest values, recent history, queryable views, and optionally cached derived data.

Responsibility:

- store latest values
- store history
- serve latest-value reads
- serve historical queries
- optionally cache reduced/derived series

Owns:

- latest sensor values
- latest remote values
- recent history in RAM
- long-term history access
- read/write buffering or snapshot logic

This module is the main source of truth for data access.

Persists:

- historical data if long-term retention is enabled
- cached derived series only if justified by cost and usage

Uses:

- `StorageBackend` for long-term reads/writes

### Display

The Display module owns presentation and interaction state. It should ask `DataStore` for the data it needs and then render it, rather than owning the underlying application data itself.

Responsibility:

- handle UI state
- request current or historical data from `dataStore`
- render the requested view

Owns:

- screen state
- frame/render state
- interaction state

Depends on:

- display HAL

Does not own:

- long-term application data

Persists:

- small logical UI state only, such as current view or navigation position, if that improves reboot recovery and testing

### UART Diagnostics

UART Diagnostics is an interactive inspection and command interface. It may read data from `DataStore` and print status, but it is not the owner of logs, sensor data, or remote data.

Responsibility:

- parse commands
- print latest values, diagnostics, and status
- request historical or latest data from `dataStore`

Owns:

- parser state
- UART interaction state

Depends on:

- UART abstraction or driver wrapper

Persists:

- normally nothing

This module is the interactive diagnostics interface. Normal runtime logging should use the ESP-IDF logging system.

### Config

`Config` is a shared service for configuration values and persistence policy. It should own settings and limits that other modules depend on, rather than letting those values be scattered across the system.

Responsibility:

- own runtime configuration
- load and save persistent configuration
- define per-module persistence policy
- define retention and budget limits where needed

Owns:

- device configuration
- persistence settings
- per-module persistence budgets

Persists:

- WiFi/backend settings if needed
- location if needed
- other small configuration values

Uses:

- `StorageBackend`

### StorageBackend

`StorageBackend` is a low-level persistence service. It owns how persistent reads and writes happen, but it does not own the meaning of the data being stored. That meaning stays with `Config`, `DataStore`, or another owning module above it.

Responsibility:

- provide low-level persistent read/write services
- abstract NVS, SD, or other persistent media

Owns:

- storage access logic
- no application-level meaning of the data

It should also serialize persistent access internally so that modules do not perform uncontrolled concurrent reads and writes against NVS or SD storage.

### Supervisor

The Supervisor module owns health evaluation and restart policy. It monitors the system and decides when the watchdog may be fed and when the system must be restarted.

Responsibility:

- monitor health registry
- check deadlines
- log faults
- decide recovery
- feed watchdog

Owns:

- health policy
- escalation rules

## Data Model

### Latest values

Latest values should be stored in `dataStore` as stable snapshots. For small latest-value data, a double-buffered or versioned snapshot model is appropriate.

That means:

- one internal buffer is safe for readers
- another internal buffer is used for updates
- when an update is complete, `dataStore` swaps which snapshot is active

This avoids partial reads and avoids forcing every consumer to keep its own copy.

### Recent history

Recent history should be stored in fixed-size RAM structures, typically ring buffers.

For example:

- sensor history at 5 minute resolution
- remote-data history at a cadence appropriate to the backend data

This keeps RAM bounded and predictable.

### Long-term history

Long-term history should be treated separately from latest-value snapshots.

For large retained datasets:

- keep only what is useful or possible in RAM
- store larger history in persistent storage
- load/query only the needed time range

This means the architecture should not assume that large historical datasets are fully resident in RAM at all times.

### Derived data

Derived data such as moving averages can be handled in stages:

- simple version: compute on demand
- if expensive and frequently requested: cache in `dataStore`

That keeps the design flexible without prematurely creating a separate analytics subsystem.

## Persistence Policy

Persistence should be intentional and configured per module.

That means:

- each module should have a defined persistence budget and purpose
- not every module should persist state
- config owns configuration persistence
- `dataStore` owns historical persistence
- UI may persist a small logical state checkpoint

The purpose is to avoid uncontrolled writes and to make persistence part of the architecture rather than an ad hoc side effect of implementation.

## How UI Should Query Data

The UI should not ask for "all raw data". It should ask for a view.

For example, UI should be able to ask for:

- latest temperature
- latest status
- last 7 days of temperature at display resolution
- last 50 days moving average at display resolution

That means the query API should be something like:

```c
ds_get_latest_temperature(...)
ds_get_series(range, maxPoints, ...)
ds_get_moving_average(range, maxPoints, ...)
```

The important point is that the UI tells the datastore:

- what time range it wants
- what maximum point count it can actually display

Then `dataStore` returns:

- a reduced or aggregated series in the right resolution

This is much more scalable than returning all raw data. If a graph is only 500 pixels wide, it should not receive millions of raw samples. It should receive data reduced to what the graph can actually render.

## Synchronization Strategy

### For data access

The preferred model is:

- producers write into `dataStore`
- consumers read from `dataStore`
- `dataStore` handles synchronization internally

For latest values, use stable snapshot access.

For larger history, use query APIs and bounded retrieval.

### For task coordination

Use RTOS primitives for:

- status changes
- refresh notifications
- connection state
- command and control signals

In other words:

- queues and event groups for coordination
- datastore for data

This separation also helps testing because queue-driven coordination and data storage can be validated independently.

## Testability Requirements

Each module should be designed so that important logic can be tested without needing the whole firmware stack to run.

That means:

- no hidden dependence on one large global mutable object
- narrow interfaces
- explicit dependencies passed during initialization
- HAL boundaries around hardware access
- pure or mostly pure logic for parsing, validation, reduction, and query handling where possible

Examples of things that should be easy to test:

- sensor data validation
- remote-data response parsing
- datastore query logic
- moving average or downsampling logic
- watchdog deadline evaluation
- UART command parsing

## Health Design

Each critical module should register one health entry:

```c
struct HealthEntry {
    const char *name;
    Tick lastProgress;
    Tick maxSilence;
    bool enabled;
};
```

The module defines:

- what meaningful progress means
- how often it should happen

The supervisor checks:

- whether progress is overdue

If all critical modules are healthy:

- supervisor feeds watchdog

If not:

- supervisor logs and applies recovery policy

For the first implementation, that recovery policy is full system restart rather than fine-grained per-module recovery.

## Final Direction

This proposal is intentionally narrower than a pattern-first architecture. It does not try to solve everything through one shared application object, generic mediator language, or mandatory per-task state machines. Instead, it tries to define the actual system we need to build:

- direct and explicit startup in `app_main()`
- modules that own their own runtime concerns
- a `dataStore` that owns stored values, history, and query access
- RTOS primitives used for signaling and coordination
- a supervisor/watchdog design based on progress and deadlines

That gives us a design we can explain, test, and start implementing without leaving the hardest ownership and data questions unresolved.
