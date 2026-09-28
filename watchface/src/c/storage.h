#pragma once
#include <pebble.h>
#include "globe.h"

#define PERSIST_KEY_STATE 101 // v2: state layout changed to remove simulated receiver metrics

bool storage_load_state(GlobeState *state);
bool storage_save_state(const GlobeState *state);
