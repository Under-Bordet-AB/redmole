# GUI Source Layout

This folder contains the internal implementation of the GUI component.

The public integration surface stays outside this folder in `components/gui/include/gui_module.h` and `components/gui/include/gui_types.h`.

## Folder Roles

- `module/`: owns GUI lifecycle, LVGL callback routing, refresh timing, and platform/display bring-up
- `control/`: owns GUI state transitions and builds `gui_view_model_t` from GUI-managed state
- `view/`: owns the shell layout, navigation, shared view helpers, and panel-specific UI under `view/panels/`
- `gui_defs.h`: shared internal definitions imported across the internal layers

## Ownership Rules

- Put public API changes in `../include/`, not here
- Put LVGL event dispatch and module runtime wiring in `module/`
- Put state mutation and model-building logic in `control/`
- Put shared layout orchestration in `view/`
- Put panel-local widgets and apply functions in `view/panels/`
- Avoid reaching across layers when a narrower helper header can express the dependency

## Current File Map

- `module/gui_module.c`: thin public-facing module orchestrator inside the internal source tree
- `module/gui_module_internal.h`: shared runtime types and module-private prototypes
- `module/gui_module_runtime.c`: runtime access and model apply helper
- `module/gui_module_events.c`: LVGL event handlers and binding dispatch
- `module/gui_module_platform.c`: display bring-up, refresh timer, and backlight handling
- `control/gui_control.h`: control-layer types and internal control API
- `control/gui_control.c`: control initialization and panel selection
- `control/gui_control_internal.h`: shared internal control helpers
- `control/gui_control_internal.c`: helper implementations for Wi-Fi and energy-plan state
- `control/gui_control_wifi.c`: Wi-Fi-specific control actions
- `control/gui_control_model.c`: view-model construction
- `view/gui_view.h`: view handle storage and top-level internal view API
- `view/gui_view.c`: shell layout and high-level apply flow
- `view/gui_view_common.h`: shared view helpers
- `view/gui_view_common.c`: shared view helper implementations
- `view/panels/gui_view_bme280_panel.c`: BME280 panel UI
- `view/panels/gui_view_energy_panel.c`: energy plan panel UI
- `view/panels/gui_view_settings_panel.c`: settings panel UI and dialogs

## Notes

- The BME280 panel is currently intentionally zeroed rather than populated with mock sensor values.
- Formatting changes should follow the repository rules in `AGENTS.md`; do not run repo-wide formatting.