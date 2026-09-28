var satellite = require('./satellite');

var CELESTRAK_BASE = 'https://celestrak.org/NORAD/elements/gp.php?FORMAT=tle&GROUP=';
var TLE_CACHE_MS = 24 * 3600 * 1000; // 24 hours
var EARTH_RADIUS_KM = 6378.137;

var CMD = { REFRESH: 1 };
var CONSTELLATION = { GPS: 1, GALILEO: 2, GLONASS: 3, BEIDOU: 4 };

// Default configuration
var defaultConfig = {
  show_labels: false,
  enable_gps: true,
  enable_galileo: true,
  enable_glonass: false,
  enable_beidou: false,
  use_phone_gps: true,
  custom_lat: null,
  custom_lon: null
};

function loadConfig() {
  try {
    var raw = localStorage.getItem('config');
    if (!raw) return defaultConfig;
    var parsed = JSON.parse(raw);
    for (var k in defaultConfig) {
      if (parsed[k] === undefined) parsed[k] = defaultConfig[k];
    }
    return parsed;
  } catch (e) {
    return defaultConfig;
  }
}

function saveConfig(cfg) {
  try {
    localStorage.setItem('config', JSON.stringify(cfg));
  } catch (e) {
    console.log('Error saving config: ' + e);
  }
}

function httpGet(url, cb) {
  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.timeout = 25000;
  req.onload = function () {
    if (req.status >= 200 && req.status < 300) cb(null, req.responseText);
    else cb('HTTP ' + req.status);
  };
  req.onerror = function () { cb('network error'); };
  req.ontimeout = function () { cb('timeout'); };
  req.send();
}

function getConstellationTles(groupName, cb) {
  var cacheKey = 'tle_' + groupName;
  try {
    var cached = JSON.parse(localStorage.getItem(cacheKey));
    if (cached && (Date.now() - cached.timestamp < TLE_CACHE_MS)) {
      return cb(null, cached.lines);
    }
  } catch (e) {}

  httpGet(CELESTRAK_BASE + groupName, function (err, text) {
    if (err || !text) {
      // Fallback to stale cache if available
      try {
        var stale = JSON.parse(localStorage.getItem(cacheKey));
        if (stale && stale.lines) return cb(null, stale.lines);
      } catch (e) {}
      return cb(err || 'empty response');
    }

    var lines = text.split(/\r?\n/).map(function (l) { return l.trim(); }).filter(Boolean);
    try {
      localStorage.setItem(cacheKey, JSON.stringify({ timestamp: Date.now(), lines: lines }));
    } catch (e) {}
    cb(null, lines);
  });
}

function parseTleCatalog(lines, constType) {
  var sats = [];
  for (var i = 0; i < lines.length - 2; i += 3) {
    var name = lines[i];
    var l1 = lines[i + 1];
    var l2 = lines[i + 2];

    if (l1.charAt(0) !== '1' || l2.charAt(0) !== '2') {
      // Not a standard 3-line set, step by 1
      i -= 2;
      continue;
    }

    var catalogMatch = l1.match(/^1 (\d{5})/);
    if (!catalogMatch) continue;
    var catalogId = parseInt(catalogMatch[1], 10);

    try {
      var satrec = satellite.twoline2satrec(l1, l2);
      sats.push({
        constellation: constType,
        catalogId: catalogId,
        satrec: satrec
      });
    } catch (e) {}
  }
  return sats;
}

function getLocation(config, cb) {
  if (!config.use_phone_gps) {
    var lat = Number(config.custom_lat);
    var lon = Number(config.custom_lon);
    if (!isFinite(lat) || lat < -90 || lat > 90 || !isFinite(lon) || lon < -180 || lon > 180) {
      return cb('Enter valid latitude (-90 to 90) and longitude (-180 to 180).');
    }
    return cb(null, { latitude: lat, longitude: lon });
  }
  if (!navigator.geolocation) return cb('Phone location is unavailable.');
  navigator.geolocation.getCurrentPosition(function (pos) {
    cb(null, { latitude: pos.coords.latitude, longitude: pos.coords.longitude });
  }, function (err) {
    cb('Phone location unavailable: ' + err.message);
  }, { timeout: 10000, maximumAge: 600000 });
}

// 4x4 matrix inversion for Dilution of Precision (DOP)
function invert4x4(m) {
  var a = m.map(function (row, r) {
    var out = row.slice();
    for (var c = 0; c < 4; c++) out.push(r === c ? 1 : 0);
    return out;
  });
  for (var i = 0; i < 4; i++) {
    var pivotRow = i;
    for (var r = i + 1; r < 4; r++) {
      if (Math.abs(a[r][i]) > Math.abs(a[pivotRow][i])) pivotRow = r;
    }
    if (Math.abs(a[pivotRow][i]) < 1e-10) return null;
    var temp = a[i]; a[i] = a[pivotRow]; a[pivotRow] = temp;
    var pivot = a[i][i];
    for (var j = 0; j < 8; j++) a[i][j] /= pivot;
    for (var k = 0; k < 4; k++) {
      if (k === i) continue;
      var factor = a[k][i];
      for (var c = 0; c < 8; c++) a[k][c] -= factor * a[i][c];
    }
  }
  return a.map(function (row) { return row.slice(4); });
}

function computeDOP(visibleVectors) {
  if (visibleVectors.length < 4) return null;
  var A = visibleVectors;
  var ATA = [ [0,0,0,0], [0,0,0,0], [0,0,0,0], [0,0,0,0] ];
  for (var i = 0; i < 4; i++) {
    for (var j = 0; j < 4; j++) {
      var s = 0;
      for (var k = 0; k < A.length; k++) s += A[k][i] * A[k][j];
      ATA[i][j] = s;
    }
  }
  var Q = invert4x4(ATA);
  if (!Q || Q[0][0] <= 0 || Q[1][1] <= 0 || Q[2][2] <= 0 || Q[3][3] <= 0) {
    return null;
  }
  var hdop = Math.sqrt(Q[0][0] + Q[1][1]);
  var vdop = Math.sqrt(Q[2][2]);
  var pdop = Math.sqrt(Q[0][0] + Q[1][1] + Q[2][2]);
  var tdop = Math.sqrt(Q[3][3]);
  var gdop = Math.sqrt(pdop * pdop + tdop * tdop);
  return { pdop: pdop, hdop: hdop, vdop: vdop, tdop: tdop, gdop: gdop };
}

function packSatellites(sats, userLoc, now) {
  var gmst = satellite.gstime(now);
  var obs = {
    latitude: satellite.degreesToRadians(userLoc.latitude),
    longitude: satellite.degreesToRadians(userLoc.longitude),
    height: 0
  };

  var packedBytes = [];
  var visibleCount = 0;
  var validSatCount = 0;
  var visibleVectors = [];

  for (var i = 0; i < sats.length; i++) {
    if (validSatCount >= 48) break; // Maximum watch capacity
    var item = sats[i];

    var pv = satellite.propagate(item.satrec, now);
    if (!pv || !pv.position) continue;

    var ecf = satellite.eciToEcf(pv.position, gmst);
    if (!ecf) continue;

    var look = satellite.ecfToLookAngles(obs, ecf);
    var gd = satellite.eciToGeodetic(pv.position, gmst);

    var el_deg = Math.round(satellite.radiansToDegrees(look.elevation));
    var az_deg = Math.round(satellite.radiansToDegrees(look.azimuth));
    if (az_deg < 0) az_deg += 360;
    var az_deg_div_2 = Math.floor(az_deg / 2) % 180;

    var sat_lat = Math.round(satellite.radiansToDegrees(gd.latitude));
    var sat_lon = Math.round(satellite.radiansToDegrees(gd.longitude));
    var lat_deg = Math.max(-90, Math.min(90, sat_lat));
    var lon_deg_div_2 = Math.round(sat_lon / 2);
    if (lon_deg_div_2 < -90) lon_deg_div_2 = -90;
    if (lon_deg_div_2 > 90) lon_deg_div_2 = 90;

    var flags = 0;
    if (look.elevation > 0) {
      flags |= 0x01; // Above horizon
      visibleCount++;
    }
    if (look.elevation > 0.2618) { // >15° elevation geometry threshold
      flags |= 0x02; // Above the selected 15-degree geometry mask
      // Add line-of-sight unit vector for DOP calculation
      var dx = -Math.cos(look.elevation) * Math.sin(look.azimuth);
      var dy = -Math.cos(look.elevation) * Math.cos(look.azimuth);
      var dz = -Math.sin(look.elevation);
      visibleVectors.push([dx, dy, dz, 1]);
    }

    // Normalized coordinates: radius of Earth = 100
    var nx = Math.round(100.0 * ecf.x / EARTH_RADIUS_KM);
    var ny = Math.round(100.0 * ecf.y / EARTH_RADIUS_KM);
    var nz = Math.round(100.0 * ecf.z / EARTH_RADIUS_KM);

    // 1 byte constellation
    packedBytes.push(item.constellation & 0xFF);
    // 2-byte NORAD catalog number from TLE line 1
    packedBytes.push(item.catalogId & 0xFF);
    packedBytes.push((item.catalogId >> 8) & 0xFF);

    // 2 bytes int16_t nx (little endian)
    packedBytes.push(nx & 0xFF);
    packedBytes.push((nx >> 8) & 0xFF);

    // 2 bytes int16_t ny
    packedBytes.push(ny & 0xFF);
    packedBytes.push((ny >> 8) & 0xFF);

    // 2 bytes int16_t nz
    packedBytes.push(nz & 0xFF);
    packedBytes.push((nz >> 8) & 0xFF);

    // 1 byte int8_t elevation
    packedBytes.push((el_deg < 0 ? el_deg + 256 : el_deg) & 0xFF);

    // 1 byte uint8_t azimuth / 2
    packedBytes.push(az_deg_div_2 & 0xFF);

    // 1 byte int8_t sub-satellite lat
    packedBytes.push((lat_deg < 0 ? lat_deg + 256 : lat_deg) & 0xFF);

    // 1 byte int8_t sub-satellite lon / 2
    packedBytes.push((lon_deg_div_2 < 0 ? lon_deg_div_2 + 256 : lon_deg_div_2) & 0xFF);

    // 1 byte flags
    packedBytes.push(flags & 0xFF);

    validSatCount++;
  }

  var dop = computeDOP(visibleVectors);
  var oldestEpoch = sats.length ? Math.min.apply(null, sats.map(function (item) { return (item.satrec.jdsatepoch + (item.satrec.jdsatepochF || 0)); })) : NaN;
  var ageHours = isFinite(oldestEpoch) ? Math.max(0, Math.floor((now.getTime() / 86400000 + 2440587.5 - oldestEpoch) * 24)) : 65535;

  return {
    bytes: packedBytes,
    count: validSatCount,
    visible: visibleCount,
    dop: dop,
    ageHours: ageHours
  };
}

function packPlanes(config, now) {
  var gmstDeg = satellite.radiansToDegrees(satellite.gstime(now));
  var planes = [];

  if (config.enable_gps) {
    // 6 GPS planes A-F, 55 deg inclination, 60 deg spacing
    for (var i = 0; i < 6; i++) {
      var raan = Math.round(((i * 60) - gmstDeg) % 360);
      if (raan > 180) raan -= 360;
      if (raan < -180) raan += 360;
      planes.push({ constellation: CONSTELLATION.GPS, inc_deg: 55, raan_deg: raan });
    }
  }

  if (config.enable_galileo) {
    // 3 Galileo planes A-C, 56 deg inclination, 120 deg spacing
    for (var j = 0; j < 3; j++) {
      var graan = Math.round(((j * 120) - 150 - gmstDeg) % 360);
      if (graan > 180) graan -= 360;
      if (graan < -180) graan += 360;
      planes.push({ constellation: CONSTELLATION.GALILEO, inc_deg: 56, raan_deg: graan });
    }
  }

  if (config.enable_glonass) {
    // 3 GLONASS planes, 64.8 deg inclination
    for (var k = 0; k < 3; k++) {
      var rraan = Math.round(((k * 120) - 90 - gmstDeg) % 360);
      if (rraan > 180) rraan -= 360;
      planes.push({ constellation: CONSTELLATION.GLONASS, inc_deg: 65, raan_deg: rraan });
    }
  }

  if (config.enable_beidou) {
    // BeiDou MEO 3 planes, 55 deg inclination
    for (var m = 0; m < 3; m++) {
      var braan = Math.round(((m * 120) - gmstDeg) % 360);
      if (braan > 180) braan -= 360;
      planes.push({ constellation: CONSTELLATION.BEIDOU, inc_deg: 55, raan_deg: braan });
    }
  }

  var planeBytes = [];
  for (var p = 0; p < planes.length; p++) {
    planeBytes.push(planes[p].constellation & 0xFF);
    planeBytes.push(planes[p].inc_deg & 0xFF);
    var r = planes[p].raan_deg;
    planeBytes.push(r & 0xFF);
    planeBytes.push((r >> 8) & 0xFF);
  }

  return {
    bytes: planeBytes,
    count: planes.length
  };
}

function refreshConstellation() {
  var config = loadConfig();

  getLocation(config, function (err, loc) {
    if (err) {
      Pebble.sendAppMessage({ STATUS: 'LOCATION UNAVAILABLE', SAT_COUNT: 0, SAT_DATA: [], PLANE_COUNT: 0, PLANE_DATA: [], DOP_VALID: 0 });
      console.log(err);
      return;
    }
    var fetchQueue = [];
    if (config.enable_gps) fetchQueue.push({ name: 'gps-ops', type: CONSTELLATION.GPS });
    if (config.enable_galileo) fetchQueue.push({ name: 'galileo', type: CONSTELLATION.GALILEO });
    if (config.enable_glonass) fetchQueue.push({ name: 'glo-ops', type: CONSTELLATION.GLONASS });
    if (config.enable_beidou) fetchQueue.push({ name: 'beidou', type: CONSTELLATION.BEIDOU });

    var allSats = [];

    function processNext() {
      if (fetchQueue.length === 0) {
        var now = new Date();
        var satResult = packSatellites(allSats, loc, now);
        var planeResult = packPlanes(config, now);
        var dop = satResult.dop;

        var statusMsg = satResult.visible + ' ABOVE HORIZON';
        var appMsg = {
          STATUS: statusMsg,
          SHOW_LABELS: config.show_labels ? 1 : 0,
          CENTER_LAT: Math.round(loc.latitude * 10000),
          CENTER_LON: Math.round(loc.longitude * 10000),
          PLANE_COUNT: planeResult.count,
          PLANE_DATA: planeResult.bytes,
          SAT_COUNT: satResult.count,
          SAT_DATA: satResult.bytes,
          PDOP: dop ? Math.round(dop.pdop * 10) : 0,
          HDOP: dop ? Math.round(dop.hdop * 10) : 0,
          VDOP: dop ? Math.round(dop.vdop * 10) : 0,
          TDOP: dop ? Math.round(dop.tdop * 10) : 0,
          GDOP: dop ? Math.round(dop.gdop * 10) : 0,
          DOP_VALID: dop ? 1 : 0,
          TLE_AGE_H: Math.min(65535, satResult.ageHours)
        };

        Pebble.sendAppMessage(appMsg, function () {
          console.log('Constellation update sent successfully: ' + satResult.count + ' satellites, geometric DOP: ' + (dop ? dop.gdop.toFixed(1) : 'N/A'));
        }, function (e) {
          console.log('AppMessage send error: ' + JSON.stringify(e));
        });
        return;
      }

      var target = fetchQueue.shift();
      getConstellationTles(target.name, function (err, lines) {
        if (!err && lines) {
          var parsed = parseTleCatalog(lines, target.type);
          allSats = allSats.concat(parsed);
        }
        processNext();
      });
    }

    processNext();
  });
}

Pebble.addEventListener('ready', function () {
  console.log('Constellation PKJS ready.');
  refreshConstellation();
});

Pebble.addEventListener('appmessage', function (e) {
  var cmd = e.payload.CMD;
  if (cmd === CMD.REFRESH) {
    refreshConstellation();
  }
});

// HTML Settings Configuration Webview
Pebble.addEventListener('showConfiguration', function () {
  var cfg = loadConfig();
  var configUrl = 'data:text/html;charset=utf-8,' + encodeURIComponent(
    '<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">' +
    '<title>Constellation Settings</title>' +
    '<style>' +
    'body{background:#0e1117;color:#c9d1d9;font-family:-apple-system,sans-serif;padding:16px;line-height:1.5;}' +
    'h1{color:#fff;font-size:1.25rem;text-align:center;margin-bottom:16px;}' +
    '.card{background:#161b22;border:1px solid #30363d;border-radius:8px;padding:14px;margin-bottom:14px;}' +
    '.row{display:flex;justify-content:space-between;align-items:center;padding:8px 0;border-bottom:1px solid #21262d;}' +
    '.row:last-child{border-bottom:none;}' +
    'label{font-size:0.95rem;font-weight:500;}' +
    'input[type=checkbox]{width:20px;height:20px;}' +
    'input[type=text]{width:100%;background:#0d1117;border:1px solid #30363d;color:#fff;padding:8px;border-radius:4px;margin-top:4px;box-sizing:border-box;}' +
    '.btn{display:block;width:100%;background:#238636;color:#fff;font-weight:600;border:none;border-radius:6px;padding:12px;font-size:1rem;margin-top:16px;cursor:pointer;}' +
    '</style></head><body>' +
    '<h1>CONSTELLATION SETTINGS</h1>' +
    '<div class="card">' +
    '<div class="row"><label>Show catalog IDs</label><input type="checkbox" id="show_labels"' + (cfg.show_labels ? ' checked' : '') + '></div>' +
    '</div>' +
    '<div class="card">' +
    '<div class="row"><label style="color:#e3b341">● GPS (USA)</label><input type="checkbox" id="enable_gps"' + (cfg.enable_gps ? ' checked' : '') + '></div>' +
    '<div class="row"><label style="color:#39c5cf">● Galileo (EU)</label><input type="checkbox" id="enable_galileo"' + (cfg.enable_galileo ? ' checked' : '') + '></div>' +
    '<div class="row"><label style="color:#3fb950">● GLONASS (RU)</label><input type="checkbox" id="enable_glonass"' + (cfg.enable_glonass ? ' checked' : '') + '></div>' +
    '<div class="row"><label style="color:#bc8cff">● BeiDou (CN)</label><input type="checkbox" id="enable_beidou"' + (cfg.enable_beidou ? ' checked' : '') + '></div>' +
    '</div>' +
    '<div class="card">' +
    '<div class="row"><label>Use Phone GPS</label><input type="checkbox" id="use_phone_gps"' + (cfg.use_phone_gps ? ' checked' : '') + '></div>' +
    '<div id="coords_div" style="' + (cfg.use_phone_gps ? 'display:none;' : '') + 'margin-top:10px">' +
    '<label>Home Latitude (°)</label><input type="text" id="custom_lat" value="' + (cfg.custom_lat == null ? '' : cfg.custom_lat) + '">' +
    '<label style="margin-top:8px;display:block">Home Longitude (°)</label><input type="text" id="custom_lon" value="' + (cfg.custom_lon == null ? '' : cfg.custom_lon) + '">' +
    '</div></div>' +
    '<button class="btn" id="save_btn">Save & Sync with Watch</button>' +
    '<script>' +
    'var gpsCheck=document.getElementById("use_phone_gps");' +
    'var cDiv=document.getElementById("coords_div");' +
    'gpsCheck.addEventListener("change",function(){cDiv.style.display=gpsCheck.checked?"none":"block";});' +
    'document.getElementById("save_btn").addEventListener("click",function(){' +
    'var res={' +
    'show_labels:document.getElementById("show_labels").checked,' +
    'enable_gps:document.getElementById("enable_gps").checked,' +
    'enable_galileo:document.getElementById("enable_galileo").checked,' +
    'enable_glonass:document.getElementById("enable_glonass").checked,' +
    'enable_beidou:document.getElementById("enable_beidou").checked,' +
    'use_phone_gps:document.getElementById("use_phone_gps").checked,' +
    'custom_lat:document.getElementById("custom_lat").value,' +
    'custom_lon:document.getElementById("custom_lon").value' +
    '};' +
    'if(!res.use_phone_gps){var lat=Number(res.custom_lat),lon=Number(res.custom_lon);if(res.custom_lat.trim()===""||res.custom_lon.trim()===""||!isFinite(lat)||lat < -90||lat > 90||!isFinite(lon)||lon < -180||lon > 180){alert("Enter latitude -90 to 90 and longitude -180 to 180.");return;}res.custom_lat=lat;res.custom_lon=lon;}' +
    'window.location.href="pebblejs://close#"+encodeURIComponent(JSON.stringify(res));' +
    '});' +
    '</script></body></html>'
  );
  Pebble.openURL(configUrl);
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (e && e.response) {
    try {
      var decoded = JSON.parse(decodeURIComponent(e.response));
      saveConfig(decoded);
      refreshConstellation();
    } catch (err) {
      console.log('Error parsing config response: ' + err);
    }
  }
});

if (typeof module !== "undefined") module.exports = { computeDOP: computeDOP, parseTleCatalog: parseTleCatalog, getLocation: getLocation };
