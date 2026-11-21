#include "faultstore.h"
#include <Preferences.h>

static Preferences prefs;

void faultStoreSave(uint8_t code, uint8_t channel) {
  if (!prefs.begin("state", false)) return;
  prefs.putUChar("fault", code);
  prefs.putUChar("faultch", channel);
  prefs.end();
}

bool faultStoreLoad(uint8_t* code, uint8_t* channel) {
  if (!prefs.begin("state", false)) return false;
  uint8_t c = prefs.getUChar("fault", 0);
  uint8_t h = prefs.getUChar("faultch", 0);
  prefs.end();
  if (c == 0) return false;
  if (code) *code = c;
  if (channel) *channel = h;
  return true;
}

void faultStoreClear() {
  if (!prefs.begin("state", false)) return;
  prefs.remove("fault");
  prefs.remove("faultch");
  prefs.end();
}
