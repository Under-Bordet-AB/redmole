# How To: Create a New Theme

This is the fast-start version of theme authoring for contributors who are new to the GUI component.

Scope:
- Add a new selectable GUI theme
- Keep enum, theme table, assets, and settings dropdown in sync
- Point to the deeper theme guide when you need more design detail

Use this guide when you want the implementation path first. Use `components/gui/docs/themes.md` when you want the full reference.

## 1. Quick Map

The theme system is controlled by a small set of files.

- `components/gui/include/gui_types.h`: theme enum `gui_view_theme_t`
- `components/gui/src/view/gui_theme_defs.h`: `GUI_THEME_COUNT` and `gui_theme_def_t`
- `components/gui/src/view/gui_theme_defs.c`: the canonical `gui_themes[]` table, display names, and optional asset declarations
- `components/gui/Kconfig`: optional build toggles for branded themes
- `components/gui/CMakeLists.txt`: conditional asset compilation
- `components/gui/src/view/gui_view.c`: runtime theme resolution and application
- `components/gui/src/view/panels/gui_view_settings_panel.c`: theme dropdown and theme-related controls
- `components/gui/src/gui.c`: routes dropdown and switch events into state updates

If those files disagree, the theme will usually build incorrectly or appear wrong in the settings screen.

## 2. Pick the Right Template Before Editing

Do this first. It keeps the amount of change small.

Use one of these starting points:

- Palette-only theme: copy Light or Dark from `components/gui/src/view/gui_theme_defs.c`
- Theme with background image and custom fonts: copy Hello Kitty, Terminal, Death Note, or SpongeBob from `components/gui/src/view/gui_theme_defs.c`
- Theme with a night variant: copy the Hello Kitty plus Hello Kitty Night pair

Recommendation:

- Start from Light if you want a bright, neutral base.
- Start from Dark if you want a low-light base without custom assets.
- Start from Terminal or Death Note if typography is part of the theme identity.
- Start from Hello Kitty if you need both a base theme and a hidden night variant.

## 3. Add the Theme in Order

Follow these steps in order. They are intentionally strict because enum and table mismatches are easy to create.

### A. Add the enum value

File:
- `components/gui/include/gui_types.h`

Add the new `gui_view_theme_t` value in the correct order.

Rules:

- The enum order matters.
- Internal variants such as a hidden night theme still need enum entries.
- Keep the new value aligned with the order you plan to use in `gui_themes[]`.

Example pattern:

```c
typedef enum {
    GUI_VIEW_THEME_LIGHT = 0,
    GUI_VIEW_THEME_DARK,
    GUI_VIEW_THEME_HELLO_KITTY,
    GUI_VIEW_THEME_TERMINAL,
    GUI_VIEW_THEME_HELLO_KITTY_NIGHT,
    GUI_VIEW_THEME_DEATH_NOTE,
    GUI_VIEW_THEME_SPONGEBOB,
    GUI_VIEW_THEME_NEW_THEME,
} gui_view_theme_t;
```

### B. Increment the theme count

File:
- `components/gui/src/view/gui_theme_defs.h`

Update `GUI_THEME_COUNT` to match the new number of table entries.

Current rule in that file:

- update the count whenever you add new enum values to `gui_view_theme_t`

Do not leave this for later. A stale count can compile and still break behavior.

### C. Add the theme table entry at the matching index

File:
- `components/gui/src/view/gui_theme_defs.c`

This file is the canonical theme registry.

Rules:

- `gui_themes[]` must stay in exactly the same order as `gui_view_theme_t`
- `display_name` is what the settings dropdown shows
- `is_user_selectable = true` makes the theme appear in the dropdown
- `has_night_variant` and `night_variant` must be internally consistent
- Every color and font field should be filled deliberately, not left half-copied from another theme

Recommended workflow:

1. Duplicate the closest existing theme block.
2. Rename `display_name`.
3. Replace colors, fonts, and optional background image.
4. Read through the whole struct once before building.

Important detail:

The dropdown options are generated from the theme table, not from a separate hard-coded list. The display name and selectable flag here directly control what users see.

### D. Add optional assets only if the theme needs them

If the theme is palette-only, skip this section.

If the theme needs fonts or a background image:

1. Add the generated asset files under `components/gui/src/view/assets/`
2. Declare them in `components/gui/src/view/gui_theme_defs.c` with `LV_IMG_DECLARE(...)` or `LV_FONT_DECLARE(...)`
3. Reference them in the theme entry
4. Register the asset source files in `components/gui/CMakeLists.txt`

Current examples already in the codebase:

- `hk_bg`, `hk_bg_night`
- `terminal_bg`
- `deathnote_bg`
- `spongebob_bg`
- matching custom fonts for those themes

## 4. Add the Build Toggle for Optional Branded Themes

Files:
- `components/gui/Kconfig`
- `components/gui/CMakeLists.txt`

The current project keeps Light and Dark as always-built themes. Branded themes are gated behind Kconfig options.

Add a new Kconfig option if your theme is optional.

Pattern to copy from `components/gui/Kconfig`:

- `CONFIG_REDMOLE_GUI_THEME_HELLO_KITTY`
- `CONFIG_REDMOLE_GUI_THEME_TERMINAL`
- `CONFIG_REDMOLE_GUI_THEME_DEATH_NOTE`
- `CONFIG_REDMOLE_GUI_THEME_SPONGEBOB`

Then use the same symbol in two places:

- around the asset declarations and theme entries in `components/gui/src/view/gui_theme_defs.c`
- around the asset source registration in `components/gui/CMakeLists.txt`

If the theme includes an internal night variant, do not add a second user-facing Kconfig option just for the night theme. Gate the base and night entries together.

## 5. Confirm the Runtime Selection Path

The settings screen and runtime apply flow already support selectable themes. Still, verify the full path after you add one more entry.

Main files:

- `components/gui/src/view/panels/gui_view_settings_panel.c`
- `components/gui/src/gui.c`
- `components/gui/src/view/gui_view.c`

Current flow:

1. The settings dropdown shows user-selectable themes from `gui_themes[]`
2. The user changes the dropdown
3. `gui_handle_theme_event(...)` in `components/gui/src/gui.c` reads the selected row
4. `gui_theme_dropdown_index_to_theme(...)` resolves the row back to a `gui_view_theme_t`
5. GUI state updates and rerender happens
6. `gui_view_effective_theme(...)` resolves the final active theme, including night-variant substitution when enabled

What to verify:

- The new theme appears exactly once in the dropdown if it is selectable
- The dropdown index mapping still resolves correctly
- Night mode stays disabled for themes without `has_night_variant`
- Background image toggle behaves gracefully if the theme has no image

## 6. Fast Checklist

Use this checklist while editing.

### Required edits

- `components/gui/include/gui_types.h`
- `components/gui/src/view/gui_theme_defs.h`
- `components/gui/src/view/gui_theme_defs.c`

### Optional edits for branded themes

- `components/gui/Kconfig`
- `components/gui/CMakeLists.txt`
- `components/gui/src/view/assets/...`

### Runtime verification points

- `components/gui/src/view/gui_view.c`
- `components/gui/src/view/panels/gui_view_settings_panel.c`
- `components/gui/src/gui.c`

## 7. Common Pitfalls

- Adding the enum value but forgetting the matching `gui_themes[]` entry.
- Updating `gui_themes[]` but forgetting to increment `GUI_THEME_COUNT`.
- Putting the new table entry at the wrong index relative to the enum.
- Marking an internal night variant as `is_user_selectable = true`.
- Adding assets but forgetting the `LV_IMG_DECLARE(...)` or `LV_FONT_DECLARE(...)` line.
- Adding asset files but forgetting to register them in `components/gui/CMakeLists.txt`.
- Creating a branded theme without a matching `CONFIG_REDMOLE_GUI_THEME_*` symbol.
- Copying another theme and missing one or two color fields, which leaves the UI with a visual mismatch that is hard to spot until runtime.

## 8. Validation Checklist

After adding the theme, verify:

1. The build succeeds.
2. The theme appears in the settings dropdown if it is selectable.
3. Selecting the theme updates all panels, not only the currently visible one.
4. The title text, subtitle text, card borders, buttons, dropdowns, and sliders are all readable.
5. The Wi-Fi dialog remains legible and state colors still make sense.
6. The background image toggle works correctly for image-backed themes.
7. The night toggle only enables for themes that actually define `has_night_variant`.
8. Rebooting preserves the selected theme if persistence is enabled through the existing appearance flow in `main/src/app_gui_bindings.c`.

## 9. When To Read the Full Theme Guide

Open `components/gui/docs/themes.md` if you need any of these:

- detailed design guidance for contrast and readability
- a deeper explanation of the night-variant pattern
- more notes on asset registration and optional theme packaging
- a more complete list of theme-specific failure modes

Use this how-to for the first pass. Use the full guide before you call the theme finished.

## 10. Related Documents

- `components/gui/docs/themes.md`: the full theme reference
- `components/gui/docs/gui_project_map.md`: GUI architecture and where the theme layer fits
- `components/gui/README.md`: component overview and doc entry points
