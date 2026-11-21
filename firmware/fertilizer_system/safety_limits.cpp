#include "safety_limits.h"
#include "settings.h"
#include <Preferences.h>
#include <string.h>

static Preferences prefs;
static bool        open_ = false;
static unsigned long maxMl   = MAX_DISPENSE_ML;
static float         minFlow = MIN_FLOW_ML_PER_S;
static unsigned long noFlow  = NO_FLOW_TIMEOUT_MS;
static int           lvLow   = LEVEL_LOW_MAX;
static int           lvMed   = LEVEL_MED_MAX;

unsigned long limMaxDispenseMl() { return maxMl; }
float         limMinFlow()       { return minFlow; }
unsigned long limNoFlowMs()      { return noFlow; }
int           limLevelLow()      { return lvLow; }
int           limLevelMed()      { return lvMed; }

void limitsBegin() {
  open_ = prefs.begin("limits", false);
  if (!open_) { Serial.println("Limits: NVS unavailable, using defaults"); return; }
  unsigned long m = prefs.getULong("maxml", MAX_DISPENSE_ML);
  float f         = prefs.getFloat("minflow", MIN_FLOW_ML_PER_S);
  unsigned long n = prefs.getULong("noflow", NO_FLOW_TIMEOUT_MS);
  int lo          = prefs.getInt("lvlow", LEVEL_LOW_MAX);
  int me          = prefs.getInt("lvmed", LEVEL_MED_MAX);
  // Reject corrupt values rather than let them reach the pumps
  if (m >= 1 && m <= 5000)        maxMl = m;
  if (f >= 0.1f && f <= 500.0f)   minFlow = f;
  if (n >= 500 && n <= 60000)     noFlow = n;
  if (lo >= 0 && me > lo && me <= 4095) { lvLow = lo; lvMed = me; }
}

const char* limitsSet(const char* key, float v) {
  if (!key) return "no key";
  if (!strcmp(key, "max_dispense_ml")) {
    if (v < 1 || v > 5000) return "max_dispense_ml must be 1-5000";
    maxMl = (unsigned long)v;  if (open_) prefs.putULong("maxml", maxMl);
  } else if (!strcmp(key, "min_flow")) {
    if (v < 0.1f || v > 500.0f) return "min_flow must be 0.1-500 ml/s";
    minFlow = v;  if (open_) prefs.putFloat("minflow", minFlow);
  } else if (!strcmp(key, "no_flow_ms")) {
    if (v < 500 || v > 60000) return "no_flow_ms must be 500-60000";
    noFlow = (unsigned long)v;  if (open_) prefs.putULong("noflow", noFlow);
  } else if (!strcmp(key, "level_low")) {
    if (v < 0 || v >= lvMed) return "level_low must be 0 and below level_med";
    lvLow = (int)v;  if (open_) prefs.putInt("lvlow", lvLow);
  } else if (!strcmp(key, "level_med")) {
    if (v <= lvLow || v > 4095) return "level_med must be above level_low and at most 4095";
    lvMed = (int)v;  if (open_) prefs.putInt("lvmed", lvMed);
  } else {
    return "unknown key (max_dispense_ml, min_flow, no_flow_ms, level_low, level_med)";
  }
  return nullptr;
}

void limitsReset() {
  if (open_) { prefs.remove("maxml"); prefs.remove("minflow"); prefs.remove("noflow"); prefs.remove("lvlow"); prefs.remove("lvmed"); }
  maxMl = MAX_DISPENSE_ML;  minFlow = MIN_FLOW_ML_PER_S;  noFlow = NO_FLOW_TIMEOUT_MS;
  lvLow = LEVEL_LOW_MAX;    lvMed = LEVEL_MED_MAX;
}

void limitsPrint() {
  Serial.printf("max_dispense_ml %lu | min_flow %.1f ml/s | no_flow_ms %lu | level_low %d | level_med %d\n",
                maxMl, minFlow, noFlow, lvLow, lvMed);
}
