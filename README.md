---
generated_by: "Antigravity AI (Gemini 3.8 Flash)"
timestamp: "2026-09-28T12:45:00-05:00"
---

# Constellation

An interactive geodesy and astrodynamics field watchapp for the **Pebble Time 2 (emery)** smartwatch. Visualizes global navigation satellite systems (**GPS**, **Galileo**, **GLONASS**, **BeiDou**) in real-time across four dedicated instrument panes inspired by classic Garmin GPS receivers (Foretrex 101/201/301) and Globe & Atlas Gridcore design principles.

```text
       ┌───────────┐
       │ SKYPLOT   │ 10:42
       ├───────────┤
       │  ( [14] ) │  <- Garmin-style polar radar (0°/45°/90°)
       │  -- ⊕ --  │  <- Zenith crosshairs & PRN lock boxes
       │  ( [05] ) │
       ├───────────┤
       │ █ ▄ █ ▅ █ │  <- Bottom signal / elevation ledger
       └───────────┘
```

---

## The Four Instrument Panes

Cycle through panes by **holding UP or DOWN** (or single-click UP/DOWN in Panes 2–4):

### Pane 1: 3D Macro Orbit Cage (`globe.c`)
- **Celestial Perspective:** 3D orthographic projection of the Earth centered on your location (or custom coordinates).
- **MEO Orbital Planes:** 3D elliptical wireframe rings for active constellations (GPS amber, Galileo cyan, GLONASS green, BeiDou magenta).
- **Interactive Rotation:** Single-click **UP/DOWN** rotates the globe and orbital cage around the yaw axis to inspect orbital geometry from any perspective.

### Pane 2: Polar Skyplot (`skyplot.c`)
- **Horizon Radar View:** Garmin 301-style concentric polar rings showing exactly what is overhead:
  - Outer ring = $0^\circ$ (local horizon).
  - Dashed ring = $45^\circ$ elevation.
  - Center crosshairs = $90^\circ$ Zenith (directly above your head).
  - Cardinal compass headings: **N**, **S**, **E**, **W**.
- **PRN Badges:**
  - **Solid filled badge:** Satellite locked and contributing to the navigation fix ($>15^\circ$ elevation).
  - **Hollow outlined badge:** Satellite tracking near horizon ($0^\circ\text{–}15^\circ$).
- **Signal / Elevation Histogram:** Compact vertical bars along the bottom ledger showing signal strength / elevation with PRN numbers below each bar.

### Pane 3: Geodesy & DOP Matrix (`geodesy.c`)
- **Surveyor Precision Matrix:** Full $4 \times 4$ geometry design matrix inversion computed from true line-of-sight vectors:
  - **PDOP:** 3D Position Dilution of Precision ($<2.0$ = ideal, $2\text{–}5$ = good, $>6.0$ = degraded).
  - **HDOP:** Horizontal precision factor (Lat/Lon).
  - **VDOP:** Vertical precision factor (Altitude).
  - **TDOP:** Time / clock bias factor.
  - **GDOP:** Total geometric dilution of precision ($\sqrt{\text{PDOP}^2 + \text{TDOP}^2}$).
  - **EPE:** Estimated Position Error radius in meters (e.g. $\pm 2.8\text{m}$).
- **Multi-GNSS Breakdown:** Tracking vs. Fix count tally for GPS, Galileo, GLONASS, and BeiDou.
- **Clockwork Telemetry:** GPS Time vs. UTC leap second offset ($+18\text{s}$) and fix classification (3D-DIFF, 3D-FIX).

### Pane 4: Ground Track World Map (`ground_track.c`)
- **2D Equirectangular Plate:** 2:1 aspect ratio whole-world map featuring 429-point continental coastlines from Natural Earth data.
- **Observer Footprint:** Amber **YOU** crosshair with a circular line-of-sight reception horizon footprint cone (~2,500 km radius).
- **Sub-Satellite Ground Points:** Real-time sub-satellite geographic coordinates plotted directly over the continents in constellation colors.

---

## Interactive Controls

| Button | Action |
|---|---|
| **UP (single click)** | Orbit Cage: rotate yaw left (-15°) • Other panes: previous pane |
| **DOWN (single click)**| Orbit Cage: rotate yaw right (+15°) • Other panes: next pane |
| **Hold UP** | Cycle to previous instrument pane |
| **Hold DOWN** | Cycle to next instrument pane |
| **SELECT (single click)** | Toggle PRN text labels ON / OFF |
| **Hold SELECT** | Reset view rotation & request fresh satellite sync |

---

## Configuration

In the Pebble phone app settings (or via `watchface/src/pkjs/config.html`):
- **Show PRN Labels:** Toggle PRN identifiers on/off.
- **Constellations:** Individual switches for GPS, Galileo, GLONASS, and BeiDou.
- **Center Location:** Use Phone GPS or enter custom Home Latitude/Longitude.

---

## Building and Testing

```bash
cd watchface
pebble build
```

Generates `watchface/build/watchface.pbw` targeting the `emery` platform.

```bash
# Run unit test suite:
python3 -m pytest tests/ -v

# Run emery emulator check:
python3 execution/emulator_check.py --dry-run
python3 execution/emulator_check.py
```
