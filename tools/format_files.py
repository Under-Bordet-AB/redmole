#!/usr/bin/env python3

from pathlib import Path
import subprocess
import sys


def main() -> int:
    script_dir = Path(__file__).resolve().parent
    repo_root = script_dir.parent
    list_path = script_dir / "clang-format-files.txt"

    if not list_path.exists():
        print(f"Could not find file list: {list_path}", file=sys.stderr)
        return 1

    try:
        lines = list_path.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        print(f"Could not read file list: {exc}", file=sys.stderr)
        return 1

    files = []
    for line in lines:
        entry = line.strip()
        if (not entry) or entry.startswith("#"):
            continue
        files.append(entry)

    for relative_path in files:
        full_path = (repo_root / relative_path).resolve()
        if not full_path.exists():
            print(f"Listed file does not exist: {relative_path}", file=sys.stderr)
            return 1

        try:
            subprocess.run(["clang-format", "-i", str(full_path)], check=True)
        except FileNotFoundError:
            print("clang-format was not found in PATH.", file=sys.stderr)
            return 1
        except subprocess.CalledProcessError as exc:
            print(f"clang-format failed for {relative_path}: {exc}", file=sys.stderr)
            return exc.returncode

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
