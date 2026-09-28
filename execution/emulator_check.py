#!/usr/bin/env python3
"""Build and capture each Constellation pane on a clean Pebble Time 2 emulator.

Screenshots are written under .tmp/emulator for visual inspection before they are
copied into store/. The app uses actual CelesTrak data and explicit location
settings; the script does not inject sample coordinates or orbital records.
"""
from __future__ import annotations

import argparse
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WATCHFACE = ROOT / "watchface"
OUT = ROOT / ".tmp" / "emulator"
PANES = ("orbit", "skyplot", "geodesy", "ground_track")


def run(args: list[str], timeout: int = 120) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=WATCHFACE, capture_output=True, text=True,
                          timeout=timeout, check=False)


def clean_kill() -> None:
    run(["pebble", "kill"])
    for proc in ("pypkjs", "qemu-pebble"):
        subprocess.run(["pkill", "-x", proc], capture_output=True, check=False)
    time.sleep(2)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    if args.dry_run:
        print(f"[dry-run] Would install on Emery and capture {len(PANES)} panes under {OUT}")
        return 0

    OUT.mkdir(parents=True, exist_ok=True)
    build = run(["pebble", "clean"])
    if build.returncode:
        print(build.stderr, file=sys.stderr)
        return 1
    build = run(["pebble", "build"])
    if build.returncode:
        print(build.stderr, file=sys.stderr)
        return 1

    print("Cleaning stale emulator processes...")
    clean_kill()
    installed = run(["pebble", "install", "--emulator", "emery"], timeout=180)
    if installed.returncode:
        print(installed.stdout + installed.stderr, file=sys.stderr)
        return 1
    time.sleep(25)  # permit the phone-side script to request location and fresh TLEs

    for index, pane in enumerate(PANES):
        path = OUT / f"constellation_{pane}.png"
        shot = run(["pebble", "screenshot", "--emulator", "emery", "--no-open",
                    "--no-correction", str(path)], timeout=60)
        if shot.returncode or not path.exists():
            print(shot.stdout + shot.stderr, file=sys.stderr)
            return 1
        print(f"Captured {pane}: {path}")
        if index < len(PANES) - 1:
            click = run(["pebble", "emu-button", "click", "down", "--duration", "650",
                         "--emulator", "emery"], timeout=30)
            if click.returncode:
                print(click.stdout + click.stderr, file=sys.stderr)
                return 1
            time.sleep(2)
    return 0


if __name__ == "__main__":
    sys.exit(main())
