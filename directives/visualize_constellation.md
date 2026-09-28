---
generated_by: "Antigravity AI (Gemini 3.8 Flash)"
timestamp: "2026-09-28T12:45:00-05:00"
title: Visualize Constellation
verb_noun: visualize_constellation
layer: 1 — Directive
applies_to: constellation-watch
---

# Visualize Constellation

A Pebble Time 2 (emery) interactive geodesy and astrodynamics watchapp for visualizing global navigation satellite systems (GPS, Galileo, GLONASS, BeiDou) across four specialized instrument panes.

## Goal
Deliver a responsive, low-power, instrument-grade multi-pane field deck on the Pebble Time 2 (200 × 228, 64-color) displaying:
1. **Pane 1: 3D Orbit Cage (`globe.c`):** Shaded Earth globe centered on user coordinates with 3D elliptical MEO orbital rings and satellite nodes.
2. **Pane 2: Polar Skyplot (`skyplot.c`):** Classic Garmin 301-style horizon/zenith radar grid ($0^\circ$, $45^\circ$, $90^\circ$ crosshairs, N/S/E/W) with PRN boxes and bottom signal/elevation histogram bars.
3. **Pane 3: Geodesy & DOP Matrix (`geodesy.c`):** Dense surveyor telemetry ledger with PDOP, HDOP, VDOP, TDOP, GDOP, EPE, constellation tally (GPS/GAL/GLO/BDS), and GPS-UTC leap second offset.
4. **Pane 4: Ground Track Map (`ground_track.c`):** 2D equirectangular world map with continental coastlines, user location with line-of-sight horizon footprint circle, and sub-satellite ground points.

## Architecture & Data Flow

### Layer 1: Phone / PebbleKit JS (`src/pkjs/`)
- **Settings / Config:** Configurable webview allowing toggling of labels, constellation selection (GPS, Galileo, GLONASS, BeiDou), and home/center coordinates.
- **Ephemeris / TLE Fetch:** Fetches current CelesTrak GP/TLE data for selected constellations (cached for 24h).
- **Orbit & Geodesy Engine:** Uses vendored `satellite.js` to compute satellite ECEF/ECI coordinates, observer look angles (azimuth, elevation), sub-satellite geodetic coordinates, and a $4 \times 4$ geometry design matrix inversion for exact Dilution of Precision (PDOP, HDOP, VDOP, TDOP, GDOP, EPE).
- **Binary Packing:** Packs 13-byte satellite records `(constellation, prn, x, y, z, el, az/2, lat, lon/2, flags)` and DOP telemetry integers into AppMessage payloads.

### Layer 2: Watch Client C (`src/c/`)
- **Pane 0: 3D Orbit Cage (`globe.c`):** Fixed-point trigonometric orthographic projection of Earth and MEO rings ($R_{orbit} \approx 76\text{ px}$).
- **Pane 1: Polar Skyplot (`skyplot.c`):** Polar radar with cardinal headings, PRN boxes (solid = locked, hollow = tracking), and elevation histogram.
- **Pane 2: Geodesy Matrix (`geodesy.c`):** Ruled Gridcore telemetry breakdown of spatial precision and satellite tally.
- **Pane 3: Ground Track Map (`ground_track.c`):** 2:1 equirectangular map with 429-point continental coastlines and user horizon cone footprint.
- **HUD Telemetry (`hud.c`):** Dynamic top header and bottom footer with pane indicator and rotation feedback.
- **Storage (`storage.c`):** Persistent caching of latest constellation state and active pane to Pebble flash memory.

## Interactive Controls

| Button | Action |
|---|---|
| **UP (single click)** | Orbit Cage: rotate yaw left (-15°) • Other panes: previous pane |
| **DOWN (single click)**| Orbit Cage: rotate yaw right (+15°) • Other panes: next pane |
| **Hold UP** | Cycle to previous instrument pane |
| **Hold DOWN** | Cycle to next instrument pane |
| **SELECT (single click)** | Toggle PRN text labels ON / OFF |
| **Hold SELECT** | Reset view rotation & request fresh satellite sync |

## Validation Criteria
- Binary compiles cleanly with `pebble build` targeting the `emery` platform.
- Memory footprint fits within Pebble's 64 KB app heap (~115 KB free heap).
- Offline launch displays cached telemetry without waiting for AppMessage.
- Seamless cycling between all 4 panes without memory leaks or graphic corruption.


## 2026-09-28 accuracy correction

This app propagates public GP/TLE elements to visualize orbit geometry. It is not connected to a GNSS receiver and must not call modeled satellites tracked/locked, claim a position fix or estimated position error, or display GPS receiver clock telemetry. DOP may be shown only as geometric DOP derived from modeled line-of-sight directions, with singular/insufficient geometry shown as N/A. Elevation bars are not signal strength. The plane rings are schematic and the map has no fixed-radius reception footprint. Label TLE-derived age and explain that TLE data carry no accuracy estimate. A location error must not substitute a sample city.
