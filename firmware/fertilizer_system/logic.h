#pragma once
/*
 * logic — pure functions with no Arduino or hardware dependencies, so they can be unit-tested
 * on a PC (see tests/test_logic.cpp). The firmware calls these from tds, level and dispenser.
 */
#include <stdint.h>

namespace logic {

// Median of up to 64 samples (sorts a local copy; the input is not modified).
inline int medianOf(const int* src, int len) {
  if (len < 1) return 0;
  if (len > 64) len = 64;
  int t[64];
  for (int i = 0; i < len; i++) t[i] = src[i];
  for (int j = 0; j < len - 1; j++) {
    for (int i = 0; i < len - j - 1; i++) {
      if (t[i] > t[i + 1]) { int tmp = t[i]; t[i] = t[i + 1]; t[i + 1] = tmp; }
    }
  }
  return (len & 1) ? t[(len - 1) / 2] : (t[len / 2] + t[len / 2 - 1]) / 2;
}

// 0 = Low (raw <= lowMax), 1 = Medium (raw <= medMax), 2 = High
inline int classifyLevel(int raw, int lowMax, int medMax) {
  return raw <= lowMax ? 0 : (raw <= medMax ? 1 : 2);
}

inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Longest a dispense may run: the time to move `volumeMl` at the slowest acceptable flow, plus a grace period.
inline unsigned long maxRunMs(unsigned long volumeMl, float minFlowMlPerS, unsigned long graceMs) {
  return (unsigned long)((float)volumeMl * 1000.0f / minFlowMlPerS) + graceMs;
}

// Adaptive run-on compensation. After the pump is switched off, some more liquid still flows
// (coasting pump, valve closing). `observedMl` is what was counted after switch-off. The estimate is a
// 50 % moving average, never negative, and never more than maxMl or half of the requested volume
// (so small requests cannot be cut off at the very start).
inline float updateRunOn(float prevMl, float observedMl, float desiredMl, float maxMl) {
  float v = 0.5f * prevMl + 0.5f * observedMl;
  float cap = desiredMl * 0.5f < maxMl ? desiredMl * 0.5f : maxMl;
  return clampf(v, 0.0f, cap);
}

// Stop the pump early by the expected run-on so the final volume lands on the target.
inline bool reachedTarget(float dispensedMl, float runOnMl, float desiredMl) {
  return dispensedMl + runOnMl >= desiredMl;
}

// Suggested MIN_FLOW setting from a measured rate: half of it, so a weak or partly blocked
// line is caught quickly without false alarms.
inline float suggestMinFlow(float measuredMlPerS) { return measuredMlPerS * 0.5f; }

}  // namespace logic
