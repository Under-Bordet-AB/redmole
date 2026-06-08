# Environment Measurements Component

`environment_measurements` owns board-local environment readings.

It is the responsibility boundary for environment sensing: sensor discovery,
sensor polling, simulation fallback, latest-sample storage, and the public API
used by GUI, UART, and future consumers.

## Design Direction

The component exposes a C API, but uses a small C++ sensor interface internally.

```text
main / GUI / UART
        |
        v
environment_measurements C API
        |
        v
EnvironmentMeasurements controller + task
        |
        +--> fixed EnvironmentSensor* array
        |       +--> Bme280Sensor at 0x76
        |       +--> Bme280Sensor at 0x77
        +--> SimulatedBme280Sensor
```

This is intentional. Application code should depend on the environment
responsibility area, not on a concrete BME280 driver or a separate sensor data
store.

## Why C++ Virtual Sensors

The internal sensor interface lets the controller treat real and simulated
sensors the same way:

```cpp
class EnvironmentSensor {
public:
    virtual esp_err_t init() = 0;
    virtual bool probe() = 0;
    virtual esp_err_t read(environment_measurement_sample_t& out) = 0;
};
```

What this gives us:

- one polling and publishing path for physical and simulated readings
- a fixed collection of physical sensors that the controller can iterate
- runtime discovery of the BME280 addresses that are actually connected
- plug/unplug fallback by switching active sensor state
- a stable public C API while internal sensor implementations change
- a clear path to more sensor types or locations later

The cost is a small vtable per concrete sensor type and one virtual call when
reading or probing a sensor. That is acceptable here because BME280 access is an
I2C transaction; bus latency dominates the cost by far. The extra abstraction is
also useful for the school-project goal: it demonstrates a deliberate runtime
polymorphism tradeoff instead of using compile-time switches or duplicated C
paths.

Raw C would also work, for example with function pointers and context structs.
For this branch, C++ virtual functions make the ownership model easier to read:
the module owns concrete sensor objects, and the controller talks to them
through one common interface.

## Memory And Lifetime

Sensor objects are fixed module-owned storage. They are created during startup
and stay alive for the lifetime of the module.

Rules:

- no runtime `new` or `delete` for sensor objects
- no runtime growth of the sensor list
- physical sensors are exposed to the controller through a fixed
  `EnvironmentSensor*` array
- startup initializes the supported physical sensors and the simulator
- runtime plug/unplug changes active/inactive state only

This keeps the C++ design deterministic enough for embedded use while avoiding
heap fragmentation from dynamic sensor allocation.

## I2C Ownership

This component does not own the I2C bus.

`board_i2c` owns the shared board bus because touch, IO expansion, and the
environment sensor all use the same hardware bus. Environment sensors depend on
`board_i2c` for register access and probing.

Future bus locking, duplicate device-handle policy, or request priority should
be solved in `board_i2c`, not inside this component.

## Runtime Behavior

At initialization:

1. initialize the shared `board_i2c` bus
2. initialize fixed BME280 sensor objects for addresses `0x76` and `0x77`
3. initialize the simulated inside sensor
4. select the physical BME280 if detected, otherwise use simulation

At runtime:

- readings are published on `CONFIG_REDMOLE_ENVIRONMENT_READING_INTERVAL_SEC`
- hardware presence is checked on `CONFIG_REDMOLE_ENVIRONMENT_DETECT_INTERVAL_SEC`
- if the physical sensor disappears, the module logs fallback to simulation
- if the physical sensor appears again, the module switches back to hardware

The separate detection interval lets demos react quickly without forcing the
release reading cadence to stay fast.

## Public API

The public header is `include/environment_measurements.h`.

Current API:

- `environment_measurements_init()`
- `environment_measurements_start()`
- `environment_measurements_deinit()`
- `environment_measurements_get_latest()`
- `environment_measurements_is_fresh()`
- `environment_measurements_get_update_count()`

Callers copy out one coherent latest snapshot with
`environment_measurements_get_latest()`. They should not call separate
temperature, humidity, and pressure getters, because separate calls could mix
values from different samples.

## Current Layout

- `include/environment_measurements.h`: public C API and sample type
- `src/environment_measurements.cpp`: controller, task, active sensor state, latest snapshot
- `src/environment_sensor.hpp`: private C++ sensor interface
- `src/bme280/`: private BME280 implementation
- `src/sim/`: private simulated BME280 fallback

See `docs/JJ/ENVIRONMENT_MODULE_CPP_DESIGN.md` for the longer design notes and
tradeoff discussion.
