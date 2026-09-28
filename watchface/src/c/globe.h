#pragma once
#include <pebble.h>

#define MAX_SATELLITES 48
#define MAX_PLANES 16

typedef enum {
  CONSTELLATION_NONE = 0,
  CONSTELLATION_GPS = 1,
  CONSTELLATION_GALILEO = 2,
  CONSTELLATION_GLONASS = 3,
  CONSTELLATION_BEIDOU = 4
} ConstellationType;

typedef enum {
  PANE_ORBIT_CAGE = 0,
  PANE_POLAR_SKYPLOT = 1,
  PANE_GEODESY_DOP = 2,
  PANE_GROUND_TRACK = 3,
  PANE_COUNT_TOTAL = 4
} InstrumentPane;

typedef struct __attribute__((packed)) {
  uint8_t constellation;  // 1=GPS, 2=Galileo, 3=GLONASS, 4=BeiDou
  uint16_t catalog_id;    // NORAD catalog number from TLE line 1
  int16_t x;              // Normalized ECEF * 100
  int16_t y;
  int16_t z;
  int8_t el_deg;          // Elevation in degrees (-90 to +90)
  uint8_t az_deg_div_2;   // Azimuth in degrees / 2 (0 to 180)
  int8_t lat_deg;         // Sub-satellite latitude (-90 to +90)
  int8_t lon_deg_div_2;   // Sub-satellite longitude / 2 (-90 to +90)
  uint8_t flags;          // bit 0: above geometric horizon, bit 1: above 15-degree mask
} SatelliteRecord;

typedef struct __attribute__((packed)) {
  uint8_t constellation;  // 1=GPS, 2=Galileo, 3=GLONASS, 4=BeiDou
  uint8_t inc_deg;        // 55, 56, 65 deg
  int16_t raan_deg;       // -180 to 180 or 0 to 360 deg
} PlaneRecord;

typedef struct {
  uint16_t pdop_x10;
  uint16_t hdop_x10;
  uint16_t vdop_x10;
  uint16_t tdop_x10;
  uint16_t gdop_x10;
  bool dop_valid;         // Geometry-only DOP is defined only for nonsingular 4+ satellite geometry
} GeodesyMetrics;

typedef struct {
  InstrumentPane active_pane; // Current view pane (0..3)
  int32_t center_lat_deg;     // User center latitude * 10000
  int32_t center_lon_deg;     // User center longitude * 10000
  bool has_location;          // True only after receiving validated phone/custom coordinates
  int16_t rot_yaw_deg;        // Interactive manual yaw offset (-180 to +180)
  int16_t rot_pitch_deg;      // Interactive manual pitch offset (-90 to +90)
  bool show_labels;           // Toggle NORAD catalog-number labels
  uint8_t sat_count;
  SatelliteRecord satellites[MAX_SATELLITES];
  uint8_t plane_count;
  uint16_t tle_age_hours;
  PlaneRecord planes[MAX_PLANES];
  GeodesyMetrics metrics;
} GlobeState;

void globe_init(void);
void globe_render(GContext *ctx, GRect bounds, GlobeState *state);
GColor globe_constellation_color(uint8_t constellation);
