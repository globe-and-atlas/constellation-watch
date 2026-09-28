#include <pebble.h>
#include "globe.h"
#include "skyplot.h"
#include "geodesy.h"
#include "ground_track.h"
#include "hud.h"
#include "storage.h"

static Window *s_main_window;
static Layer *s_canvas_layer;
static GlobeState s_globe_state;
static char s_status_text[32] = "GNSS CAGE";

// Default orbital planes fixture (used before phone connects)
static void init_default_planes(void) {
  s_globe_state.plane_count = 9;
  
  // 6 GPS Planes (A-F), 55 deg inclination, 60 deg RAAN spacing
  for (int i = 0; i < 6; i++) {
    s_globe_state.planes[i].constellation = CONSTELLATION_GPS;
    s_globe_state.planes[i].inc_deg = 55;
    s_globe_state.planes[i].raan_deg = (i * 60) - 180;
  }

  // 3 Galileo Planes (A-C), 56 deg inclination, 120 deg RAAN spacing
  for (int i = 0; i < 3; i++) {
    s_globe_state.planes[6 + i].constellation = CONSTELLATION_GALILEO;
    s_globe_state.planes[6 + i].inc_deg = 56;
    s_globe_state.planes[6 + i].raan_deg = (i * 120) - 150;
  }
}

static void send_cmd(uint8_t cmd) {
  DictionaryIterator *iter;
  AppMessageResult res = app_message_outbox_begin(&iter);
  if (res == APP_MSG_OK) {
    dict_write_uint8(iter, MESSAGE_KEY_CMD, cmd);
    app_message_outbox_send();
  }
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Layout boundaries
  GRect header_bounds = GRect(0, 0, bounds.size.w, 20);
  GRect footer_bounds = GRect(0, bounds.size.h - 20, bounds.size.w, 20);
  GRect content_bounds = GRect(0, 20, bounds.size.w, bounds.size.h - 40);

  // Background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  const char *header_label = s_status_text;

  // Render the selected instrument pane
  switch (s_globe_state.active_pane) {
    case PANE_ORBIT_CAGE:
      globe_render(ctx, content_bounds, &s_globe_state);
      header_label = s_status_text;
      break;

    case PANE_POLAR_SKYPLOT:
      skyplot_render(ctx, content_bounds, &s_globe_state);
      header_label = "SKYPLOT • 0°/45°/90°";
      break;

    case PANE_GEODESY_DOP:
      geodesy_render(ctx, content_bounds, &s_globe_state);
      header_label = "GEODESY & PRECISION";
      break;

    case PANE_GROUND_TRACK:
      ground_track_render(ctx, content_bounds, &s_globe_state);
      header_label = "GROUND TRACKS 2D";
      break;

    default:
      break;
  }

  // Render Telemetry HUD
  hud_render_header(ctx, header_bounds, header_label);

  char footer_buf[48];
  if (s_globe_state.active_pane == PANE_ORBIT_CAGE && (s_globe_state.rot_yaw_deg != 0 || s_globe_state.rot_pitch_deg != 0)) {
    snprintf(footer_buf, sizeof(footer_buf), "ROT: %+d° • PANE 1/4", s_globe_state.rot_yaw_deg);
  } else {
    snprintf(footer_buf, sizeof(footer_buf), "PANE %d/4 • %s",
             (int)s_globe_state.active_pane + 1,
             (s_globe_state.active_pane == PANE_ORBIT_CAGE) ? "ORBIT CAGE" :
             (s_globe_state.active_pane == PANE_POLAR_SKYPLOT) ? "POLAR RADAR" :
             (s_globe_state.active_pane == PANE_GEODESY_DOP) ? "DOP MATRIX" : "WORLD MAP");
  }
  hud_render_footer(ctx, footer_bounds, &s_globe_state, footer_buf);
}

// Button click handlers
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_globe_state.active_pane == PANE_ORBIT_CAGE) {
    s_globe_state.rot_yaw_deg -= 15;
    if (s_globe_state.rot_yaw_deg < -180) s_globe_state.rot_yaw_deg += 360;
  } else {
    s_globe_state.active_pane = (s_globe_state.active_pane + PANE_COUNT_TOTAL - 1) % PANE_COUNT_TOTAL;
    storage_save_state(&s_globe_state);
  }
  layer_mark_dirty(s_canvas_layer);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_globe_state.active_pane == PANE_ORBIT_CAGE) {
    s_globe_state.rot_yaw_deg += 15;
    if (s_globe_state.rot_yaw_deg > 180) s_globe_state.rot_yaw_deg -= 360;
  } else {
    s_globe_state.active_pane = (s_globe_state.active_pane + 1) % PANE_COUNT_TOTAL;
    storage_save_state(&s_globe_state);
  }
  layer_mark_dirty(s_canvas_layer);
}

static void up_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  s_globe_state.active_pane = (s_globe_state.active_pane + PANE_COUNT_TOTAL - 1) % PANE_COUNT_TOTAL;
  vibes_short_pulse();
  storage_save_state(&s_globe_state);
  layer_mark_dirty(s_canvas_layer);
}

static void down_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  s_globe_state.active_pane = (s_globe_state.active_pane + 1) % PANE_COUNT_TOTAL;
  vibes_short_pulse();
  storage_save_state(&s_globe_state);
  layer_mark_dirty(s_canvas_layer);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Toggle PRN labels
  s_globe_state.show_labels = !s_globe_state.show_labels;
  storage_save_state(&s_globe_state);
  layer_mark_dirty(s_canvas_layer);
}

static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Reset rotation to home center and request fresh sync
  s_globe_state.rot_yaw_deg = 0;
  s_globe_state.rot_pitch_deg = 0;
  snprintf(s_status_text, sizeof(s_status_text), "SYNCING...");
  layer_mark_dirty(s_canvas_layer);
  send_cmd(1); // 1 = REFRESH
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_UP, 500, up_long_click_handler, NULL);
  window_long_click_subscribe(BUTTON_ID_DOWN, 500, down_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 600, select_long_click_handler, NULL);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_canvas_layer);
}

// AppMessage handlers
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *status_tuple = dict_find(iterator, MESSAGE_KEY_STATUS);
  if (status_tuple) {
    snprintf(s_status_text, sizeof(s_status_text), "%s", status_tuple->value->cstring);
  }

  Tuple *labels_tuple = dict_find(iterator, MESSAGE_KEY_SHOW_LABELS);
  if (labels_tuple) {
    s_globe_state.show_labels = (labels_tuple->value->int32 != 0);
  }

  Tuple *lat_tuple = dict_find(iterator, MESSAGE_KEY_CENTER_LAT);
  if (lat_tuple) {
    s_globe_state.center_lat_deg = lat_tuple->value->int32;
  }

  Tuple *lon_tuple = dict_find(iterator, MESSAGE_KEY_CENTER_LON);
  if (lon_tuple) {
    s_globe_state.center_lon_deg = lon_tuple->value->int32;
  }

  Tuple *plane_count_tuple = dict_find(iterator, MESSAGE_KEY_PLANE_COUNT);
  Tuple *plane_data_tuple = dict_find(iterator, MESSAGE_KEY_PLANE_DATA);
  if (plane_count_tuple && plane_data_tuple) {
    uint8_t count = (uint8_t)plane_count_tuple->value->int32;
    if (count > MAX_PLANES) count = MAX_PLANES;
    s_globe_state.plane_count = count;
    memcpy(s_globe_state.planes, plane_data_tuple->value->data, count * sizeof(PlaneRecord));
  }

  Tuple *sat_count_tuple = dict_find(iterator, MESSAGE_KEY_SAT_COUNT);
  Tuple *sat_data_tuple = dict_find(iterator, MESSAGE_KEY_SAT_DATA);
  if (sat_count_tuple && sat_data_tuple) {
    uint8_t count = (uint8_t)sat_count_tuple->value->int32;
    if (count > MAX_SATELLITES) count = MAX_SATELLITES;
    s_globe_state.sat_count = count;
    memcpy(s_globe_state.satellites, sat_data_tuple->value->data, count * sizeof(SatelliteRecord));
  }

  // DOP & Geodesy Telemetry
  Tuple *pdop_tuple = dict_find(iterator, MESSAGE_KEY_PDOP);
  if (pdop_tuple) s_globe_state.metrics.pdop_x10 = (uint16_t)pdop_tuple->value->int32;

  Tuple *hdop_tuple = dict_find(iterator, MESSAGE_KEY_HDOP);
  if (hdop_tuple) s_globe_state.metrics.hdop_x10 = (uint16_t)hdop_tuple->value->int32;

  Tuple *vdop_tuple = dict_find(iterator, MESSAGE_KEY_VDOP);
  if (vdop_tuple) s_globe_state.metrics.vdop_x10 = (uint16_t)vdop_tuple->value->int32;

  Tuple *tdop_tuple = dict_find(iterator, MESSAGE_KEY_TDOP);
  if (tdop_tuple) s_globe_state.metrics.tdop_x10 = (uint16_t)tdop_tuple->value->int32;

  Tuple *gdop_tuple = dict_find(iterator, MESSAGE_KEY_GDOP);
  if (gdop_tuple) s_globe_state.metrics.gdop_x10 = (uint16_t)gdop_tuple->value->int32;

  Tuple *epe_tuple = dict_find(iterator, MESSAGE_KEY_EPE_M);
  if (epe_tuple) s_globe_state.metrics.epe_dm = (uint16_t)epe_tuple->value->int32;

  Tuple *fix_tuple = dict_find(iterator, MESSAGE_KEY_FIX_TYPE);
  if (fix_tuple) s_globe_state.metrics.fix_type = (uint8_t)fix_tuple->value->int32;

  Tuple *leap_tuple = dict_find(iterator, MESSAGE_KEY_GPS_LEAP);
  if (leap_tuple) s_globe_state.metrics.gps_leap_sec = (uint8_t)leap_tuple->value->int32;

  storage_save_state(&s_globe_state);
  layer_mark_dirty(s_canvas_layer);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
}

static void init(void) {
  globe_init();
  skyplot_init();
  geodesy_init();
  ground_track_init();
  hud_init();

  // Try to load cached state; otherwise load initial defaults
  if (!storage_load_state(&s_globe_state)) {
    s_globe_state.active_pane = PANE_ORBIT_CAGE;
    s_globe_state.center_lat_deg = 297604;  // 29.7604 N
    s_globe_state.center_lon_deg = -953698; // -95.3698 W
    s_globe_state.show_labels = true;
    s_globe_state.rot_yaw_deg = 0;
    s_globe_state.rot_pitch_deg = 0;
    s_globe_state.sat_count = 0;
    s_globe_state.metrics.pdop_x10 = 14;
    s_globe_state.metrics.hdop_x10 = 9;
    s_globe_state.metrics.vdop_x10 = 11;
    s_globe_state.metrics.tdop_x10 = 8;
    s_globe_state.metrics.gdop_x10 = 16;
    s_globe_state.metrics.epe_dm = 28; // 2.8m
    s_globe_state.metrics.fix_type = 4; // 3D-DIFF
    s_globe_state.metrics.gps_leap_sec = 18;
    init_default_planes();
  }

  s_main_window = window_create();
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  window_stack_push(s_main_window, true);

  // AppMessage registration with large buffer for packed constellation arrays
  app_message_register_inbox_received(inbox_received_callback);
  app_message_open(1024, 128);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  app_message_deregister_callbacks();
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
