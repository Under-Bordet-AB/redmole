# How To: Create a Settings Card and Wire a Callback or NVS to It

This guide is for contributors who are new to the redmole GUI codebase and need to add one more settings control.

Scope:
- Add a new settings card or setting row in the GUI
- Route user changes to the right owner
- Optionally persist the value through NVS
- Keep the existing GUI and app adapter split intact

This is an end-to-end recipe, not a generic LVGL tutorial.

## 1. Start With the Ownership Decision

Before writing code, decide who owns the setting.

Use this rule:

- GUI-owned setting: the value is part of presentation state and the GUI can expose setter/getter APIs for it. Current example: theme, background image, night mode, brightness.
- App-owned setting: the value belongs to another module or service, and the GUI should only surface a control plus a callback. Current example: Wi-Fi scan/connect/disconnect requests.

Why this matters:

- GUI-owned settings usually add fields to shared GUI types and can be persisted by the app adapter through `gui_get_*` and `gui_set_*` APIs.
- App-owned settings usually need a new callback in `gui_module_bindings_t` and should not quietly become GUI-internal state just because the control lives on the settings screen.

## 2. Quick Map

These files are the main path for a new setting:

- `components/gui/src/view/panels/gui_view_settings_panel.c`: builds the settings cards and LVGL controls.
- `components/gui/src/gui.c`: listens to LVGL events and turns them into state updates or callback requests.
- `components/gui/include/gui_module.h`: public GUI API and callback binding surface.
- `components/gui/include/gui_types.h`: shared GUI state structs such as `gui_appearance_settings_t` and `gui_view_model_t`.
- `components/gui/src/gui_state.c`: GUI-owned state transitions.
- `main/src/app_gui_bindings.c`: bridges GUI events to app services and persists selected GUI state through NVS.
- `main/main.c`: loads saved startup appearance before `gui_init()`.

Existing reference flows worth copying:

- GUI-owned persisted settings: `app_gui_bindings_load_saved_appearance(...)`, `cache_current_appearance(...)`, `save_appearance_if_changed(...)`, and `app_gui_bindings_sync(...)` in `main/src/app_gui_bindings.c`.
- App-owned callback flow: the Wi-Fi callbacks registered in `app_gui_bindings_init(...)` and requested from `components/gui/src/gui.c`.

## 3. Reuse the Existing Settings Card Pattern

The settings screen already has helper functions for the card layout. Start there instead of inventing another style.

Card helpers in `components/gui/src/view/panels/gui_view_settings_panel.c`:

- `gui_view_create_settings_card(...)`: section-level card with title and subtitle
- `gui_view_create_setting_item_card(...)`: one settings item with title and subtitle
- `gui_view_create_setting_item_card_shell(...)`: empty shell when you need a custom layout inside the card

Recommended approach:

1. Find the existing settings card that is closest to what you need.
2. Duplicate the layout pattern, not just the LVGL widget type.
3. Keep the new control inside the existing settings panel rendering flow so theme application and rerender behavior stay consistent.

## 4. Recommended Implementation Order

Follow these steps in order.

### A. Add the shared model field if the GUI should store the value

File:
- `components/gui/include/gui_types.h`

Examples already present:

- `gui_appearance_settings_t`
- `gui_wifi_settings_t`

If your setting is GUI-owned, add a field to the right shared struct instead of creating a disconnected local variable inside the panel code.

Example directions:

- Add a `bool` for a toggle
- Add a small enum for a dropdown selection
- Add an integer field for a slider-backed value

Then add or extend the corresponding public API in:

- `components/gui/include/gui_module.h`
- `components/gui/src/gui.c`

Use the existing appearance APIs as the model:

- `gui_set_appearance_settings(...)`
- `gui_get_appearance_settings(...)`
- `gui_set_brightness(...)`
- `gui_get_brightness(...)`

### B. Add the control to the settings panel

File:
- `components/gui/src/view/panels/gui_view_settings_panel.c`

Typical work:

1. Create or extend a card in the settings panel.
2. Create the LVGL control.
3. Store the widget pointer on the `gui_view_t` screen/view struct if the control needs later synchronization.
4. Update the panel apply logic so the control reflects the current model state during rerender.

The existing appearance controls already demonstrate the required synchronization behavior:

- `gui_view_appearance_settings_changed(...)` checks whether the rendered control state still matches the current appearance model.
- The settings panel then updates dropdowns and switches only when needed.

That is the right pattern for a new persistent control too. Do not rely on a one-time widget setup if the screen can rerender.

### C. Route the event in the GUI coordinator

File:
- `components/gui/src/gui.c`

The coordinator converts LVGL events into either GUI state changes or app callbacks.

Relevant existing handlers:

- `gui_handle_theme_event(...)`
- `gui_handle_brightness_event(...)`
- `gui_handle_wifi_action_event(...)`
- `gui_handle_password_textarea_event(...)`

Choose one of these two routes:

- GUI-owned route: update GUI state through a `gui_state_*` function or existing setter/getter path, then rerender.
- App-owned route: call a binding callback such as the Wi-Fi request functions do.

If you need a new callback:

1. Add the callback to `gui_module_bindings_t` in `components/gui/include/gui_module.h`.
2. Add a small request helper in `components/gui/src/gui.c` if that keeps the event code clean.
3. Trigger it from the relevant event handler.
4. Register the implementation in `app_gui_bindings_init(...)`.

The Wi-Fi pattern is the concrete reference:

- GUI side request helpers: `gui_request_wifi_scan(...)`, `gui_request_wifi_connect(...)`, `gui_request_wifi_disconnect(...)`
- App registration: `app_gui_bindings_init(...)`
- App implementation: `on_wifi_scan_requested(...)`, `on_wifi_connect_requested(...)`, and related functions in `main/src/app_gui_bindings.c`

### D. Persist GUI-owned state through the app adapter

File:
- `main/src/app_gui_bindings.c`

This is the current persistence boundary. Follow the same sequence the appearance settings already use.

Startup load path:

1. Define an NVS key near the top of the file.
2. Read the saved value in `app_gui_bindings_load_saved_appearance(...)` or in a sibling load helper if the setting should not live in `gui_init_config_t`.
3. Apply the value before or immediately after GUI initialization, depending on when the setting must exist for the first render.

Runtime save path:

1. Cache the last saved value in `s_bindings`.
2. Compare the live GUI value with the cached value inside `save_appearance_if_changed(...)` or a sibling save helper.
3. Write through `rm_nvs_set_u8(...)` or `rm_nvs_set_str(...)` only when the value changed.
4. Call that helper from `app_gui_bindings_sync(...)`.

Current appearance persistence flow:

- `app_gui_bindings_load_saved_appearance(...)` reads `gui_theme`, `gui_bg`, `gui_night`, and `gui_bright`
- `cache_current_appearance(...)` seeds the last-saved cache
- `save_appearance_if_changed(...)` writes only on change
- `app_gui_bindings_sync(...)` calls the save helper every loop iteration

That is the main persistence pattern newcomers should copy.

## 5. A Concrete Example: Add an "Eco Mode" Toggle

Use this as a mental template even if the real setting has a different name.

### Option 1: Eco Mode is GUI-owned and persisted

Use this route if the toggle only affects GUI presentation or a GUI-managed mode.

Implementation sketch:

1. Add `bool eco_mode_enabled` to the relevant GUI settings struct in `components/gui/include/gui_types.h`.
2. Extend the GUI public getter and setter surface if needed.
3. Add the switch widget to `components/gui/src/view/panels/gui_view_settings_panel.c`.
4. Add event handling in `components/gui/src/gui.c` so a toggle updates GUI state and triggers `gui_render_runtime(...)`.
5. Add `GUI_NVS_KEY_ECO_MODE` in `main/src/app_gui_bindings.c`.
6. Load the saved value during startup.
7. Cache and persist it in the sync loop.

### Option 2: Eco Mode belongs to another service

Use this route if the toggle should start or stop behavior outside the GUI.

Implementation sketch:

1. Add a callback such as `on_eco_mode_toggled` to `gui_module_bindings_t`.
2. Add the switch widget to `components/gui/src/view/panels/gui_view_settings_panel.c`.
3. In `components/gui/src/gui.c`, call the new binding when the user toggles it.
4. Implement the callback in `main/src/app_gui_bindings.c`.
5. From there, call the owning service and optionally persist the app-owned setting through NVS in that adapter layer.

Do not hide an app service decision in GUI code just because the UI control is local to the settings panel.

## 6. File-by-File Checklist

Use this as your working checklist.

### GUI layout and rendering

- `components/gui/src/view/panels/gui_view_settings_panel.c`
- `components/gui/src/view/gui_view.c`
- `components/gui/src/gui_screen.c`

Check that:

- The widget is created in the right card or section.
- The control pointer is stored if it needs later updates.
- The rendered widget state is synchronized from the current model.
- The control still looks correct across theme changes.

### GUI state and public API

- `components/gui/include/gui_types.h`
- `components/gui/include/gui_module.h`
- `components/gui/src/gui.c`
- `components/gui/src/gui_state.c`

Check that:

- The setting lives in one clear owner struct.
- The event handler updates the correct state.
- Public getters and setters exist if the app adapter needs them.
- Rerendering happens after the state change.

### App adapter and persistence

- `main/src/app_gui_bindings.c`
- `main/main.c`

Check that:

- Startup load happens early enough for the first visible render.
- Runtime persistence writes only on change.
- NVS keys are named consistently with the existing `gui_*` keys.
- App-owned callbacks stay in the adapter instead of leaking into the GUI component.

## 7. Common Pitfalls

- Adding the widget but not storing or synchronizing its state, so rerenders reset it visually.
- Writing directly to another service from the settings panel instead of routing through `gui.c` and the binding layer.
- Persisting every loop iteration without comparing against the last saved value first.
- Storing an app-owned setting in a GUI-only struct because it was faster during the first implementation pass.
- Updating the LVGL widget directly in one event path but forgetting to update the underlying GUI model, which causes the next render to undo the user action.
- Extending `gui_module_bindings_t` without wiring the new callback in `app_gui_bindings_init(...)`.

## 8. Validation Checklist

After you add the setting, verify:

1. The project builds.
2. The new card renders correctly on the settings screen.
3. The control survives rerendering and panel switches.
4. The value updates the right owner layer.
5. If persisted, the value survives reboot.
6. If callback-driven, the callback fires exactly once per user action.
7. The new control remains readable in Light mode, Dark mode, and at least one optional branded theme.

## 9. When To Split the Work

If the feature is larger than one control, split it into two steps:

1. Land the GUI-owned model and UI first.
2. Land callback integration or NVS persistence second.

That makes it easier to validate the ownership decision before the adapter code grows.

## 10. Related Documents

Use these when you need deeper context:

- `components/gui/docs/gui_project_map.md`: where the GUI layer stops and the app adapter begins
- `components/gui/docs/themes.md`: theme-specific authoring details
- `components/gui/README.md`: high-level GUI component overview
