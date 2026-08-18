#!/usr/bin/env python3
"""Build and publish companion environments using the current LVGL UI."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence


VERSION_RE = re.compile(r"^v\d+(?:\.\d+)+(?:[-_][0-9A-Za-z.-]+)?$")
COMPANION_ENV_RE = re.compile(r"_companion_radio_(?:ble|usb|wifi)$", re.IGNORECASE)
ACTIVE_MESH_DEBUG_RE = re.compile(
    r"^(?P<indent>\s*)(?P<flag>-D\s+MESH_DEBUG(?:\s*=\s*[^;\s]+)?)(?P<tail>\s*(?:;.*)?)$"
)
DEBUG_UNDEFINES = (
    "-UMESH_DEBUG",
    "-UBLE_DEBUG_LOGGING",
    "-UWIFI_DEBUG_LOGGING",
    "-UBRIDGE_DEBUG",
    "-UGPS_NMEA_DEBUG",
    "-UMESH_PACKET_LOGGING",
    "-UCORE_DEBUG_LEVEL",
    "-UESPNOW_DEBUG_LOGGING",
    "-UDEBUG_RP2040_WIRE",
    "-UDEBUG_RP2040_SPI",
    "-UDEBUG_RP2040_CORE",
    "-UDEBUG_RP2040_PORT",
    "-URADIOLIB_DEBUG_SPI",
    "-URADIOLIB_DEBUG_BASIC",
    "-URADIOLIB_DEBUG_PROTOCOL",
)


@dataclass(frozen=True)
class Target:
    environment: str
    platform: str
    device: str


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Build PlatformIO companion environments using the LVGL UI and publish "
            "nRF52840 UF2 / merged ESP32 BIN images to ../09_Release/<version>."
        )
    )
    parser.add_argument("version", nargs="?", help="Release version, for example v1.16.0.3_beta")
    parser.add_argument(
        "--release-root",
        type=Path,
        help="Release root directory (default: sibling 09_Release next to this repository)",
    )
    parser.add_argument("--pio", type=Path, help="Path to pio or pio.exe")
    parser.add_argument(
        "--env",
        action="append",
        default=[],
        metavar="NAME",
        help="Only build this LVGL companion environment; repeat for multiple environments",
    )
    parser.add_argument("--list", action="store_true", help="List matching environments and exit")
    parser.add_argument("--dry-run", action="store_true", help="Print build/publish actions without running them")
    parser.add_argument("--force", action="store_true", help="Replace an existing version directory")
    return parser.parse_args(argv)


def find_repo_root() -> Path:
    script_dir = Path(__file__).resolve().parent
    repo = script_dir.parent
    if not (repo / "platformio.ini").is_file():
        raise RuntimeError(f"platformio.ini was not found under {repo}")
    return repo


def find_pio(explicit: Path | None) -> Path:
    candidates: list[Path] = []
    if explicit:
        candidates.append(explicit.expanduser())

    for name in ("pio", "pio.exe", "platformio", "platformio.exe"):
        found = shutil.which(name)
        if found:
            candidates.append(Path(found))

    user_home = Path.home()
    candidates.extend(
        [
            user_home / ".platformio" / "penv" / "Scripts" / "pio.exe",
            user_home / ".platformio" / "penv" / "bin" / "pio",
        ]
    )
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    raise RuntimeError("PlatformIO was not found; pass --pio with the pio executable path")


def run(
    command: Sequence[os.PathLike[str] | str],
    *,
    cwd: Path,
    env: dict[str, str] | None = None,
    capture: bool = False,
) -> subprocess.CompletedProcess[str]:
    printable = subprocess.list2cmdline([str(part) for part in command])
    print(f"> {printable}", flush=True)
    return subprocess.run(
        [str(part) for part in command],
        cwd=cwd,
        env=env,
        check=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        stdout=subprocess.PIPE if capture else None,
    )


def option_map(options: Sequence[Sequence[object]]) -> dict[str, object]:
    return {str(key): value for key, value in options}


def option_text(options: dict[str, object], name: str) -> str:
    value = options.get(name, "")
    if isinstance(value, list):
        return "\n".join(str(item) for item in value)
    return str(value)


def is_lvgl_environment(options: dict[str, object]) -> bool:
    libs = option_text(options, "lib_deps").replace("\\", "/").lower()
    sources = option_text(options, "build_src_filter").replace("\\", "/").lower()
    links_lvgl = "file://lib/lvgl" in libs
    compiles_current_ui = "+<heltec/ui/" in sources or "+<heltec/ui/**" in sources
    return links_lvgl and compiles_current_ui


def is_companion_environment(environment: str) -> bool:
    return COMPANION_ENV_RE.search(environment) is not None


def target_platform(options: dict[str, object]) -> str | None:
    flags = option_text(options, "build_flags")
    if "NRF52_PLATFORM" in flags:
        return "nrf52"
    if "ESP32_PLATFORM" in flags:
        return "esp32"
    return None


def device_name(environment: str, options: dict[str, object]) -> str:
    name = environment.lower()
    board = str(options.get("board", "")).lower()
    if name.startswith("heltec_t096_") or board == "heltec_t096":
        return "T096"
    if name.startswith("heltec_t114_") or board == "heltec_t114":
        return "T114"
    if name.startswith("heltec_t1_") or board == "heltec_t1":
        return "T1"
    if "tracker_v2" in name or board == "heltec_tracker_v2":
        return "Tracker V2"
    if name.startswith("heltec_wireless_tracker_"):
        return "Tracker"
    if name.startswith("heltec_wsl3_"):
        return "WSL3"
    if name.startswith("heltec_v4_r8_") or board == "heltec_v4_r8":
        return "V4 R8"
    if name.startswith("heltec_v4_") or board == "heltec_v4":
        return "V4"
    if name.startswith("heltec_v3_"):
        return "V3"

    prefix = re.split(
        r"_(?:companion|repeater|room_server|terminal_chat|sensor|kiss_modem)",
        environment,
        maxsplit=1,
        flags=re.IGNORECASE,
    )[0]
    return prefix.replace("_", " ")


def load_targets(repo: Path, pio: Path) -> list[Target]:
    result = run([pio, "project", "config", "--json-output"], cwd=repo, capture=True)
    config = json.loads(result.stdout.lstrip("\ufeff"))
    targets: list[Target] = []
    for section, raw_options in config:
        if not str(section).startswith("env:"):
            continue
        environment = str(section)[4:]
        if not is_companion_environment(environment):
            continue
        options = option_map(raw_options)
        if not is_lvgl_environment(options):
            continue
        platform = target_platform(options)
        if platform not in ("nrf52", "esp32"):
            continue
        targets.append(Target(environment, platform, device_name(environment, options)))
    return sorted(targets, key=lambda item: (item.device.casefold(), item.environment.casefold()))


def platformio_files(repo: Path) -> list[Path]:
    files = [repo / "platformio.ini"]
    files.extend(sorted((repo / "variants").glob("*/platformio.ini")))
    local = repo / "platformio.local.ini"
    if local.is_file():
        files.append(local)
    return files


class MeshDebugGuard:
    """Temporarily comments active PlatformIO MESH_DEBUG build flags."""

    def __init__(self, files: Iterable[Path]) -> None:
        self.files = list(files)
        self.originals: dict[Path, bytes] = {}
        self.changed_lines = 0

    def __enter__(self) -> "MeshDebugGuard":
        try:
            for path in self.files:
                if not path.is_file():
                    continue
                raw = path.read_bytes()
                text = raw.decode("utf-8")
                newline = "\r\n" if "\r\n" in text else "\n"
                lines = text.splitlines()
                changed = False
                output: list[str] = []
                for line in lines:
                    match = ACTIVE_MESH_DEBUG_RE.match(line)
                    if match:
                        line = (
                            f"{match.group('indent')}; RELEASE_DISABLED: "
                            f"{match.group('flag')}{match.group('tail')}"
                        )
                        changed = True
                        self.changed_lines += 1
                    output.append(line)
                if changed:
                    self.originals[path] = raw
                    path.write_text(newline.join(output) + newline, encoding="utf-8", newline="")
        except BaseException:
            self._restore()
            raise
        return self

    def __exit__(self, exc_type: object, exc: object, traceback: object) -> None:
        self._restore()

    def _restore(self) -> None:
        for path, raw in self.originals.items():
            path.write_bytes(raw)


def version_flags(version: str) -> str:
    build_date = dt.datetime.now().strftime("%d-%b-%Y")
    return " ".join(
        [
            *DEBUG_UNDEFINES,
            f'-DFIRMWARE_BUILD_DATE=\\"{build_date}\\"',
            f'-DFIRMWARE_VERSION=\\"{version}\\"',
        ]
    )


def build_target(repo: Path, pio: Path, target: Target, version: str) -> Path:
    process_env = os.environ.copy()
    existing_flags = process_env.get("PLATFORMIO_BUILD_FLAGS", "").strip()
    process_env["PLATFORMIO_BUILD_FLAGS"] = " ".join(
        part for part in (existing_flags, version_flags(version)) if part
    )

    run([pio, "run", "-e", target.environment], cwd=repo, env=process_env)
    build_dir = repo / ".pio" / "build" / target.environment
    if target.platform == "nrf52":
        artifact = build_dir / "firmware.uf2"
        run(
            [pio, "run", "-e", target.environment, "-t", "create_uf2"],
            cwd=repo,
            env=process_env,
        )
    else:
        run([pio, "run", "-e", target.environment, "-t", "mergebin"], cwd=repo, env=process_env)
        artifact = build_dir / "firmware-merged.bin"

    if not artifact.is_file() or artifact.stat().st_size == 0:
        raise RuntimeError(f"Expected artifact was not generated: {artifact}")
    return artifact


def artifact_name(target: Target, version: str) -> str:
    suffix = ".uf2" if target.platform == "nrf52" else ".bin"
    environment = re.sub(r"^heltec_", "", target.environment, flags=re.IGNORECASE)
    return f"Heltec_F&T_{environment}-{version}{suffix}"


def prepare_release_dir(path: Path, force: bool) -> None:
    #if path.exists() and not force:
    #    raise RuntimeError(f"Release directory already exists: {path}; pass --force to replace it")
    if not path.exists():
        path.mkdir(parents=True)


def print_targets(targets: Sequence[Target]) -> None:
    for target in targets:
        extension = "uf2" if target.platform == "nrf52" else "merged bin"
        print(f"{target.device:<12} {target.environment:<52} {extension}")
    print(f"Total: {len(targets)} environment(s)")


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    if not args.list and not args.version:
        raise RuntimeError("A version is required unless --list is used")
    if args.version and not VERSION_RE.fullmatch(args.version):
        raise RuntimeError("Invalid version; expected a value such as v1.16.0.3_beta")

    repo = find_repo_root()
    pio = find_pio(args.pio)
    targets = load_targets(repo, pio)
    if args.env:
        requested = set(args.env)
        known = {target.environment for target in targets}
        missing = sorted(requested - known)
        if missing:
            raise RuntimeError(
                f"Not an LVGL companion nRF52/ESP32 environment: {', '.join(missing)}"
            )
        targets = [target for target in targets if target.environment in requested]
    if not targets:
        raise RuntimeError("No matching LVGL companion nRF52/ESP32 environments were found")

    print_targets(targets)
    if args.list:
        return 0

    release_root = (args.release_root or (repo.parent / "09_Release")).resolve()
    release_dir = release_root / args.version
    print(f"Release directory: {release_dir}")
    if args.dry_run:
        return 0

    prepare_release_dir(release_dir, args.force)
    published: list[Path] = []
    try:
        with MeshDebugGuard(platformio_files(repo)) as debug_guard:
            print(f"Temporarily disabled {debug_guard.changed_lines} active MESH_DEBUG flag(s).")
            for index, target in enumerate(targets, 1):
                print(f"\n[{index}/{len(targets)}] Building {target.environment}", flush=True)
                source = build_target(repo, pio, target, args.version)
                device_dir = release_dir / target.device
                device_dir.mkdir(parents=True, exist_ok=True)
                destination = device_dir / artifact_name(target, args.version)
                shutil.copy2(source, destination)
                published.append(destination)
                print(f"Published: {destination}")
    except BaseException:
        #if release_dir.exists():
        #    shutil.rmtree(release_dir)
        raise

    print(f"\nPublished {len(published)} firmware image(s) to {release_dir}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, subprocess.CalledProcessError, json.JSONDecodeError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        raise SystemExit(1)
