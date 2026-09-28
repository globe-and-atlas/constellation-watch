#include "skyplot.h"

static GFont s_micro_font;
static GFont s_label_font;

void skyplot_init(void) {
  s_micro_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  s_label_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
}

void skyplot_render(GContext *ctx, GRect bounds, GlobeState *state) {
  int16_t cx = bounds.origin.x + (bounds.size.w / 2);
  int16_t cy = bounds.origin.y + 72; // Center of radar circle
  const int16_t r_radar = 64;       // 0° horizon radius
  const int16_t r_mid = 32;         // 45° elevation radius

  // 1. Draw Polar Radar Grid
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_draw_circle(ctx, GPoint(cx, cy), r_radar); // 0° Horizon
  graphics_draw_circle(ctx, GPoint(cx, cy), r_mid);   // 45° Elevation

  // Crosshairs
  graphics_draw_line(ctx, GPoint(cx - r_radar, cy), GPoint(cx + r_radar, cy));
  graphics_draw_line(ctx, GPoint(cx, cy - r_radar), GPoint(cx, cy + r_radar));

  // Cardinal labels
  graphics_context_set_text_color(ctx, GColorChromeYellow);
  graphics_draw_text(ctx, "N", s_label_font, GRect(cx - 6, cy - r_radar - 15, 12, 14),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  graphics_draw_text(ctx, "S", s_label_font, GRect(cx - 6, cy + r_radar + 1, 12, 14),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  graphics_draw_text(ctx, "W", s_label_font, GRect(cx - r_radar - 14, cy - 8, 12, 14),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  graphics_draw_text(ctx, "E", s_label_font, GRect(cx + r_radar + 3, cy - 8, 12, 14),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  // 2. Plot positions above the geometric horizon, not receiver-tracked signals
  uint8_t vis_sats[16];
  uint8_t vis_count = 0;

  for (uint8_t i = 0; i < state->sat_count; i++) {
    SatelliteRecord *sat = &state->satellites[i];
    if (sat->el_deg <= 0) continue; // Below horizon

    if (vis_count < 16) {
      vis_sats[vis_count++] = i;
    }

    // Polar coordinates
    int16_t az_deg = sat->az_deg_div_2 * 2;
    int16_t el_deg = sat->el_deg;

    // Radius from zenith: r = r_radar * (90 - el) / 90
    int32_t dist = (r_radar * (90 - el_deg)) / 90;

    int32_t az_trig = DEG_TO_TRIGANGLE(az_deg);
    int16_t sx = cx + (int16_t)((dist * sin_lookup(az_trig)) / TRIG_MAX_RATIO);
    int16_t sy = cy - (int16_t)((dist * cos_lookup(az_trig)) / TRIG_MAX_RATIO);

    GColor col = globe_constellation_color(sat->constellation);
    bool above_geometry_mask = (sat->flags & 0x02) != 0;

    // Draw a symbol for elevation above/below the 15-degree geometry mask
    GRect badge_rect = GRect(sx - 5, sy - 5, 11, 11);
    if (above_geometry_mask) {
      graphics_context_set_fill_color(ctx, col);
      graphics_fill_rect(ctx, badge_rect, 1, GCornersAll);
      graphics_context_set_text_color(ctx, GColorBlack);
    } else {
      graphics_context_set_stroke_color(ctx, col);
      graphics_draw_rect(ctx, badge_rect);
      graphics_context_set_text_color(ctx, col);
    }

  }

  // 3. Bottom geometric elevation bars (not signal strength)
  int16_t bar_y_base = bounds.origin.y + bounds.size.h - 18;
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_draw_text(ctx, "ELEVATION", s_micro_font, GRect(4, bar_y_base + 1, 82, 14), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  graphics_draw_line(ctx, GPoint(4, bar_y_base), GPoint(bounds.size.w - 4, bar_y_base));

  uint8_t bars_to_show = (vis_count > 10) ? 10 : vis_count;
  for (uint8_t b = 0; b < bars_to_show; b++) {
    SatelliteRecord *sat = &state->satellites[vis_sats[b]];
    int16_t bar_x = 8 + (b * 12);
    int16_t bar_h = (sat->el_deg * 22) / 90;
    if (bar_h < 2) bar_h = 2;

    GColor bar_col = globe_constellation_color(sat->constellation);
    graphics_context_set_fill_color(ctx, bar_col);
    graphics_fill_rect(ctx, GRect(bar_x, bar_y_base - bar_h, 8, bar_h), 0, GCornerNone);

  }

  // Right side DOP mini readout
  char dop_summary[32];
  if (state->metrics.dop_valid) {
    snprintf(dop_summary, sizeof(dop_summary), "G-DOP %d.%d", state->metrics.gdop_x10 / 10, state->metrics.gdop_x10 % 10);
  } else {
    snprintf(dop_summary, sizeof(dop_summary), "G-DOP N/A");
  }
  graphics_context_set_text_color(ctx, GColorChromeYellow);
  graphics_draw_text(ctx, dop_summary, s_micro_font,
                     GRect(bounds.size.w - 62, bar_y_base - 26, 60, 26),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
}
