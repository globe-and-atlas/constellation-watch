# Constellation Watch Tasks

<!--
ISC Criteria Reminder:
- Atomic: one condition per criterion, testable with binary yes/no
- Splitting Test: no compound "and" conditions — split into separate items
- Scope words: explicitly enumerate covered cases
- Independent failure: each criterion must be falsifiable on its own
-->

## Acceptance Criteria

### Core Infrastructure & 3D Orbit Cage (Pane 0)
- [x] `watchface/package.json` defines an interactive watchapp for the `emery` platform.
- [x] `watchface/package.json` includes configuration capability for user settings.
- [x] `watchface/src/pkjs/config.html` provides a toggle switch for satellite PRN labels.
- [x] `watchface/src/pkjs/config.html` provides toggle switches for GPS constellation.
- [x] `watchface/src/pkjs/config.html` provides toggle switches for Galileo constellation.
- [x] `watchface/src/pkjs/config.html` provides toggle switches for GLONASS constellation.
- [x] `watchface/src/pkjs/config.html` provides toggle switches for BeiDou constellation.
- [x] `watchface/src/pkjs/config.html` provides text inputs for custom home latitude and longitude.
- [x] `watchface/src/pkjs/index.js` fetches CelesTrak TLEs for enabled constellations.
- [x] `watchface/src/pkjs/index.js` packs satellite coordinates into binary byte packets for AppMessage.
- [x] `watchface/src/c/globe.c` renders an orthographic 3D projection of Earth.
- [x] `watchface/src/c/globe.c` renders 3D orbital rings for active constellation planes.
- [x] `watchface/src/c/globe.c` draws satellite markers on projected orbital rings.
- [x] `watchface/src/c/globe.c` displays PRN labels when the label option is active.
- [x] `watchface/src/c/globe.c` hides PRN labels when the label option is disabled.
- [x] `watchface/src/c/main.c` cycles or rotates the 3D globe view on UP and DOWN button clicks.
- [x] `watchface/src/c/main.c` toggles PRN label visibility on SELECT button click.
- [x] `watchface/src/c/storage.c` caches the last received constellation packet in persistent storage.
- [x] `watchface/src/c/storage.c` restores cached constellation data when launching offline.
- [x] `pebble build` compiles the project without errors for `emery`.

### Multi-Pane Instrument Deck (Panes 1, 2, 3)
- [x] `watchface/src/c/skyplot.c` renders a polar azimuth/elevation radar grid (horizon, 45° ring, zenith crosshair).
- [x] `watchface/src/c/skyplot.c` plots visible satellite PRN boxes at polar azimuth and elevation coordinates.
- [x] `watchface/src/c/skyplot.c` displays signal/elevation bar histograms along the bottom ledger.
- [x] `watchface/src/c/geodesy.c` renders a tabular dilution-of-precision matrix (PDOP, HDOP, VDOP, TDOP, GDOP, EPE).
- [x] `watchface/src/c/geodesy.c` displays constellation tracking breakdown and GPS-UTC leap second offset.
- [x] `watchface/src/c/ground_track.c` renders a 2D equirectangular world map with continental coastlines.
- [x] `watchface/src/c/ground_track.c` plots sub-satellite ground points and observer horizon footprint circle.
- [x] `watchface/src/c/main.c` switches active display panes on long-press of UP and DOWN buttons.
- [x] `watchface/src/pkjs/index.js` computes dilution of precision values (PDOP, HDOP, VDOP, TDOP, GDOP, EPE).
- [x] `watchface/src/pkjs/index.js` transmits azimuth, elevation, sub-satellite lat/lon, and DOP values to the watch.
