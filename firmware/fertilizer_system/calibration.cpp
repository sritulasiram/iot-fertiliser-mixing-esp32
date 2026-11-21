#include "calibration.h"
#include "settings.h"
#include <Preferences.h>
#include <strings.h>

static Preferences prefs;
static CalProfile  profile    = CalProfile::STANDARD;
static float       custom[2]  = {FLOW_STANDARD_1, FLOW_STANDARD_2};
static bool        hasCustom  = false;
static float       tds        = TDS_CALIB_DEFAULT;

static bool flowOk(float v) { return v >= CAL_MIN_ML_PER_PULSE && v <= CAL_MAX_ML_PER_PULSE; }
static bool tdsOk(float v)  { return v >= CAL_MIN_TDS_FACTOR  && v <= CAL_MAX_TDS_FACTOR; }

static float preset(CalProfile p, uint8_t i) {
  if (p == CalProfile::HIGHER) return (i & 1) ? FLOW_HIGH_2 : FLOW_HIGH_1;
  return (i & 1) ? FLOW_STANDARD_2 : FLOW_STANDARD_1;
}

void calBegin() {
  if (!prefs.begin("fert", false)) {
    Serial.println("Calibration: NVS unavailable, using the standard profile");
    return;
  }
  float c1 = prefs.getFloat("flow1", 0.0f);
  float c2 = prefs.getFloat("flow2", 0.0f);
  float t  = prefs.getFloat("tds",   TDS_CALIB_DEFAULT);
  uint8_t p = prefs.getUChar("profile", (uint8_t)CalProfile::STANDARD);

  // A corrupt or out-of-range stored value must never reach the pumps
  hasCustom = flowOk(c1) && flowOk(c2);
  if (hasCustom) { custom[0] = c1; custom[1] = c2; }
  tds = tdsOk(t) ? t : TDS_CALIB_DEFAULT;

  profile = (p <= (uint8_t)CalProfile::CUSTOM) ? (CalProfile)p : CalProfile::STANDARD;
  if (profile == CalProfile::CUSTOM && !hasCustom) profile = CalProfile::STANDARD;

  Serial.printf("Calibration: profile %s, flow1 %.3f, flow2 %.3f ml/pulse, TDS factor %.3f\n",
                calProfileName(profile), calFlow(0), calFlow(1), tds);
}

float calFlow(uint8_t i) {
  return profile == CalProfile::CUSTOM ? custom[i & 1] : preset(profile, i);
}
float calTds() { return tds; }

CalProfile calProfile() { return profile; }

const char* calProfileName(CalProfile p) {
  switch (p) {
    case CalProfile::HIGHER:   return "high";
    case CalProfile::CUSTOM: return "custom";
    default:                 return "standard";
  }
}

bool calParseProfile(const char* name, CalProfile* out) {
  if (!name || !out) return false;
  if (!strcasecmp(name, "standard")) { *out = CalProfile::STANDARD; return true; }
  if (!strcasecmp(name, "high"))     { *out = CalProfile::HIGHER;     return true; }
  if (!strcasecmp(name, "custom"))   { *out = CalProfile::CUSTOM;   return true; }
  return false;
}

bool calHasCustom() { return hasCustom; }

bool calSetProfile(CalProfile p) {
  if (p == CalProfile::CUSTOM && !hasCustom) return false;   // nothing custom to switch to
  profile = p;
  prefs.putUChar("profile", (uint8_t)p);
  return true;
}

bool calSetFlow(uint8_t i, float v) {
  if (!flowOk(v)) return false;
  if (!hasCustom) {                       // first custom value: start both channels from what is active now
    custom[0] = calFlow(0);
    custom[1] = calFlow(1);
    hasCustom = true;
    prefs.putFloat("flow1", custom[0]);
    prefs.putFloat("flow2", custom[1]);
  }
  custom[i & 1] = v;
  prefs.putFloat((i & 1) ? "flow2" : "flow1", v);
  profile = CalProfile::CUSTOM;
  prefs.putUChar("profile", (uint8_t)profile);
  return true;
}

bool calSetTds(float v) {
  if (!tdsOk(v)) return false;
  tds = v;
  prefs.putFloat("tds", v);
  return true;
}

void calReset() {
  prefs.remove("flow1");
  prefs.remove("flow2");
  prefs.remove("tds");
  prefs.remove("profile");
  profile   = CalProfile::STANDARD;
  hasCustom = false;
  custom[0] = FLOW_STANDARD_1;
  custom[1] = FLOW_STANDARD_2;
  tds       = TDS_CALIB_DEFAULT;
}

bool calIsCustom() {
  return profile == CalProfile::CUSTOM || tds != TDS_CALIB_DEFAULT;
}
