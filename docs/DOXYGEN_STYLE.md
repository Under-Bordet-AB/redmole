# Doxygen Style Guide

This file is the ground truth for Doxygen comments in this repository.

The goal is simple and informative documentation of mainly .h files. Comments explain what the
type, function, or module means: purpose, ownership, lifecycle, units, side
effects, valid inputs, return behavior, and state meaning. Avoid restating the C
syntax.

Use this common Doxygen subset for project-owned code:

- `/** ... */` blocks before documented declarations.
- `@file` and `@brief` for header/module summaries.
- `@param` for function parameters.
- `@return` for non-`void` return values.
- `@note`, `@warning`, and `@see` when they add real caller value.
- `/*!< ... */` inline comments for struct and enum fields.

Use more advanced Doxygen features only when they add clear value.

## File Comments

Every project-owned header should start with a file block after the include
guard:

```c
/**
 * @file
 * @brief Short module summary.
 *
 * Optional short paragraph explaining the module boundary, ownership, or role
 * in the system.
 */
```

Use the file comment to describe the module, not to list every function.

Project-owned `.c` files may also have an `@file` block when the file owns a
subsystem, backend, state machine, or other non-obvious implementation boundary.
Do not add file blocks to tiny `.c` files only to satisfy a checklist.

## Function Comments

Use this shape for documented functions:

```c
/**
 * @brief One-sentence summary.
 *
 * Optional short detail paragraph for ownership, lifecycle, units, blocking
 * behavior, side effects, or important error cases.
 *
 * @param name Meaning, ownership, valid values, or NULL behavior.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
```

Rules:

- Keep `@brief` to one sentence.
- Add detail only when it helps the caller use the API correctly.
- Document each parameter with `@param`.
- Document return values with `@return` for non-`void` functions.
- For pointer parameters, state whether `NULL` is valid.
- For lifecycle APIs, state ordering expectations such as init-before-start.
- For repeated calls, state whether the function is idempotent when relevant.
- For blocking, task creation, storage, hardware access, or logging side effects,
  mention the behavior briefly.

## Structs, Enums, And Fields

Document public structs and important private structs with a short block:

```c
/**
 * @brief Latest local environmental sample.
 */
typedef struct {
    int64_t timestamp_ms;       /*!< Timestamp in milliseconds from esp_timer. */
    int32_t temperature_deci_c; /*!< Temperature in deci-degrees Celsius. */
} sensor_data_sample;
```

Rules:

- Document units on fields.
- Document ownership for pointers.
- Document whether fields are public state, private state, or caller-owned.
- Document file-scope `.c` structs when they represent module state, backend
  state, task context, cached data, protocol data, configuration, or hardware
  state.
- For private `.c` structs, focus on what the state represents. Do not document
  every obvious scalar field if the struct-level comment is enough.

For enums, document what the enum controls and add field comments when the names
are not already clear:

```c
/**
 * @brief Local sensor service lifecycle state.
 */
typedef enum {
    LOCAL_SENSOR_STOPPED, /*!< Polling task is not running. */
    LOCAL_SENSOR_RUNNING, /*!< Polling task is active. */
} local_sensor_state;
```

## What To Document

Document:

- Public headers in `include/`.
- Internal headers that define cross-file contracts.
- Public typedefs, structs, enums, macros, and functions.
- Important file-scope structs and enums in `.c` files.
- Important ownership, lifecycle, and threading assumptions.

Usually do not document:

- Private `static` helper functions in `.c` files unless the logic, side
  effects, or concurrency behavior is unusual.
- Tiny local structs used only inside one function when the fields are obvious.
- Obvious implementation details.
- Generated or vendored code.

## Tone

Good comments are direct:

```c
/**
 * @brief Copy the latest published local sensor sample.
 *
 * The returned sample can be valid but old. Consumers that require live data
 * should also call sensor_data_is_local_fresh().
 *
 * @param out Output sample, must not be NULL.
 * @return True when a valid sample has been published, false otherwise.
 */
```

Avoid comments that only repeat the symbol name:

```c
/**
 * @brief Gets the latest local sensor sample.
 */
```

## File Review Checklist

Use this section when checking whether a file is completely documented.

- The file is project-owned, not generated or vendored.
- Public headers have an `@file` block.
- File-level comments explain the module purpose and boundary.
- File-level comments describe code responsibility without naming personal
  authors.
- Public functions have an `@brief`.
- Public function parameters are documented with `@param`.
- Public non-`void` return values are documented with `@return`.
- Pointer parameters document ownership and `NULL` behavior.
- Lifecycle APIs document required ordering such as init/start/stop/deinit.
- Lifecycle APIs document repeated-call behavior when relevant.
- Public structs are documented.
- Public enums are documented.
- Public macros are documented when their purpose is not obvious.
- Struct fields have inline comments when their meaning is not obvious.
- Enum values have inline comments when their meaning is not obvious.
- Numeric values document units, ranges, or scaling where relevant.
- Pointers stored in structs document ownership.
- Important file-scope `.c` structs are documented when they carry module state.
- Important file-scope `.c` enums are documented when they carry module state.
- Backend state, task context, cached data, protocol data, configuration, and
  hardware state are documented where they appear.
- Private helper functions avoid Doxygen unless they have unusual logic, side
  effects, or concurrency behavior.
- Blocking behavior is documented where it affects callers or maintainers.
- Task creation is documented where it affects callers or maintainers.
- Hardware access is documented where it affects callers or maintainers.
- Storage side effects are documented where they affect callers or maintainers.
- Logging side effects are documented where they affect callers or maintainers.
- Threading assumptions are documented where they affect callers or maintainers.
- Related APIs use `@see` when discoverability benefits.
- Comments explain behavior and intent instead of repeating C syntax.
- Comments are concise enough to stay maintainable.

