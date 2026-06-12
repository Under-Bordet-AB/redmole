# How To: Create a New Panel or Modal Dialog

This guide is for contributors who need to add a new screen area to the GUI and are not yet familiar with how the view tree is organized.

Scope:
- Add a new top-level panel, or
- Add a modal dialog owned by an existing panel
- Keep the current `event -> state -> render` split intact

Use this guide when you need to decide where a new UI surface belongs and what files have to change.

## 1. Decide First: Panel or Dialog

Use this rule before touching code.

- Panel: use when the UI is a top-level destination that should occupy the main content area and optionally have its own sidebar navigation button.
- Modal dialog: use when the UI is temporary, blocks or overlays part of the current panel, and should be shown or hidden without changing the active top-level panel.

Current examples:

- Top-level panels: BME280, Energy Plan, Forecast, Settings
- Modal dialogs: the Wi-Fi network dialog and the Wi-Fi password dialog owned by the settings panel

The important boundary is ownership:

- Panels are part of the main content switching flow in `components/gui/src/view/gui_view.c`.
- Dialogs are owned by the panel that creates them, even if the coordinator exposes convenience wrappers through `gui_screen.c`.

## 2. Quick Map

These files are the main path for adding a new panel or dialog:

- `components/gui/include/gui_types.h`: top-level panel enum and shared view-model types
- `components/gui/src/gui.c`: coordinator event routing and top-level panel selection
- `components/gui/src/gui_state.c`: GUI-owned state transitions and model building
- `components/gui/src/gui_screen.c`: thin coordinator-facing wrapper around the view layer
- `components/gui/src/view/gui_view.h`: persistent LVGL object pointers stored by the screen
- `components/gui/src/view/gui_view.c`: view initialization, panel visibility switching, and apply flow
- `components/gui/src/view/panels/*.c`: panel-local widget creation and apply helpers
- `components/gui/src/view/panels/gui_view_settings_panel.c`: concrete dialog ownership example

## 3. How a New Panel Fits

The current panel flow is simple and explicit.

1. `gui_panel_id_t` in `components/gui/include/gui_types.h` defines the top-level panel identity.
2. The coordinator stores the active panel in GUI state.
3. `components/gui/src/view/gui_view.h` keeps one root LVGL container pointer per panel.
4. `components/gui/src/view/gui_view.c` creates each panel during view initialization.
5. `gui_view_apply(...)` shows one panel and hides the others.
6. The active panel's `gui_view_apply_*_panel(...)` function renders its current model data.

You can see the current apply switch in `gui_view_apply(...)` and the existing panel implementation pattern in files such as `components/gui/src/view/panels/gui_view_bme280_panel.c`.

## 4. How a Modal Dialog Fits

The current dialog pattern is panel-owned rather than globally registered.

The Wi-Fi dialogs are the reference implementation:

- Widgets live on `gui_view_t` in `components/gui/src/view/gui_view.h`
- Creation and show or hide logic live in `components/gui/src/view/panels/gui_view_settings_panel.c`
- Thin wrapper functions exist in `components/gui/src/view/gui_view.c`
- `components/gui/src/gui_screen.c` exposes those wrappers to the coordinator
- `components/gui/src/gui.c` decides when to show or hide them based on events

That is the right pattern for new dialogs too:

- keep dialog creation near the panel that owns it
- only add `gui_screen_*` wrappers if the coordinator needs to control it
- do not turn every dialog into a global top-level surface unless it truly spans the whole screen architecture

## 5. Recipe: Add a New Top-Level Panel

Follow these steps in order.

### A. Add the panel identity

File:
- `components/gui/include/gui_types.h`

Add a new `gui_panel_id_t` entry.

Example pattern:

```c
typedef enum {
    GUI_PANEL_BME280 = 0,
    GUI_PANEL_ENERGY_PLAN,
    GUI_PANEL_FORECAST,
    GUI_PANEL_SETTINGS,
    GUI_PANEL_NEW_PANEL,
} gui_panel_id_t;
```

If the new panel needs new data, extend the shared model in the same file so the view layer receives all required state through `gui_view_model_t`.

### B. Add panel-local source files

Files:
- `components/gui/src/view/panels/gui_view_new_panel.h`
- `components/gui/src/view/panels/gui_view_new_panel.c`

Follow the existing panel pattern:

- `gui_view_init_<panel>_panel(...)` creates the widgets and stores any needed pointers on `gui_view_t`
- `gui_view_apply_<panel>_panel(...)` updates the widgets from the current model snapshot

The BME280 panel is the smallest example to copy because it shows a direct `init` plus `apply` split without dialog complexity.

### C. Extend the view handle

File:
- `components/gui/src/view/gui_view.h`

Add persistent widget pointers for the new panel.

Typical additions:

- a root container such as `lv_obj_t *new_panel;`
- any child labels, charts, or buttons that must be updated during `apply`
- any cached last-applied model fragments if the panel needs its own redraw optimization

This step is required because the view tree is persistent across rerenders.

### D. Create and apply the panel in the view layer

File:
- `components/gui/src/view/gui_view.c`

Update the view in three places:

1. Include the new panel header near the other panel includes.
2. Create the panel during `gui_view_init(...)` beside the other panel init calls.
3. Extend `gui_view_apply_header(...)` and `gui_view_apply(...)` so the new panel is shown when active and hidden otherwise.

If the panel should be navigable from the sidebar, also add a sidebar button and include it in the nav styling and event mapping.

Current control points to update:

- `gui_screen_target_to_panel(...)` in `components/gui/src/gui.c`
- the nav button pointers in `components/gui/src/view/gui_view.h`
- the nav button styling inside `gui_view_apply(...)`

### E. Update coordinator and state routing

Files:
- `components/gui/src/gui.c`
- `components/gui/src/gui_state.c`

Check that:

- the new panel can become the active panel
- any panel-specific events route to the right callbacks or GUI state transitions
- the model builder populates the new panel data if it needs new fields

If the panel is view-only and uses existing model state, this step may be small. If the panel introduces new GUI-owned state, do not skip `gui_state.c`.

### F. Register the new panel source file

File:
- `components/gui/CMakeLists.txt`

Add the new panel source file to `gui_srcs` so it builds.

## 6. Recipe: Add a New Modal Dialog

Follow this path when the UI belongs to an existing panel and should appear as an overlay.

### A. Keep ownership with the parent panel

Choose the panel that owns the dialog.

Example:

- the settings panel owns the Wi-Fi dialogs

That means the creation and apply behavior should usually live in that panel's source file rather than in a new global dialog module.

### B. Add dialog widget pointers to the view

File:
- `components/gui/src/view/gui_view.h`

Add the root dialog container pointer and any child widgets that need updates or event handling.

Use the Wi-Fi dialog fields as the model:

- dialog container
- title and subtitle labels
- action buttons
- optional shared scrim

### C. Create the dialog in the owning panel file

File:
- `components/gui/src/view/panels/gui_view_settings_panel.c`

For a new dialog owned by another panel, follow the same pattern there:

1. Build the LVGL objects during that panel's init flow.
2. Hide the dialog by default.
3. Store all important pointers on `gui_view_t`.
4. Add helper functions that show, hide, and lay out the dialog.

The current reference helpers are:

- `gui_view_hide_wifi_dialogs_impl(...)`
- `gui_view_show_network_dialog_impl(...)`
- `gui_view_show_password_dialog_impl(...)`

### D. Add coordinator-facing wrappers only if needed

Files:
- `components/gui/src/view/gui_view.c`
- `components/gui/src/gui_screen.c`

If the coordinator needs to open or close the dialog, add thin wrappers like the existing Wi-Fi dialog wrappers.

Keep these wrappers thin. They should forward to the panel-owned implementation rather than reimplementing UI logic.

### E. Wire dialog events through the coordinator

File:
- `components/gui/src/gui.c`

Use `gui_handle_*_event(...)` style handlers to convert dialog button presses or text input into:

- GUI-owned state changes, or
- binding callbacks into the app adapter

The password dialog is the concrete reference because it shows a modal input flow that updates GUI state, opens and closes overlays, and forwards connection requests.

## 7. Panel vs Dialog Checklist

Use this checklist before you start implementing.

Choose a panel if:

- the surface needs a dedicated root container in the main content area
- users should navigate to it directly from the sidebar or top-level flow
- it deserves a distinct `gui_panel_id_t`

Choose a dialog if:

- the surface only makes sense while another panel is active
- it should overlay existing content temporarily
- it can be owned and updated locally by one panel implementation

## 8. Common Pitfalls

- Adding a new panel enum but forgetting to create or hide its LVGL container in `gui_view.c`
- Adding panel widgets without storing them on `gui_view_t`, which makes later apply updates awkward or impossible
- Adding a dialog as a top-level panel even though it only belongs to one parent panel
- Putting dialog construction in `gui.c` instead of keeping it in the view or panel layer
- Forgetting to register a new panel source file in `components/gui/CMakeLists.txt`
- Updating visibility logic in one place but not in `gui_view_apply_header(...)` and `gui_view_apply(...)`
- Adding a new navigation button without extending `gui_screen_target_to_panel(...)`

## 9. Validation Checklist

After adding a panel or dialog, verify:

1. The build succeeds.
2. The new surface appears in the correct place.
3. Panel switching hides the other top-level panels correctly.
4. Dialog show or hide behavior does not leave stale overlays behind.
5. The new widgets survive rerendering and theme changes.
6. Any new event handlers trigger exactly once per interaction.
7. If the panel adds navigation, the button styling and active state update correctly.

## 10. Related Documents

- `components/gui/docs/gui_project_map.md`: where the panel and coordinator layers fit
- `components/gui/docs/howto/create-settings-card.md`: how to add settings UI and route callbacks or NVS
- `components/gui/docs/themes.md`: theme behavior that affects new panels and dialogs
- `components/gui/README.md`: GUI component overview