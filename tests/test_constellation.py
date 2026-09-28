import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
WATCHFACE = ROOT / "watchface"

def test_package_json_structure():
    pkg_file = WATCHFACE / "package.json"
    assert pkg_file.exists()
    with open(pkg_file) as f:
        data = json.load(f)

    pebble_cfg = data.get("pebble", {})
    assert "emery" in pebble_cfg.get("targetPlatforms", [])
    assert pebble_cfg.get("watchapp", {}).get("watchface") is False
    assert "location" in pebble_cfg.get("capabilities", [])
    assert "configurable" in pebble_cfg.get("capabilities", [])

    keys = pebble_cfg.get("messageKeys", [])
    required_keys = [
        "CMD", "STATUS", "SHOW_LABELS",
        "ENABLE_GPS", "ENABLE_GALILEO", "ENABLE_GLONASS", "ENABLE_BEIDOU",
        "CENTER_LAT", "CENTER_LON",
        "SAT_COUNT", "SAT_DATA",
        "PLANE_COUNT", "PLANE_DATA",
        "PDOP", "HDOP", "VDOP", "TDOP", "GDOP",
        "DOP_VALID", "TLE_AGE_H"
    ]
    for rk in required_keys:
        assert rk in keys, f"Missing messageKey: {rk}"

def test_config_html_options():
    cfg_html = WATCHFACE / "src" / "pkjs" / "config.html"
    assert cfg_html.exists()
    content = cfg_html.read_text()
    assert "show_labels" in content
    assert "enable_gps" in content
    assert "enable_galileo" in content
    assert "enable_glonass" in content
    assert "enable_beidou" in content
    assert "custom_lat" in content
    assert "custom_lon" in content
    assert "id=\"show_labels\" checked" not in content
    assert "pebblejs://close#" in content
    assert "NORAD catalog numbers" in content
    assert "latitude -90 to 90" in content

def test_panes_source_files_exist():
    c_dir = WATCHFACE / "src" / "c"
    expected_files = [
        "globe.h", "globe.c",
        "skyplot.h", "skyplot.c",
        "geodesy.h", "geodesy.c",
        "ground_track.h", "ground_track.c",
        "hud.h", "hud.c",
        "storage.h", "storage.c",
        "earth_land.h", "main.c"
    ]
    for ef in expected_files:
        path = c_dir / ef
        assert path.exists(), f"Missing C component: {ef}"

def test_earth_coastlines_header():
    header_file = WATCHFACE / "src" / "c" / "earth_land.h"
    assert header_file.exists()
    content = header_file.read_text()
    assert "s_earth_coastlines" in content
    assert "s_earth_coastline_count" in content

def test_pebble_build():
    cmd = ["pebble", "build"]
    res = subprocess.run(cmd, cwd=WATCHFACE, capture_output=True, text=True, check=False)
    assert res.returncode == 0, f"pebble build failed: {res.stderr}"
    pbw_file = WATCHFACE / "build" / "watchface.pbw"
    assert pbw_file.exists(), "watchface.pbw not created"


def test_source_does_not_claim_receiver_measurements():
    pkjs = (WATCHFACE / "src/pkjs/index.js").read_text()
    assert "show_labels: false" in pkjs
    geodesy = (WATCHFACE / "src/c/geodesy.c").read_text()
    skyplot = (WATCHFACE / "src/c/skyplot.c").read_text()
    assert "Estimated Position Error" not in pkjs + geodesy
    assert "3D-DIFF" not in pkjs + geodesy
    assert "GPS-UTC" not in geodesy
    assert '"ELEVATION"' in skyplot
    assert "sat->catalog_id" in (WATCHFACE / "src/c/globe.c").read_text()
    assert "FALLBACK" not in pkjs


def test_js_geometry_contract():
    result = subprocess.run(["node", "--test", str(ROOT / "watchface/test/accuracy.test.js")],
                            cwd=ROOT, capture_output=True, text=True, check=False)
    assert result.returncode == 0, result.stdout + result.stderr


def test_public_copy_describes_the_actual_tle_model():
    readme = (ROOT / "README.md").read_text().lower()
    store = (ROOT / "store/description.txt").read_text().lower()
    for copy in (readme, store):
        assert "not a gnss receiver" in copy
        assert "not fitted" in copy or "schematic" in copy
        assert "bars show elevation only" in copy or "bars represent elevation only, not signal strength" in copy
    assert "accuracy estimate" in readme
    assert "not sent to celestrak" in store
    assert len((ROOT / "store/description.txt").read_text()) <= 1600
    assert "receiver fix" not in store
    assert "epe" not in store
