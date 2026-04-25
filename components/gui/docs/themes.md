# Adding New GUI Themes

This guide explains how to add a new GUI theme in the redmole GUI component.

Scope:
- GUI only (LVGL view/theme integration)
- No non-GUI module setup

## Theme Architecture (Quick Map)

Theme behavior is driven by these files:

- `components/gui/include/gui_types.h`: theme enum (`gui_view_theme_t`) and appearance model fields.
- `components/gui/src/view/gui_theme_defs.h`: theme descriptor (`gui_theme_def_t`) and `GUI_THEME_COUNT`.
- `components/gui/src/view/gui_theme_defs.c`: canonical theme table, optional assets/fonts declarations, and dropdown option generation.
- `components/gui/src/view/gui_view.c`: runtime application logic and night-variant resolution.
- `components/gui/src/view/panels/gui_view_settings_panel.c`: settings panel dropdown and switches.
- `components/gui/src/module/gui_module_events.c`: maps settings dropdown selection to enum values.

## Checklist: Add a New Selectable Theme

Follow these in order.

1. Add a new enum value in `gui_view_theme_t`.

File:
- `components/gui/include/gui_types.h`

Example pattern:

```c
typedef enum {
    GUI_VIEW_THEME_LIGHT = 0,
    GUI_VIEW_THEME_DARK,
    GUI_VIEW_THEME_HELLO_KITTY,
    GUI_VIEW_THEME_TERMINAL,
    GUI_VIEW_THEME_HELLO_KITTY_NIGHT,
    GUI_VIEW_THEME_NEW_THEME,
} gui_view_theme_t;
```

2. Increment `GUI_THEME_COUNT`.

File:
- `components/gui/src/view/gui_theme_defs.h`

```c
#define GUI_THEME_COUNT 6
```

3. Add a matching table entry at the correct index in `gui_themes[]`.

File:
- `components/gui/src/view/gui_theme_defs.c`

Rules:
- Table order must match enum order exactly.
- Set `is_user_selectable = true` for normal user-facing themes.
- Provide values for all color/font fields.
- Set `has_night_variant` and `night_variant` consistently.

Suggested workflow:
- Duplicate the closest existing theme (for example Light or Dark).
- Rename `display_name`.
- Adjust fonts/colors/background image pointer.

4. If the theme should appear in Settings dropdown, keep it user-selectable.

The dropdown options string is generated from entries where:

- `is_user_selectable == true`

Function:
- `gui_theme_build_dropdown_string(...)` in `components/gui/src/view/gui_theme_defs.c`

5. Update event mapping in settings handler.

File:
- `components/gui/src/module/gui_module_events.c`

Current behavior maps dropdown index manually (hardcoded if/else).
If you add another selectable theme, update this mapping so the new option selects the intended enum.

## Night Variant Pattern (Optional)

Use this when a base theme should have a night version.

1. Add an internal night enum value in `gui_view_theme_t`.
2. Add another table entry in `gui_themes[]` with:
   - `is_user_selectable = false`
   - usually `has_night_variant = false`
3. In the base theme entry set:
   - `has_night_variant = true`
   - `night_variant = <night enum value>`

Runtime behavior:
- `gui_view_effective_theme(...)` in `components/gui/src/view/gui_view.c` switches to `night_variant` when the toggle is enabled and the selected base theme supports it.

Settings behavior:
- Night toggle is disabled for themes without `has_night_variant`.

## Background Images and Fonts (Optional)

If your theme uses custom image/font assets:

1. Add generated LVGL C asset files under:
- `components/gui/src/view/assets/...`

2. Add source files to GUI component registration.

File:
- `components/gui/CMakeLists.txt`

3. Declare assets in `gui_theme_defs.c` and reference them from your theme entry.

Examples already present:

```c
LV_IMG_DECLARE(hk_bg);
LV_IMG_DECLARE(hk_bg_night);
LV_IMG_DECLARE(terminal_bg);
LV_FONT_DECLARE(hellokitty18);
LV_FONT_DECLARE(terminal24);
```

4. Set:
- `background_image = &your_bg` (or `NULL` if no image)
- `body_font` and `emphasis_font` to matching font objects.

## Design Guidance

Use these conventions to keep themes coherent and readable:

- Keep strong contrast between text and all major backgrounds.
- Verify title, subtitle, muted, and value text colors independently.
- Ensure active/inactive nav button states are visually distinct.
- Keep primary and secondary action buttons clearly different.
- Preserve legibility in dropdowns, keyboard keys, and Wi-Fi dialog rows.
- If you use a vivid accent, keep `accent_soft_color` less intense for secondary emphasis.
- Use a consistent visual story across panel/card/item surfaces.

Practical checks:
- Card border still visible on content background.
- Selected dropdown option text remains readable.
- Slider knob remains visible against track and panel.
- Wi-Fi connected/known/selected states are each distinguishable.

## Validation Checklist

After adding a theme, verify:

1. Build succeeds (`idf.py build`).
2. Theme appears in the dropdown (if selectable).
3. Selecting it updates all panels and dialogs, not just one area.
4. Background image toggle behaves correctly:
   - Theme with image: image can be shown/hidden.
   - Theme without image: no crash and graceful fallback.
5. Night toggle behavior is correct:
   - Enabled only when the selected theme supports `has_night_variant`.
6. No enum/table mismatch:
   - `gui_view_theme_t` order == `gui_themes[]` order.
   - `GUI_THEME_COUNT` equals actual table length.

## Common Pitfalls

- Forgetting to increment `GUI_THEME_COUNT` after adding enum/table entries.
- Adding enum values but not adding a table entry at the same index.
- Marking internal variants as selectable by mistake.
- Adding selectable themes without updating dropdown index mapping in `gui_module_events.c`.
- Adding image/font files but forgetting to register them in `components/gui/CMakeLists.txt`.

## Recommended Future Hardening

The current dropdown selection handler uses fixed index-to-enum logic.
A safer long-term approach is to derive the selected enum directly from the selectable theme list built from `gui_themes[]`, so adding themes does not require manual index branching updates.
