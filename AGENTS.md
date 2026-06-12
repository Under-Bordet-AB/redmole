# AGENTS

## Clang-Format

This repository uses a root-level [`.clang-format`](./.clang-format) file for C formatting rules.

Current style highlights:
- Pointer style is configured by `.clang-format`
- Function and control-statement braces use attached style: `if (...) {`
- Only one empty line in a row is allowed

## What To Format

Formatting is intentionally opt-in per file.

Only files listed in [`tools/clang-format-files.txt`](./tools/clang-format-files.txt) are formatted by the helper script. This prevents accidental repo-wide formatting changes.

To add a file:
1. Add its repo-relative path to `tools/clang-format-files.txt`
2. Run the formatter script

Example list entry:

```text
components/example/include/example.h
components/example/src/example.c
```

## How To Run

Use the cross-platform Python helper:

```bash
python tools/format_files.py
```

Requirements:
- `python` must be available on `PATH`
- `clang-format` must be available on `PATH`

## Contributor Rule

Do not run `clang-format` across the whole repository. Users will tell you to first add files to  `tools/clang-format-files.txt`.
