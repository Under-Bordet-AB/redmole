# Redmole Theme Editor

Static GUI theme editor for `gui_theme_def_t` color tuning.

Open `index.html` directly in a browser. The app stores edits in browser `localStorage` per preset and exports a C initializer that can be pasted into `components/gui/src/view/gui_theme_defs.c`.

Files:

- `index.html`: app shell and preview markup
- `styles.css`: editor and Redmole GUI mock styling
- `app.js`: seeded themes, live color editing, import/export, local storage

No Node, npm, server, or firmware build is required.
