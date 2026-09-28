#!/usr/bin/env python3
"""
execution/emulator_check.py
Boots clean Pebble emery emulator, installs constellation-watch, and takes a screenshot.
Respects the _PEBBLE skill guidelines:
- Never pkill -9 QEMU
- One emulator driver at a time
- Screenshots with --no-open --no-correction

Usage:
  python3 execution/emulator_check.py [--dry-run]
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

def run(args: list[str], timeout: int = 120) -> subprocess.CompletedProcess:
    return subprocess.run(args, cwd=WATCHFACE, capture_output=True, text=True, timeout=timeout, check=False)

def clean_kill():
    run(["pebble", "kill"])
    for proc in ("pypkjs", "qemu-pebble"):
        subprocess.run(["pkill", "-f", proc], capture_output=True, check=False)
    time.sleep(2)

def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--dry-run", action="store_true", help="Print actions without booting emulator")
    args = ap.parse_args()

    if args.dry_run:
        print(f"[dry-run] Would build watchface, clean stale emulators, install on emery, and capture screenshot to {OUT}")
        return 0

    OUT.mkdir(parents=True, exist_ok=True)
    shot_path = OUT / "constellation_emery.png"

    print("Building watchface...")
    build_res = run(["pebble", "build"])
    if build_res.returncode != 0:
        print(f"Build failed: {build_res.stderr}", file=sys.stderr)
        return 1

    print("Cleaning stale emulator processes...")
    clean_kill()

    print("Installing on emery emulator (1st pass to boot QEMU)...")
    run(["pebble", "install", "--emulator", "emery"])
    time.sleep(15)

    print("Re-installing to ensure clean app load...")
    install_res = run(["pebble", "install", "--emulator", "emery"])
    if install_res.returncode != 0:
        print(f"Install warning/failed: {install_res.stderr}")

    time.sleep(4)
    print(f"Capturing screenshot to {shot_path}...")
    shot_res = run(["pebble", "screenshot", "--emulator", "emery", "--no-open", "--no-correction", str(shot_path)])
    if shot_res.returncode == 0 and shot_path.exists():
        print(f"✓ Screenshot saved: {shot_path}")
        return 0
    else:
        print(f"Screenshot failed: {shot_res.stderr}", file=sys.stderr)
        return 1

if __name__ == "__main__":
    sys.exit(main())
