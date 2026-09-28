---
generated_by: "Antigravity AI (Gemini 3.8 Flash)"
timestamp: "2026-09-28T12:15:00-05:00"
title: Visualize Constellation
verb_noun: visualize_constellation
layer: 1 — Directive
applies_to: constellation-watch
---

# Visualize Constellation

A Pebble Time 2 (emery) interactive watchapp for visualizing global navigation satellite systems (GPS, Galileo, GLONASS, BeiDou) in 3D orbit around a rotating Earth.

## Goal
Deliver a responsive, low-power, instrument-grade 3D orthographic globe view on the Pebble Time 2 (200 × 228, 64-color) displaying:
1. Shaded/gridded Earth centered on the user's location (or custom coordinates).
2. 3D elliptical orbital rings corresponding to active constellation planes.
3. Satellite nodes positioned along their true orbital paths.
4. Optional PRN labels (e.g. `G14`, `E05`) toggleable by the user.
5. User configuration for constellation toggles (GPS, Galileo, GLONASS, BeiDou), label visibility, and home center location.
6. Offline caching in persistent watch storage so the constellation renders instantly upon launch.

## Architecture & Data Flow

### Layer 1: Phone / PebbleKit JS (`src/pkjs/`)
- **Settings / Config:** Configurable webview allowing toggling of labels, constellation selection (GPS, Galileo, GLONASS, BeiDou), and home/center coordinates.
- **Ephemeris / TLE Fetch:** Fetches current CelesTrak GP/TLE data for selected constellations (cached for 24h).
- **Orbit Propagation:** Uses vendored `satellite.js` to compute satellite ECEF/ECI Cartesian coordinates and orbital plane inclinations/nodes at epoch.
- **Binary Packing:** Packs satellite records `(constellation_id, prn, x, y, z, elevation_flag)` into a compact binary byte array sent via AppMessage to the watch.

### Layer 2: Watch Client C (`src/c/`)
- **3D Orthographic Engine (`globe.c`):**
  - Renders central Earth sphere (radius $R_{earth} \approx 28\text{ px}$ with continent vectors and equator/meridian lines).
  - Projects MEO orbital rings ($R_{orbit} \approx 70\text{–}85\text{ px}$) using fixed-point trigonometric transforms (`sin_lookup`, `cos_lookup`).
  - Colors: GPS = Amber (`GColorChromeYellow` / `GColorRajah`), Galileo = Cyan (`GColorElectricBlue` / `GColorCeleste`), GLONASS = Green (`GColorMalachite`), BeiDou = Magenta (`GColorMagenta`).
  - Renders satellite nodes with optional 1-pixel micro font or compact PRN labels (`G14`, `E05`).
  - Renders user ground beacon (**YOU**) with line-of-sight elevation indicator.
- **Interaction & Controls (`main.c`):**
  - **UP / DOWN:** Rotate globe and orbital cage around polar and azimuth axes.
  - **SELECT (short):** Toggle satellite PRN labels ON / OFF.
  - **SELECT (long):** Request fresh GPS fix and TLE refresh from phone.
- **HUD Telemetry (`hud.c`):** Top header with title/time and active fix count; bottom footer with active constellations and HDOP/geometry indicator.
- **Storage (`storage.c`):** Persists latest constellation state to `persist_write_data()` so app launches with valid telemetry even when phone is disconnected.

## Validation Criteria
- Binary compiles cleanly with `pebble build` targeting the `emery` platform.
- Memory footprint fits within Pebble's 64 KB app heap.
- Offline launch displays cached globe and orbits without crashing or waiting indefinitely for AppMessage.
- Label toggle immediately updates display without redrawing lag.
