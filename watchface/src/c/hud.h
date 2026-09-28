#pragma once
#include <pebble.h>
#include "globe.h"

void hud_init(void);
void hud_render_header(GContext *ctx, GRect bounds, const char *status_str);
void hud_render_footer(GContext *ctx, GRect bounds, GlobeState *state, const char *footer_str);
