# Constellation

An educational orbital-geometry watchapp for Pebble Time 2 (emery). It propagates public CelesTrak GP/TLE element sets with SGP4 and displays modeled satellite positions for GPS, Galileo, GLONASS, and BeiDou. It is **not a GNSS receiver**: it does not receive satellite signals, produce a navigation fix, or estimate real position accuracy. CelesTrak notes that TLE data do not include accuracy information ([TLE format](https://celestrak.org/NORAD/documentation/tle-fmt.php), [SGP4 tutorial](https://www.celestrak.org/software/tutorials/sgp4.php)).

## Install and source

- [RePebble listing](https://apps.repebble.com/constellation_10f46ae849224d009644f7e8)
- [GitHub release v0.1.2](https://github.com/globe-and-atlas/constellation-watch/releases/tag/v0.1.2)


## Four views

- **Orbit globe:** modeled satellite positions around a simplified Earth. The displayed orbital-plane rings are schematic guides and are not fitted from the satellite TLEs.
- **Skyplot:** modeled azimuth and elevation relative to the selected center. Filled markers mean elevation exceeds the app's 15° geometry mask; they do not mean a receiver has tracked or locked a signal. Bars show elevation only.
- **Geometry:** geometric DOP values derived from modeled look directions when the matrix is solvable. Values show as N/A when fewer than four suitable directions exist. DOP characterizes modeled geometry, not real-world position accuracy.
- **World map:** simplified coastlines, selected center, and modeled sub-satellite points. The map does not show a reception-footprint circle.

The selected center comes from phone location or validated coordinates entered in settings. Coordinates are used on-device and are not sent to CelesTrak; the phone requests TLE data from CelesTrak. TLE age is displayed because element freshness matters, but age is not an accuracy estimate. Fresh data require an internet connection. Location permission is required when phone GPS is selected; location failure is shown rather than replaced with a sample location.

## Controls

| Button | Action |
|---|---|
| UP/DOWN click | Rotate the globe; change panes in other views |
| Hold UP/DOWN | Previous/next pane |
| SELECT click | Toggle NORAD catalog-number labels on the globe |
| Hold SELECT | Refresh TLE data and location |

NORAD catalog numbers are parsed from line 1 of each TLE ([format reference](https://celestrak.org/NORAD/documentation/tle-fmt.php)); they are not GNSS PRNs.

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
