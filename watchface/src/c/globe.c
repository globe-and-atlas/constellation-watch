#include "globe.h"
#include "earth_land.h"

static GFont s_micro_font;

void globe_init(void) {
  s_micro_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
}

GColor globe_constellation_color(uint8_t constellation) {
  switch (constellation) {
    case CONSTELLATION_GPS:
      return GColorChromeYellow;    // Golden Amber for GPS
    case CONSTELLATION_GALILEO:
      return GColorElectricBlue;    // Cyan / Electric Blue for Galileo
    case CONSTELLATION_GLONASS:
      return GColorMalachite;       // Green for GLONASS
    case CONSTELLATION_BEIDOU:
      return GColorMagenta;         // Magenta for BeiDou
    default:
      return GColorLightGray;
  }
}

// 3D vector rotation using Pebble fixed-point trig with 64-bit arithmetic to prevent overflow
static void project_3d_point(
    int32_t x, int32_t y, int32_t z,
    int32_t cos_a, int32_t sin_a,
    int32_t cos_b, int32_t sin_b,
    int32_t *out_xp, int32_t *out_yp, int32_t *out_zp) {
  
  // Rotate around Z axis by angle alpha
  int32_t x1 = (int32_t)(((int64_t)x * cos_a - (int64_t)y * sin_a) / TRIG_MAX_RATIO);
  int32_t y1 = (int32_t)(((int64_t)x * sin_a + (int64_t)y * cos_a) / TRIG_MAX_RATIO);
  int32_t z1 = z;

  // Rotate around Y axis by angle beta (tilt latitude)
  *out_xp = y1;
  *out_yp = (int32_t)((-(int64_t)x1 * sin_b + (int64_t)z1 * cos_b) / TRIG_MAX_RATIO);
  *out_zp = (int32_t)(( (int64_t)x1 * cos_b + (int64_t)z1 * sin_b) / TRIG_MAX_RATIO);
}

void globe_render(GContext *ctx, GRect bounds, GlobeState *state) {
  int16_t cx = bounds.origin.x + (bounds.size.w / 2);
  int16_t cy = bounds.origin.y + (bounds.size.h / 2);

  const int32_t r_earth = 25;       // Earth disk radius in pixels
  const int32_t r_orbit = 76;       // MEO orbit ring radius in pixels

  // Compute view rotation angles
  int16_t center_lat = (int16_t)(state->center_lat_deg / 10000);
  int16_t center_lon = (int16_t)(state->center_lon_deg / 10000);

  // Combine with interactive rotation offsets
  int16_t total_yaw = -center_lon + state->rot_yaw_deg;
  int16_t total_pitch = center_lat + state->rot_pitch_deg;

  int32_t angle_a = DEG_TO_TRIGANGLE(total_yaw);
  int32_t angle_b = DEG_TO_TRIGANGLE(total_pitch);

  int32_t cos_a = cos_lookup(angle_a);
  int32_t sin_a = sin_lookup(angle_a);
  int32_t cos_b = cos_lookup(angle_b);
  int32_t sin_b = sin_lookup(angle_b);

  // 1. Draw Earth Globe (Deep blue ocean base)
  graphics_context_set_fill_color(ctx, GColorCobaltBlue);
  graphics_fill_circle(ctx, GPoint(cx, cy), r_earth);

  // Atmosphere halo ring
  graphics_context_set_stroke_color(ctx, GColorPictonBlue);
  graphics_draw_circle(ctx, GPoint(cx, cy), r_earth);

  // 2. Draw Coastlines & Continents
  graphics_context_set_stroke_color(ctx, GColorMintGreen);
  GPoint prev_pt = GPoint(0, 0);
  bool prev_visible = false;

  for (uint16_t i = 0; i < s_earth_coastline_count; i++) {
    int16_t v_lat = s_earth_coastlines[i].lat;
    int16_t v_lon = s_earth_coastlines[i].lon;
    uint8_t pen_down = s_earth_coastlines[i].pen_down;

    int32_t lat_trig = DEG_TO_TRIGANGLE(v_lat);
    int32_t lon_trig = DEG_TO_TRIGANGLE(v_lon);

    int32_t cos_lat = cos_lookup(lat_trig);
    int32_t sin_lat = sin_lookup(lat_trig);
    int32_t cos_lon = cos_lookup(lon_trig);
    int32_t sin_lon = sin_lookup(lon_trig);

    // ECEF coordinates on unit sphere * TRIG_MAX_RATIO
    int32_t ex = (int32_t)(((int64_t)cos_lat * cos_lon) / TRIG_MAX_RATIO);
    int32_t ey = (int32_t)(((int64_t)cos_lat * sin_lon) / TRIG_MAX_RATIO);
    int32_t ez = sin_lat;

    int32_t xp, yp, zp;
    project_3d_point(ex, ey, ez, cos_a, sin_a, cos_b, sin_b, &xp, &yp, &zp);

    if (zp >= 0) {
      int16_t sx = cx + (int16_t)(((int64_t)xp * r_earth) / TRIG_MAX_RATIO);
      int16_t sy = cy - (int16_t)(((int64_t)yp * r_earth) / TRIG_MAX_RATIO);
      GPoint curr_pt = GPoint(sx, sy);

      if (pen_down && prev_visible) {
        graphics_draw_line(ctx, prev_pt, curr_pt);
      }
      prev_pt = curr_pt;
      prev_visible = true;
    } else {
      prev_visible = false;
    }
  }

  // 3. Draw Observer "YOU" Beacon only after valid coordinates arrive.
  if (state->has_location) {
    int32_t u_lat_trig = DEG_TO_TRIGANGLE(center_lat);
    int32_t u_lon_trig = DEG_TO_TRIGANGLE(center_lon);
    int32_t ux = (int32_t)(((int64_t)cos_lookup(u_lat_trig) * cos_lookup(u_lon_trig)) / TRIG_MAX_RATIO);
    int32_t uy = (int32_t)(((int64_t)cos_lookup(u_lat_trig) * sin_lookup(u_lon_trig)) / TRIG_MAX_RATIO);
    int32_t uz = sin_lookup(u_lat_trig);

    int32_t u_xp, u_yp, u_zp;
    project_3d_point(ux, uy, uz, cos_a, sin_a, cos_b, sin_b, &u_xp, &u_yp, &u_zp);

    if (u_zp >= 0) {
      int16_t usx = cx + (int16_t)(((int64_t)u_xp * r_earth) / TRIG_MAX_RATIO);
      int16_t usy = cy - (int16_t)(((int64_t)u_yp * r_earth) / TRIG_MAX_RATIO);
      graphics_context_set_fill_color(ctx, GColorChromeYellow);
      graphics_fill_circle(ctx, GPoint(usx, usy), 2);
    }
  }

  // 4. Draw 3D Orbital Plane Rings (Smooth 48-step circular projection)
  const int NUM_RING_STEPS = 48;
  // Plane rings are a schematic orientation guide, not fitted to the displayed TLEs.
  for (uint8_t p = 0; p < state->plane_count; p++) {
    PlaneRecord *plane = &state->planes[p];
    GColor plane_color = globe_constellation_color(plane->constellation);
    graphics_context_set_stroke_color(ctx, plane_color);

    int32_t inc_trig = DEG_TO_TRIGANGLE(plane->inc_deg);
    int32_t raan_trig = DEG_TO_TRIGANGLE(plane->raan_deg);

    int32_t cos_inc = cos_lookup(inc_trig);
    int32_t sin_inc = sin_lookup(inc_trig);
    int32_t cos_raan = cos_lookup(raan_trig);
    int32_t sin_raan = sin_lookup(raan_trig);

    GPoint ring_prev = GPoint(0, 0);
    bool ring_prev_valid = false;

    for (int step = 0; step <= NUM_RING_STEPS; step++) {
      int16_t theta_deg = (step * 360) / NUM_RING_STEPS;
      int32_t theta_trig = DEG_TO_TRIGANGLE(theta_deg);

      int32_t cos_th = cos_lookup(theta_trig);
      int32_t sin_th = sin_lookup(theta_trig);

      // Parametric orbit plane circle using int64_t to prevent 32-bit overflow
      int32_t term1 = (int32_t)(((int64_t)cos_raan * cos_th) / TRIG_MAX_RATIO);
      int32_t term2 = (int32_t)(((((int64_t)sin_raan * sin_th) / TRIG_MAX_RATIO) * cos_inc) / TRIG_MAX_RATIO);
      int32_t ox = term1 - term2;

      int32_t term3 = (int32_t)(((int64_t)sin_raan * cos_th) / TRIG_MAX_RATIO);
      int32_t term4 = (int32_t)(((((int64_t)cos_raan * sin_th) / TRIG_MAX_RATIO) * cos_inc) / TRIG_MAX_RATIO);
      int32_t oy = term3 + term4;

      int32_t oz = (int32_t)(((int64_t)sin_th * sin_inc) / TRIG_MAX_RATIO);

      int32_t oxp, oyp, ozp;
      project_3d_point(ox, oy, oz, cos_a, sin_a, cos_b, sin_b, &oxp, &oyp, &ozp);

      int16_t osx = cx + (int16_t)(((int64_t)oxp * r_orbit) / TRIG_MAX_RATIO);
      int16_t osy = cy - (int16_t)(((int64_t)oyp * r_orbit) / TRIG_MAX_RATIO);
      GPoint ring_curr = GPoint(osx, osy);

      // Check if occluded directly behind solid Earth
      int32_t dist_x = (int32_t)(((int64_t)oxp * r_orbit) / TRIG_MAX_RATIO);
      int32_t dist_y = (int32_t)(((int64_t)oyp * r_orbit) / TRIG_MAX_RATIO);
      bool occluded_by_earth = (ozp < 0) &&
          ((dist_x * dist_x + dist_y * dist_y) < (r_earth * r_earth));

      if (!occluded_by_earth) {
        if (ring_prev_valid) {
          graphics_draw_line(ctx, ring_prev, ring_curr);
        }
        ring_prev = ring_curr;
        ring_prev_valid = true;
      } else {
        ring_prev_valid = false;
      }
    }
  }

  graphics_context_set_text_color(ctx, GColorLightGray);
  graphics_draw_text(ctx, "SCHEMATIC ORBITS", s_micro_font,
                     GRect(bounds.origin.x + 4, bounds.origin.y + bounds.size.h - 15, bounds.size.w - 8, 15),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  // 5. Draw TLE-propagated satellite positions & optional catalog labels
  for (uint8_t s = 0; s < state->sat_count; s++) {
    SatelliteRecord *sat = &state->satellites[s];
    GColor sat_color = globe_constellation_color(sat->constellation);

    // sat->x, y, z are normalized * 100
    int32_t sx_in = sat->x;
    int32_t sy_in = sat->y;
    int32_t sz_in = sat->z;

    int32_t sxp, syp, szp;
    project_3d_point(sx_in, sy_in, sz_in, cos_a, sin_a, cos_b, sin_b, &sxp, &syp, &szp);

    // Scaling: r_orbit is ~4.16x Earth radius
    int16_t ssx = cx + (int16_t)(((int64_t)sxp * r_orbit) / 416);
    int16_t ssy = cy - (int16_t)(((int64_t)syp * r_orbit) / 416);

    // Eclipse check: behind Earth disk
    int32_t s_dist_x = (int32_t)(((int64_t)sxp * r_orbit) / 416);
    int32_t s_dist_y = (int32_t)(((int64_t)syp * r_orbit) / 416);
    bool is_eclipsed = (szp < -200) &&
        ((s_dist_x * s_dist_x + s_dist_y * s_dist_y) < (r_earth * r_earth));

    if (is_eclipsed) continue;

    bool above_horizon = (sat->flags & 0x01) != 0;

    if (above_horizon) {
      // In direct line of sight: bright node with outer halo
      graphics_context_set_fill_color(ctx, GColorWhite);
      graphics_fill_rect(ctx, GRect(ssx - 1, ssy - 1, 3, 3), 0, GCornerNone);
      graphics_context_set_stroke_color(ctx, sat_color);
      graphics_draw_circle(ctx, GPoint(ssx, ssy), 3);
    } else {
      // Below horizon: muted 2x2 dot
      graphics_context_set_fill_color(ctx, sat_color);
      graphics_fill_rect(ctx, GRect(ssx - 1, ssy - 1, 2, 2), 0, GCornerNone);
    }

    // Optional TLE NORAD catalog number (not a GNSS PRN)
    if (state->show_labels && above_horizon) {
      char label_buf[8];
      char prefix = 'S';
      if (sat->constellation == CONSTELLATION_GPS) prefix = 'G';
      else if (sat->constellation == CONSTELLATION_GALILEO) prefix = 'E';
      else if (sat->constellation == CONSTELLATION_GLONASS) prefix = 'R';
      else if (sat->constellation == CONSTELLATION_BEIDOU) prefix = 'C';

      snprintf(label_buf, sizeof(label_buf), "%c%05u", prefix, sat->catalog_id);

      graphics_context_set_text_color(ctx, sat_color);
      graphics_draw_text(ctx, label_buf, s_micro_font,
                         GRect(ssx + 4, ssy - 8, 42, 16),
                         GTextOverflowModeTrailingEllipsis,
                         GTextAlignmentLeft, NULL);
    }
  }
}
