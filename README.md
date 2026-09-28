---
generated_by: "Antigravity AI (Gemini 3.8 Flash)"
timestamp: "2026-09-28T12:35:00-05:00"
---

# Constellation

An interactive 3D GNSS orbital visualizer for the **Pebble Time 2 (emery)** smartwatch. Visualizes the global navigation satellite infrastructure—**GPS** (USA), **Galileo** (EU), **GLONASS** (Russia), and **BeiDou** (China)—in 3D orbit around a rotating Earth globe on your wrist.

```text
       ┌───────────┐
       │ GNSS CAGE │ 10:42
       ├───────────┤
       │   \  |  / │
       │  -- (⊕) --│  <- 3D MEO orbital rings & PRN nodes
       │   /  |  \ │
       ├───────────┤
       │ GPS: 12   │ GAL: 9 • FIX
       └───────────┘
```

## Features

- **3D Orthographic Celestial Globe:** Renders an Earth globe centered on your coordinates with blue oceans, mint continental land outlines, an atmospheric limb, and an amber ground beacon (**YOU**).
- **MEO Orbital Planes in 3D:** True 3D elliptical wireframe rings for active constellation planes:
  - **GPS:** 6 orbital planes at $55^\circ$ inclination (Golden Amber).
  - **Galileo:** 3 orbital planes at $56^\circ$ inclination (Electric Cyan).
  - **GLONASS:** 3 orbital planes at $64.8^\circ$ inclination (Malachite Green).
  - **BeiDou:** 3 MEO orbital planes at $55^\circ$ inclination (Magenta).
- **Live Satellite Nodes & PRNs:** Satellite positions calculated from CelesTrak TLEs using `satellite.js` and packed into compact binary tuples. Direct line-of-sight satellites ($>0^\circ$ elevation) are brightly illuminated with PRN tags (`G14`, `E05`); occluded satellites are rendered discreetly.
- **Interactive Button Controls:**
  - **UP / DOWN:** Rotate the globe and orbital cage around the yaw axis to inspect satellite geometry from any angle.
  - **SELECT (single click):** Instantly toggle PRN labels on/off.
  - **SELECT (long click):** Reset manual rotation to home center and request a fresh TLE sync from the phone.
- **Full Configuration Support:**
  - Toggle PRN text labels.
  - Toggle individual constellations (GPS, Galileo, GLONASS, BeiDou).
  - Choose between Phone GPS or custom Home Latitude/Longitude.
- **Offline Persistence:** Automatically caches the latest constellation telemetry and orbital state in Pebble persistent storage (`storage.c`), launching immediately without waiting for phone tethering.

---

## Interactive Controls

| Button | Action |
|---|---|
| **UP** | Rotate view yaw left (-15°) |
| **DOWN** | Rotate view yaw right (+15°) |
| **SELECT** | Toggle PRN labels ON / OFF |
| **Hold SELECT** | Reset view rotation & request fresh satellite sync |

---

## Architecture

This project implements the workshop 3-layer architecture:

```text
Layer 1: directives/visualize_constellation.md
         Markdown SOP defining operational requirements and telemetry models.

Layer 2: Orchestration (Antigravity AI)
         Verification, test execution, and learning capture.

Layer 3: watchface/ & execution/
         - watchface/src/c/ (3D fixed-point projection, Earth rendering, HUD)
         - watchface/src/pkjs/ (TLE ingestion, satellite propagation, settings HTML)
         - execution/build_earth_data.py (Continental coastline compiler)
         - execution/emulator_check.py (Pebble emery QEMU verification)
```

---

## Building and Testing

### 1. Build Pebble App (.pbw)
```bash
cd watchface
pebble build
```
Generates `watchface/build/watchface.pbw` targeting the `emery` platform.

### 2. Run Test Suite
```bash
python3 -m pytest tests/ -v
```

### 3. Emulator Verification (Dry Run or Live)
```bash
python3 execution/emulator_check.py --dry-run
python3 execution/emulator_check.py
```
Outputs emulator capture to `.tmp/emulator/constellation_emery.png`.
