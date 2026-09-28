#include "ground_track.h"
#include "earth_land.h"

static GFont s_micro_font;
static GFont s_bold_font;

void ground_track_init(void) {
  s_micro_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  s_bold_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
}

// Convert Lat (-90..+90) and Lon (-180..+180) to screen pixel coordinates
static GPoint coord_to_screen(int16_t lat, int16_t lon, GRect map_rect) {
  if (lon < -180) lon += 360;
  if (lon > 180) lon -= 360;
  if (lat < -90) lat = -90;
  if (lat > 90) lat = 90;

  int16_t sx = map_rect.origin.x + (int16_t)(((int32_t)(lon + 180) * map_rect.size.w) / 360);
  int16_t sy = map_rect.origin.y + (int16_t)(((int32_t)(90 - lat) * map_rect.size.h) / 180);
  return GPoint(sx, sy);
}

void ground_track_render(GContext *ctx, GRect bounds, GlobeState *state) {
  // 2:1 Equirectangular Map Frame (192 x 96 px)
  GRect map_rect = GRect(bounds.origin.x + 4, bounds.origin.y + 12, bounds.size.w - 8, 96);

  // Background ocean
  graphics_context_set_fill_color(ctx, GColorOxfordBlue);
  graphics_fill_rect(ctx, map_rect, 0, GCornerNone);

  // Equator & Prime Meridian Grid
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  int16_t eq_y = map_rect.origin.y + (map_rect.size.h / 2);
  int16_t pm_x = map_rect.origin.x + (map_rect.size.w / 2);
  graphics_draw_line(ctx, GPoint(map_rect.origin.x, eq_y), GPoint(map_rect.origin.x + map_rect.size.w - 1, eq_y));
  graphics_draw_line(ctx, GPoint(pm_x, map_rect.origin.y), GPoint(pm_x, map_rect.origin.y + map_rect.size.h - 1));

  // Map Border
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_rect(ctx, map_rect);

  // 1. Draw 2D Continental Coastlines
  graphics_context_set_stroke_color(ctx, GColorMintGreen);
  GPoint prev_pt = GPoint(0, 0);
  int16_t prev_lon = 0;

  for (uint16_t i = 0; i < s_earth_coastline_count; i++) {
    int16_t lat = s_earth_coastlines[i].lat;
    int16_t lon = s_earth_coastlines[i].lon;
    uint8_t pen_down = s_earth_coastlines[i].pen_down;

    GPoint pt = coord_to_screen(lat, lon, map_rect);

    // Prevent antimeridian wrap-around lines
    bool wrap = (i > 0) && (abs(lon - prev_lon) > 180);

    if (pen_down && !wrap) {
      graphics_draw_line(ctx, prev_pt, pt);
    }
    prev_pt = pt;
    prev_lon = lon;
  }

  // 2. Draw Observer "YOU" Crosshair & Horizon Footprint
  int16_t u_lat = (int16_t)(state->center_lat_deg / 10000);
  int16_t u_lon = (int16_t)(state->center_lon_deg / 10000);
  GPoint u_pt = coord_to_screen(u_lat, u_lon, map_rect);

  // Only the observer location is plotted; no approximate visibility footprint is implied.
  if (state->has_location) {
    graphics_context_set_stroke_color(ctx, GColorChromeYellow);
    graphics_draw_line(ctx, GPoint(u_pt.x - 3, u_pt.y), GPoint(u_pt.x + 3, u_pt.y));
    graphics_draw_line(ctx, GPoint(u_pt.x, u_pt.y - 3), GPoint(u_pt.x, u_pt.y + 3));
  }

  // 3. Draw Sub-Satellite Ground Points
  for (uint8_t s = 0; s < state->sat_count; s++) {
    SatelliteRecord *sat = &state->satellites[s];
    int16_t sat_lat = sat->lat_deg;
    int16_t sat_lon = sat->lon_deg_div_2 * 2;

    GPoint sat_pt = coord_to_screen(sat_lat, sat_lon, map_rect);
    GColor sat_col = globe_constellation_color(sat->constellation);

    bool in_view = (sat->flags & 0x01) != 0;
    if (in_view) {
      graphics_context_set_fill_color(ctx, GColorWhite);
      graphics_fill_rect(ctx, GRect(sat_pt.x - 1, sat_pt.y - 1, 3, 3), 0, GCornerNone);
      graphics_context_set_stroke_color(ctx, sat_col);
      graphics_draw_circle(ctx, sat_pt, 3);
    } else {
      graphics_context_set_fill_color(ctx, sat_col);
      graphics_fill_rect(ctx, GRect(sat_pt.x - 1, sat_pt.y - 1, 2, 2), 0, GCornerNone);
    }
  }

  // 4. Telemetry Footer Details
  int16_t y_details = map_rect.origin.y + map_rect.size.h + 8;
  graphics_context_set_text_color(ctx, GColorChromeYellow);
  graphics_draw_text(ctx, "SUB-SATELLITE POINTS", s_bold_font,
                     GRect(map_rect.origin.x, y_details, map_rect.size.w, 15),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  char coord_str[48];
  if (state->has_location) {
    snprintf(coord_str, sizeof(coord_str), "CENTER: %+d.%02d°, %+d.%02d° (YOU)",
             u_lat, abs((int16_t)(state->center_lat_deg % 10000) / 100),
             u_lon, abs((int16_t)(state->center_lon_deg % 10000) / 100));
  } else {
    snprintf(coord_str, sizeof(coord_str), "LOCATION UNAVAILABLE • NO CENTER");
  }
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, coord_str, s_micro_font,
                     GRect(map_rect.origin.x, y_details + 16, map_rect.size.w, 15),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  char leg_str[48];
  snprintf(leg_str, sizeof(leg_str), "MEO CONSTELLATION CAGE • %d SVs", state->sat_count);
  graphics_context_set_text_color(ctx, GColorElectricBlue);
  graphics_draw_text(ctx, leg_str, s_micro_font,
                     GRect(map_rect.origin.x, y_details + 31, map_rect.size.w, 15),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}
