#include "hud.h"

static GFont s_hud_font_bold;
static GFont s_hud_font_small;

void hud_init(void) {
  s_hud_font_bold = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  s_hud_font_small = fonts_get_system_font(FONT_KEY_GOTHIC_14);
}

void hud_render_header(GContext *ctx, GRect bounds, const char *status_str) {
  // Top bar background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Border line
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_draw_line(ctx, GPoint(bounds.origin.x, bounds.origin.y + bounds.size.h - 1),
                          GPoint(bounds.origin.x + bounds.size.w, bounds.origin.y + bounds.size.h - 1));

  // Current time
  char time_buf[16];
  clock_copy_time_string(time_buf, sizeof(time_buf));

  // Left status string
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, status_str ? status_str : "GNSS CAGE",
                     s_hud_font_bold,
                     GRect(bounds.origin.x + 4, bounds.origin.y + 2, bounds.size.w - 55, bounds.size.h - 4),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);

  // Right local time
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, time_buf,
                     s_hud_font_bold,
                     GRect(bounds.origin.x + bounds.size.w - 52, bounds.origin.y + 2, 48, bounds.size.h - 4),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentRight, NULL);
}

void hud_render_footer(GContext *ctx, GRect bounds, GlobeState *state, const char *footer_str) {
  // Bottom bar background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Border line
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_draw_line(ctx, GPoint(bounds.origin.x, bounds.origin.y),
                          GPoint(bounds.origin.x + bounds.size.w, bounds.origin.y));

  char display_buf[64];
  if (footer_str && footer_str[0] != '\0') {
    snprintf(display_buf, sizeof(display_buf), "%s", footer_str);
  } else if (state->rot_yaw_deg != 0 || state->rot_pitch_deg != 0) {
    snprintf(display_buf, sizeof(display_buf), "ROT: YAW %+d° • PITCH %+d°",
             state->rot_yaw_deg, state->rot_pitch_deg);
  } else {
    // Count satellites by constellation
    uint8_t gps_vis = 0, gal_vis = 0, other_vis = 0;
    for (uint8_t i = 0; i < state->sat_count; i++) {
      if (state->satellites[i].flags & 0x01) {
        if (state->satellites[i].constellation == CONSTELLATION_GPS) gps_vis++;
        else if (state->satellites[i].constellation == CONSTELLATION_GALILEO) gal_vis++;
        else other_vis++;
      }
    }
    snprintf(display_buf, sizeof(display_buf), "GPS: %d • GAL: %d%s",
             gps_vis, gal_vis, state->show_labels ? " • [LBL]" : "");
  }

  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, display_buf,
                     s_hud_font_bold,
                     GRect(bounds.origin.x + 4, bounds.origin.y + 2, bounds.size.w - 8, bounds.size.h - 4),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);
}
