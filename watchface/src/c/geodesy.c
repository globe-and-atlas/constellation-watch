#include "geodesy.h"

static GFont s_bold_font;
static GFont s_med_font;

void geodesy_init(void) {
  s_bold_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  s_med_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
}

void geodesy_render(GContext *ctx, GRect bounds, GlobeState *state) {
  int16_t x0 = bounds.origin.x + 6;
  int16_t y0 = bounds.origin.y + 4;
  int16_t w = bounds.size.w - 12;

  // Background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Section 1 Header: DILUTION OF PRECISION
  graphics_context_set_text_color(ctx, GColorChromeYellow);
  graphics_draw_text(ctx, "GEODESY & DILUTION OF PRECISION", s_bold_font,
                     GRect(x0, y0, w, 16), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_draw_line(ctx, GPoint(x0, y0 + 17), GPoint(x0 + w, y0 + 17));

  // DOP Matrix Rows
  char row1[48], row2[48], row3[48];
  snprintf(row1, sizeof(row1), "PDOP: %d.%d (3D)      HDOP: %d.%d (H)",
           state->metrics.pdop_x10 / 10, state->metrics.pdop_x10 % 10,
           state->metrics.hdop_x10 / 10, state->metrics.hdop_x10 % 10);
  snprintf(row2, sizeof(row2), "VDOP: %d.%d (Vert)    TDOP: %d.%d (Time)",
           state->metrics.vdop_x10 / 10, state->metrics.vdop_x10 % 10,
           state->metrics.tdop_x10 / 10, state->metrics.tdop_x10 % 10);
  snprintf(row3, sizeof(row3), "GDOP: %d.%d (Geom)    EPE:  ±%d.%dm",
           state->metrics.gdop_x10 / 10, state->metrics.gdop_x10 % 10,
           state->metrics.epe_dm / 10, state->metrics.epe_dm % 10);

  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, row1, s_med_font, GRect(x0, y0 + 20, w, 15), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, row2, s_med_font, GRect(x0, y0 + 35, w, 15), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, row3, s_med_font, GRect(x0, y0 + 50, w, 15), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // Section 2 Header: CONSTELLATIONS IN VIEW
  int16_t y_sec2 = y0 + 72;
  graphics_context_set_text_color(ctx, GColorElectricBlue);
  graphics_draw_text(ctx, "MULTI-GNSS SATELLITE TALLY", s_bold_font,
                     GRect(x0, y_sec2, w, 16), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  graphics_draw_line(ctx, GPoint(x0, y_sec2 + 17), GPoint(x0 + w, y_sec2 + 17));

  // Tally counts
  uint8_t gps_trk = 0, gps_fix = 0;
  uint8_t gal_trk = 0, gal_fix = 0;
  uint8_t glo_trk = 0, glo_fix = 0;
  uint8_t bds_trk = 0, bds_fix = 0;

  for (uint8_t i = 0; i < state->sat_count; i++) {
    SatelliteRecord *sat = &state->satellites[i];
    bool in_view = (sat->flags & 0x01) != 0;
    bool in_fix = (sat->flags & 0x02) != 0;

    if (sat->constellation == CONSTELLATION_GPS) {
      if (in_view) gps_trk++;
      if (in_fix) gps_fix++;
    } else if (sat->constellation == CONSTELLATION_GALILEO) {
      if (in_view) gal_trk++;
      if (in_fix) gal_fix++;
    } else if (sat->constellation == CONSTELLATION_GLONASS) {
      if (in_view) glo_trk++;
      if (in_fix) glo_fix++;
    } else if (sat->constellation == CONSTELLATION_BEIDOU) {
      if (in_view) bds_trk++;
      if (in_fix) bds_fix++;
    }
  }

  char c_gps[40], c_gal[40], c_glo[40], c_bds[40];
  snprintf(c_gps, sizeof(c_gps), "● GPS: %2d TRK • %2d FIX (USA/NAV)", gps_trk, gps_fix);
  snprintf(c_gal, sizeof(c_gal), "● GAL: %2d TRK • %2d FIX (EUR/GAL)", gal_trk, gal_fix);
  snprintf(c_glo, sizeof(c_glo), "● GLO: %2d TRK • %2d FIX (RUS/GLO)", glo_trk, glo_fix);
  snprintf(c_bds, sizeof(c_bds), "● BDS: %2d TRK • %2d FIX (CHN/BDS)", bds_trk, bds_fix);

  graphics_context_set_text_color(ctx, GColorChromeYellow);
  graphics_draw_text(ctx, c_gps, s_med_font, GRect(x0, y_sec2 + 20, w, 15), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  graphics_context_set_text_color(ctx, GColorElectricBlue);
  graphics_draw_text(ctx, c_gal, s_med_font, GRect(x0, y_sec2 + 35, w, 15), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  graphics_context_set_text_color(ctx, GColorMalachite);
  graphics_draw_text(ctx, c_glo, s_med_font, GRect(x0, y_sec2 + 50, w, 15), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  graphics_context_set_text_color(ctx, GColorMagenta);
  graphics_draw_text(ctx, c_bds, s_med_font, GRect(x0, y_sec2 + 65, w, 15), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // Section 3: CLOCK & COORD TELEMETRY
  int16_t y_sec3 = y_sec2 + 86;
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_draw_line(ctx, GPoint(x0, y_sec3), GPoint(x0 + w, y_sec3));

  char clock_buf[48];
  snprintf(clock_buf, sizeof(clock_buf), "GPS-UTC: +%ds   FIX: %s",
           state->metrics.gps_leap_sec ? state->metrics.gps_leap_sec : 18,
           (state->metrics.fix_type == 4) ? "3D-DIFF" : (state->metrics.fix_type == 3) ? "3D-FIX" : "2D-FIX");
  graphics_context_set_text_color(ctx, GColorLightGray);
  graphics_draw_text(ctx, clock_buf, s_med_font, GRect(x0, y_sec3 + 2, w, 15), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}
