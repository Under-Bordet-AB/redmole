# RedMole Unity Tests

This directory is a separate ESP-IDF firmware project used to build and run
RedMole tests on the ESP32-S3.

The normal firmware and test firmware share production components, but they
have separate entry points, configuration files, build directories, and output
binaries.

## Index

- [Start Here: Add Tests For A Module](#start-here-add-tests-for-a-module)
- [Running Existing Tests](#running-existing-tests)
- [Current Repository Layout](#current-repository-layout)
- [How The Existing Test Project Was Created](#how-the-existing-test-project-was-created)
- [How The Build Works](#how-the-build-works)
- [Unity Fundamentals](#unity-fundamentals)
- [Worked Example: SimProducer Returns Three Measurements](#worked-example-simproducer-returns-three-measurements)
- [Test Types And Suggested Tags](#test-types-and-suggested-tags)
- [C And C++ Tests](#c-and-c-tests)
- [IntelliSense And Compilation Databases](#intellisense-and-compilation-databases)
- [Automatic Runner Versus Interactive Menu](#automatic-runner-versus-interactive-menu)
- [Troubleshooting](#troubleshooting)

```text
Normal firmware: repository root -> build/
Test firmware:   test/           -> test/build/
```

The test firmware uses ESP-IDF's Unity integration. It automatically runs every
registered `TEST_CASE` after boot and prints a pass/fail summary.

## Start Here: Add Tests For A Module

Use this section when the global test project already exists and you only want
to add tests for a component.

Assume the component is named `example`.

### 1. Create The Component Test Directory

Create:

```text
components/example/test/
```

Tests live beside the component they verify. They are not added to the normal
firmware build.

### 2. Add The Local Test CMake File

Create `components/example/test/CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS "test_example.c"
    REQUIRES unity example
)
```

This declares a test component that:

- compiles `test_example.c`
- uses Unity
- links the production `example` component

For a C++ test that needs private headers from the component's `src/`
directory, use:

```cmake
idf_component_register(
    SRCS "test_example.cpp"
    INCLUDE_DIRS "../src"
    REQUIRES unity example
)
```

`../src` is resolved relative to the component's `test/` directory.

### 3. Write A Test

Minimal C or C++ test:

```c
#include "unity.h"

TEST_CASE("example returns its expected value", "[example][unit]")
{
    int actual = example_get_value();

    TEST_ASSERT_EQUAL_INT(42, actual);
}
```

The first `TEST_CASE` argument is a human-readable behavior description. The
second argument contains searchable tags.

Use Arrange, Act, Assert:

```c
TEST_CASE("example accepts a valid input", "[example][unit]")
{
    // Arrange: create inputs and prepare state.
    example_t example = {};

    // Act: perform the behavior being tested.
    esp_err_t result = example_init(&example);

    // Assert: verify externally visible results.
    TEST_ASSERT_EQUAL(ESP_OK, result);
}
```

### 4. Enable The Module's Tests

Add the component name to `TEST_COMPONENTS` in `test/CMakeLists.txt`:

```cmake
set(TEST_COMPONENTS
    "environment_measurements"
    "example"
    CACHE STRING
    "Project components whose Unity tests are included"
)
```

Only listed components have their local `test/` directories included in the
test firmware.

`TEST_COMPONENTS` is stored in CMake's build cache. After changing this list,
recreate the test build cache so the new default is applied:

```powershell
cd test
idf.py fullclean
idf.py reconfigure
```

### 5. Build And Run

From the repository root:

```powershell
cd test
idf.py build
idf.py -p COM4 flash monitor
```

Replace `COM4` with the board's serial port. On Linux, a port may look like
`/dev/ttyUSB0` or `/dev/ttyACM0`.

Every registered test runs automatically after the test firmware boots.
Exit the ESP-IDF monitor with `Ctrl+]`.

## Running Existing Tests

The first build on a new checkout requires selecting the target:

```powershell
cd test
idf.py set-target esp32s3
```

Then build, flash, and monitor:

```powershell
idf.py build
idf.py -p COM4 flash monitor
```

After changing only test or production source code, run the same commands
again. Ninja rebuilds only affected files.

A successful build proves that the test firmware compiled and linked. It does
not prove that tests passed. Tests run only after the firmware is flashed and
booted on the ESP32-S3.

Typical successful output:

```text
Running sim producer returns three measurements...
components/environment_measurements/test/test_sim_producer.cpp:3:
sim producer returns three measurements:PASS

-----------------------
1 Tests 0 Failures 0 Ignored
OK
```

## Current Repository Layout

```text
redmole/
|-- CMakeLists.txt
|-- build/                              Normal firmware build output
|-- components/
|   `-- environment_measurements/
|       |-- CMakeLists.txt              Production component definition
|       |-- src/                        Production implementation
|       `-- test/
|           |-- CMakeLists.txt          Environment test component definition
|           `-- test_sim_producer.cpp   Environment tests
`-- test/
    |-- CMakeLists.txt                  Test firmware project definition
    |-- sdkconfig                       Test firmware configuration
    |-- build/                          Test firmware build output
    |-- README.md                       This guide
    `-- main/
        |-- CMakeLists.txt              Test runner component definition
        `-- test_main.c                 Test firmware entry point
```

## How The Existing Test Project Was Created

This section reproduces the complete setup from a repository with no test
firmware to a working Unity test.

Most contributors do not need to repeat these steps. Use
[Start Here: Add Tests For A Module](#start-here-add-tests-for-a-module) when
the global test project already exists.

### Step 1: Create The Global Test Project Directories

From the repository root:

```bash
mkdir -p test/main
```

### Step 2: Create The Test Project CMake File

Create `test/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.22)

set(EXTRA_COMPONENT_DIRS "../components")

set(TEST_COMPONENTS
    "environment_measurements"
    CACHE STRING
    "Project components whose Unity tests are included"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

idf_build_set_property(MINIMAL_BUILD ON)

project(redmole_tests)
```

Line meanings:

- `cmake_minimum_required(...)` states the oldest supported CMake version.
- `EXTRA_COMPONENT_DIRS` lets the separate test project discover RedMole's
  production components.
- `TEST_COMPONENTS` selects component-local test directories to include.
- `include(...)` loads ESP-IDF's CMake system and functions.
- `MINIMAL_BUILD` avoids building unrelated components.
- `project(...)` declares and configures the test firmware project.

### Step 3: Create The Test Runner Component

Create `test/main/CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS "test_main.c"
    PRIV_REQUIRES unity
    WHOLE_ARCHIVE
)
```

This declares the test firmware's `main` component:

- `SRCS` selects the source file to compile.
- `PRIV_REQUIRES unity` gives the runner access to Unity.
- `WHOLE_ARCHIVE` prevents registration-related code from being discarded by
  the linker because no ordinary function directly references it.

### Step 4: Create The Automatic Test Runner

Create `test/main/test_main.c`:

```c
#include "unity.h"

void app_main(void)
{
    UNITY_BEGIN();
    unity_run_all_tests();
    UNITY_END();
}
```

At runtime:

```text
ESP-IDF boots
-> ESP-IDF calls app_main()
-> Unity resets its counters
-> Unity runs every registered TEST_CASE
-> Unity prints the result summary
```

`app_main()` is the ESP-IDF application entry point. ESP-IDF owns the lower
level startup code because it must initialize the chip and FreeRTOS first.

### Step 5: Create A Component-Local Test Directory

Create:

```text
components/environment_measurements/test/
```

Create `components/environment_measurements/test/CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS "test_sim_producer.cpp"
    INCLUDE_DIRS "../src"
    REQUIRES unity environment_measurements
)
```

The test uses C++ because `SimProducer` is a C++ class. The private `src/`
include directory allows this focused module test to include internal headers.

### Step 6: Add A Focused Unit Test

Create `components/environment_measurements/test/test_sim_producer.cpp`:

```cpp
#include "unity.h"

#include "sim/sim_producer.hpp"

using redmole::environment::MeasurementBatch;
using redmole::environment::MeasurementChannel;
using redmole::environment::sim::SimProducer;

TEST_CASE("sim producer returns three measurements", "[environment_measurements][unit][sim]") {
    SimProducer producer(MeasurementChannel::IndoorAmbientTemperature,
                         MeasurementChannel::IndoorRelativeHumidity,
                         MeasurementChannel::IndoorPressure);
    MeasurementBatch batch = {};

    TEST_ASSERT_EQUAL(ESP_OK, producer.init());
    TEST_ASSERT_EQUAL(ESP_OK, producer.read(batch));
    TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(batch.count));
}
```

This focused unit test proves both the test path and one production behavior:

```text
test source compiles
-> TEST_CASE registers
-> test firmware links
-> board boots
-> Unity runs the test
-> simulated producer returns one complete three-value batch
```

### Step 7: Configure And Build The Test Project

From `test/`:

```powershell
idf.py set-target esp32s3
idf.py build
```

`set-target` creates the test project's generated configuration and build
files. The normal and test projects therefore have separate `sdkconfig` and
`build/` directories.

### Step 8: Flash And Run

```powershell
idf.py -p COM4 flash monitor
```

The automatic runner executes all configured tests after boot.

## How The Build Works

Running `idf.py build` starts several tools:

```text
idf.py
-> CMake reads CMakeLists.txt files and creates a build graph
-> Ninja executes the build graph
-> the C/C++ compiler creates object files
-> the linker creates an ELF executable
-> ESP-IDF creates a flashable binary
```

CMake commands such as `set`, `include`, and `project` are built into CMake.
ESP-IDF-specific commands such as `idf_component_register` are loaded by:

```cmake
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
```

CMake executes during configuration on the developer's computer. Test code
executes later on the ESP32-S3.

## Unity Fundamentals

### Test Registration

```c
TEST_CASE("descriptive behavior", "[module][type]")
{
}
```

ESP-IDF's `TEST_CASE` macro creates a test function and registers it so
`unity_run_all_tests()` can find it.

### Common Assertions

```c
TEST_ASSERT_TRUE(condition);
TEST_ASSERT_FALSE(condition);
TEST_ASSERT_NULL(pointer);
TEST_ASSERT_NOT_NULL(pointer);
TEST_ASSERT_EQUAL(expected, actual);
TEST_ASSERT_EQUAL_INT(expected, actual);
TEST_ASSERT_EQUAL_UINT32(expected, actual);
TEST_ASSERT_EQUAL_INT64(expected, actual);
TEST_ASSERT_EQUAL_STRING(expected, actual);
```

Use an assertion matching the compared type. Better type information produces
clearer failure output.

When an assertion fails, Unity stops the current test, records the failure, and
continues with later tests.

## Worked Example: SimProducer Returns Three Measurements

This example verifies one focused behavior:

> A successful simulated environment acquisition returns exactly three logical
> measurements.

It is a unit test because it exercises `SimProducer` directly without starting
the polling task, using the store, accessing I2C, or requiring a BME280.

In `components/environment_measurements/test/test_sim_producer.cpp`:

```cpp
#include "unity.h"

#include "sim/sim_producer.hpp"

using redmole::environment::MeasurementBatch;
using redmole::environment::MeasurementChannel;
using redmole::environment::sim::SimProducer;

TEST_CASE("sim producer returns three measurements", "[environment_measurements][unit][sim]")
{
    // Arrange: create a producer and caller-owned output batch.
    SimProducer producer(
        MeasurementChannel::IndoorAmbientTemperature,
        MeasurementChannel::IndoorRelativeHumidity,
        MeasurementChannel::IndoorPressure);
    MeasurementBatch batch = {};

    // Act and assert: initialization and reading must both succeed.
    TEST_ASSERT_EQUAL(ESP_OK, producer.init());
    TEST_ASSERT_EQUAL(ESP_OK, producer.read(batch));

    // Assert: one complete simulated acquisition contains three values.
    TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(batch.count));
}
```

Why each check exists:

- Checking `init()` distinguishes initialization failure from read failure.
- Checking `read()` prevents the test from trusting an output batch after an
  unsuccessful operation.
- Checking `batch.count` verifies the behavior named by the test.
- Constructing the producer inside the test gives every run fresh independent
  state.

Do not immediately add every possible check to this test. Separate tests should
verify channel identities, first-sample values, ramp progression, and reset
behavior. Small tests make failures easier to understand.

### Test Design Rules

- Test one behavior per `TEST_CASE`.
- Give tests behavior-focused names.
- Keep tests independent; one test must not require another to run first.
- Check return values before checking output produced by a function.
- Prefer deterministic inputs and outputs.
- Avoid real hardware in unit tests when a simulator or fake can answer the
  same software question.

## Test Types And Suggested Tags

### Unit Test

Tests one small class or function with minimal dependencies.

Example: call `SimProducer::read()` directly and verify it returns three
measurements.

```text
[environment_measurements][unit]
```

### Integration Test

Tests several real module parts together.

Example: connect a fake producer, `MeasurementsManager`, and
`MeasurementStore`, then verify publication.

```text
[environment_measurements][integration]
```

### Hardware Test

Requires physical hardware or a connected peripheral.

Example: read and verify a real BME280 over I2C.

```text
[environment_measurements][hardware]
```

Hardware tests should clearly document their required wiring and state.

## C And C++ Tests

Use `.c` for C APIs and `.cpp` for C++ classes.

A C++ test can call a public C API because the public header provides the
required `extern "C"` declarations:

```cpp
#include "environment_measurements.h"
```

Tests of private C++ module types may include internal headers when their local
test CMake file exposes `../src`.

Do not expose private headers through the production component's public
`INCLUDE_DIRS` only to make testing easier.

## IntelliSense And Compilation Databases

CMake generates a compilation database containing the exact compiler command
for every source file:

```text
build/compile_commands.json       Normal firmware
test/build/compile_commands.json  Test firmware
```

Unity test files exist only in the test compilation database. If VS Code uses
only the normal database, `TEST_CASE`, Unity headers, and private test include
paths may show incorrect squiggles even when the test build succeeds.

For Microsoft's C/C++ extension, configure both databases:

```json
"C_Cpp.default.compileCommands": [
    "${workspaceFolder}/build/compile_commands.json",
    "${workspaceFolder}/test/build/compile_commands.json"
]
```

The equivalent `compileCommands` property can also be placed in
`.vscode/c_cpp_properties.json`.

After changing the setting:

1. Open the VS Code command palette.
2. Run `C/C++: Reset IntelliSense Database`.
3. Reopen the test source file.

The `.vscode/` directory is currently ignored in this repository, so each
developer may need to configure this locally.

## Automatic Runner Versus Interactive Menu

This repository uses:

```c
unity_run_all_tests();
```

It runs every configured test automatically and requires no serial input.

Unity also provides:

```c
unity_run_menu();
```

The menu allows selecting tests interactively over the console, but it
busy-waits for input and can starve the FreeRTOS idle task. If a future test
firmware uses the interactive menu, disable automatic task-watchdog startup in
that test project's configuration and ensure the monitor uses the primary
console that Unity reads.

The automatic runner is preferred for repeatable tests and future scripting.

## Troubleshooting

### `TEST_CASE` Has IntelliSense Squiggles

Build the test project so `test/build/compile_commands.json` exists, configure
VS Code to use it, and reset the IntelliSense database.

### Test Source Is Not Compiled

Check:

- the filename is listed in the local test `CMakeLists.txt`
- the component is listed in `TEST_COMPONENTS`
- the file starts with `test` by convention
- CMake was reconfigured after adding new files

Run:

```powershell
cd test
idf.py reconfigure
idf.py build
```

### Test Builds But Does Not Run

Check that:

- it uses `TEST_CASE`, not an ordinary uncalled function
- the runner calls `unity_run_all_tests()`
- registration code was not discarded by the linker
- the updated test firmware was flashed to the board

### Private Header Cannot Be Found

Add the required internal directory to the local test component:

```cmake
INCLUDE_DIRS "../src"
```

Do not add broad include paths without understanding which private boundary the
test needs.

### Build Passes But Tests Fail

This is expected when the implementation does not meet the assertions.

```text
Build success = firmware compiled and linked.
Test success  = firmware ran and every assertion passed.
```

### Monitor Does Not Exit After Tests

`idf.py monitor` remains open after the automatic Unity summary. Exit with
`Ctrl+]`.

For CI that must return an automatic process exit code, add an ESP-IDF
`pytest-embedded` runner rather than parsing an indefinitely running monitor.

### Interactive Menu Prints Task-Watchdog Warnings

`unity_run_menu()` busy-waits for serial input. Prefer the automatic runner.
If the interactive menu is required, disable task-watchdog startup in the test
project only.
