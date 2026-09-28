#pragma once
#include <pebble.h>
#include "globe.h"

void ground_track_init(void);
void ground_track_render(GContext *ctx, GRect bounds, GlobeState *state);
