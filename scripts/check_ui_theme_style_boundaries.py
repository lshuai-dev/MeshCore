#!/usr/bin/env python3
"""Reject LVGL Style ownership outside the UI theme directory."""

from pathlib import Path
import re
import sys


REPO_ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOTS = (REPO_ROOT / "src", REPO_ROOT / "variants")
THEME_ROOT = REPO_ROOT / "src" / "heltec" / "ui" / "theme"
SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".h", ".hh", ".hpp"}

FORBIDDEN = re.compile(
    r"\blv_style_t\b"
    r"|\blv_style_(?:init|reset|set_)"
    r"|\blv_obj_(?:set_style_|add_style|remove_style)"
)


def main() -> int:
    violations: list[str] = []
    for source_root in SOURCE_ROOTS:
        for path in source_root.rglob("*"):
            if not path.is_file() or path.suffix not in SOURCE_SUFFIXES:
                continue
            if THEME_ROOT in path.parents:
                continue
            for line_number, line in enumerate(
                path.read_text(encoding="utf-8").splitlines(), start=1
            ):
                if FORBIDDEN.search(line):
                    relative = path.relative_to(REPO_ROOT).as_posix()
                    violations.append(f"{relative}:{line_number}: {line.strip()}")

    if not violations:
        print("Project-wide UI theme Style boundary check passed")
        return 0

    print("LVGL Style ownership must stay under src/heltec/ui/theme/:")
    for violation in violations:
        print(f"  {violation}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
