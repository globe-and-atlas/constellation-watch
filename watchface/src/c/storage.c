#include "storage.h"

bool storage_load_state(GlobeState *state) {
  if (!persist_exists(PERSIST_KEY_STATE)) {
    return false;
  }
  int bytes_read = persist_read_data(PERSIST_KEY_STATE, state, sizeof(GlobeState));
  return (bytes_read == sizeof(GlobeState));
}

bool storage_save_state(const GlobeState *state) {
  int bytes_written = persist_write_data(PERSIST_KEY_STATE, state, sizeof(GlobeState));
  return (bytes_written == sizeof(GlobeState));
}
