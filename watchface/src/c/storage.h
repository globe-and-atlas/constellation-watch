#pragma once
#include <pebble.h>
#include "globe.h"

#define PERSIST_KEY_STATE 100

bool storage_load_state(GlobeState *state);
bool storage_save_state(const GlobeState *state);
