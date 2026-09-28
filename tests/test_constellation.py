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
        "PLANE_COUNT", "PLANE_DATA"
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
    assert "pebblejs://close#" in content

def test_earth_coastlines_header():
    header_file = WATCHFACE / "src" / "c" / "earth_land.h"
    assert header_file.exists()
    content = header_file.read_text()
    assert "s_earth_coastlines" in content
    assert "s_earth_coastline_count" in content

def test_pebble_build():
    cmd = ["pebble", "build"]
    res = subprocess.run(cmd, cwd=WATCHFACE, capture_output=True, text=True)
    assert res.returncode == 0, f"pebble build failed: {res.stderr}"
    pbw_file = WATCHFACE / "build" / "watchface.pbw"
    assert pbw_file.exists(), "watchface.pbw not created"
