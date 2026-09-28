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
static char s_status_text[32] = "WAITING FOR PHONE";

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
      header_label = "GEOMETRY DOP";
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
  // Toggle NORAD catalog-number labels
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
    if (strcmp(s_status_text, "LOCATION UNAVAILABLE") == 0) { s_globe_state.has_location = false; s_globe_state.sat_count = 0; s_globe_state.plane_count = 0; s_globe_state.metrics.dop_valid = false; }
  }

  Tuple *labels_tuple = dict_find(iterator, MESSAGE_KEY_SHOW_LABELS);
  if (labels_tuple) {
    s_globe_state.show_labels = (labels_tuple->value->int32 != 0);
  }

  Tuple *lat_tuple = dict_find(iterator, MESSAGE_KEY_CENTER_LAT);
  Tuple *lon_tuple = dict_find(iterator, MESSAGE_KEY_CENTER_LON);
  if (lat_tuple && lon_tuple) {
    s_globe_state.center_lat_deg = lat_tuple->value->int32;
    s_globe_state.center_lon_deg = lon_tuple->value->int32;
    s_globe_state.has_location = true;
  }

  Tuple *plane_count_tuple = dict_find(iterator, MESSAGE_KEY_PLANE_COUNT);
  Tuple *plane_data_tuple = dict_find(iterator, MESSAGE_KEY_PLANE_DATA);
  if (plane_count_tuple && plane_data_tuple) {
    uint8_t count = (uint8_t)plane_count_tuple->value->int32;
    if (count > MAX_PLANES) count = MAX_PLANES;
    if (plane_data_tuple->length >= count * sizeof(PlaneRecord)) {
      s_globe_state.plane_count = count;
      memcpy(s_globe_state.planes, plane_data_tuple->value->data, count * sizeof(PlaneRecord));
    }
  }

  Tuple *sat_count_tuple = dict_find(iterator, MESSAGE_KEY_SAT_COUNT);
  Tuple *sat_data_tuple = dict_find(iterator, MESSAGE_KEY_SAT_DATA);
  if (sat_count_tuple && sat_data_tuple) {
    uint8_t count = (uint8_t)sat_count_tuple->value->int32;
    if (count > MAX_SATELLITES) count = MAX_SATELLITES;
    if (sat_data_tuple->length >= count * sizeof(SatelliteRecord)) {
      s_globe_state.sat_count = count;
      memcpy(s_globe_state.satellites, sat_data_tuple->value->data, count * sizeof(SatelliteRecord));
    }
  }

  Tuple *age_tuple = dict_find(iterator, MESSAGE_KEY_TLE_AGE_H);
  if (age_tuple) s_globe_state.tle_age_hours = (uint16_t)age_tuple->value->int32;

  Tuple *valid_tuple = dict_find(iterator, MESSAGE_KEY_DOP_VALID);
  if (valid_tuple) s_globe_state.metrics.dop_valid = valid_tuple->value->int32 != 0;

  // Geometry-only DOP; no receiver fix, accuracy, or GPS clock is measured.
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
    s_globe_state.metrics.dop_valid = false;
    s_globe_state.tle_age_hours = 65535;
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
