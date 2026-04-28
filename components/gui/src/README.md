# GUI Source Layout

This folder contains the internal implementation of the GUI component.

The public integration surface stays outside this folder in `components/gui/include/gui_module.h` and `components/gui/include/gui_types.h`.

## Folder Roles

- `gui.c`: owns lifecycle, bindings, and the `event -> state -> render` coordinator loop
- `gui_state.*`: owns GUI state transitions and builds `gui_view_model_t` from GUI-managed state
- `gui_screen.*`: owns the screen-facing interface used by the coordinator
- `gui_platform.*`: owns platform/display bring-up, refresh timing, and backlight handling
- `view/`: owns the shell layout, shared view helpers, and panel-specific UI under `view/panels/`
- `gui_defs.h`: shared internal definitions imported across the internal layers

## Ownership Rules

- Put public API changes in `../include/`, not here
- Put orchestration and callback-to-action routing in `gui.c`
- Put state mutation and model-building logic in `gui_state.*`
- Put the coordinator-facing render boundary in `gui_screen.*`
- Put shared LVGL layout orchestration in `view/`
- Put panel-local widgets and apply functions in `view/panels/`
- Avoid reaching across layers when a narrower helper header can express the dependency

## Current File Map

- `gui.c`: public-facing GUI orchestration and event routing
- `gui_internal.h`: shared runtime types and coordinator-private declarations
- `gui_state.h` / `gui_state.c`: GUI state transitions and screen-model building
- `gui_screen.h` / `gui_screen.c`: coordinator-facing screen wrapper around LVGL view code
- `gui_platform.h` / `gui_platform.c`: display bring-up, refresh timer, and backlight handling
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
