#pragma once
#include <pebble.h>
#include "globe.h"

void skyplot_init(void);
void skyplot_render(GContext *ctx, GRect bounds, GlobeState *state);
