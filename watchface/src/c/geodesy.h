#pragma once
#include <pebble.h>
#include "globe.h"

void geodesy_init(void);
void geodesy_render(GContext *ctx, GRect bounds, GlobeState *state);
