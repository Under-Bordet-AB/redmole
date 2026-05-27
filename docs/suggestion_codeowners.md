# Suggestion: CODEOWNERS

## Summary

`CODEOWNERS` is a repository-level file that maps files or directories to the
people or teams responsible for reviewing and maintaining them.

It is not a replacement for Doxygen comments, Git history, or architecture
documentation. It answers a different question: who should be involved when this
area of the code changes?

## What CODEOWNERS Is For

CODEOWNERS is commonly used to:

- Define review responsibility for parts of the repository.
- Automatically request reviewers on pull requests.
- Make ownership visible without adding personal names to source files.
- Route changes to people familiar with a module, component, or subsystem.
- Keep source comments focused on code behavior instead of team process.

## What CODEOWNERS Is Not For

CODEOWNERS should not be used to:

- Explain how code works.
- Document API behavior.
- Track who originally wrote a file.
- Replace Git history.
- Replace Doxygen comments.

## Difference From Doxygen Ownership

Doxygen should document ownership as code responsibility.

Example:

```c
/**
 * @file
 * @brief Application service that polls the board-local environmental sensor.
 *
 * The service owns the BME280 HAL instance and the polling task.
 */
```

This explains what the code owns.

CODEOWNERS documents human or team responsibility.

Example:

```text
/components/bme280/ @sensor-team
/components/local_sensor_service/ @sensor-team
/components/gui/ @gui-team
/docs/ @maintainers
```

This explains who should review or maintain changes.

## Example CODEOWNERS File

A possible starting point for this repository could look like this:

```text
# Default maintainers for the repository.
* @maintainers

# Sensor stack.
/components/bme280/ @sensor-team
/components/sensor_data/ @sensor-team
/components/local_sensor_service/ @sensor-team

# GUI stack.
/components/gui/ @gui-team
/main/include/app_gui_bindings.h @gui-team
/main/src/app_gui_bindings.c @gui-team

# Infrastructure and shared services.
/components/board_i2c/ @platform-team
/components/system_services/ @platform-team
/components/nvs/ @platform-team

# Documentation.
/docs/ @maintainers
```

Team names are examples only. The actual names should match the repository
hosting platform and the team structure.

## Benefits

- Clearer pull request routing.
- Less need to add personal owner comments in source files.
- Easier onboarding for new contributors.
- Better separation between code documentation and maintenance process.
- Ownership can change without editing source files.

## Costs And Decisions

Before adopting CODEOWNERS, the team should decide:

- Which repository hosting platform rules apply.
- Whether owners are individuals, teams, or both.
- Whether every file needs an owner or only important areas.
- Whether ownership means required review or suggested review.
- Who maintains the CODEOWNERS file itself.

## Recommendation

Use Doxygen for code behavior and code responsibility.

Use Git for authorship history.

Use CODEOWNERS if the team wants explicit review or maintenance ownership by
path.

