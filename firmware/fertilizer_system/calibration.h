#pragma once
/*
 * calibration — flow-factor profiles and TDS factor, saved in ESP32 flash (NVS via Preferences).
 *
 * Flow factors (ml per pulse) come from one of three profiles:
 *   STANDARD  4.25 / 5.25   the values used in the project report's accuracy tests
 *   HIGHER    5.25 / 5.50   the final prototype sketch values; name "high" (HIGH is an Arduino macro)
 *   CUSTOM    your own      saved by 'cal set' after measuring your real flow
 * The active profile survives reboots. Presets live in settings.h.
 */
#include <Arduino.h>

enum class CalProfile : uint8_t { STANDARD, HIGHER, CUSTOM };   // not HIGH: that is an Arduino macro

void        calBegin();                          // load profile and stored values
float       calFlow(uint8_t channelIdx);         // active ml/pulse, channelIdx 0 or 1
float       calTds();                            // TDS calibration factor

CalProfile  calProfile();
const char* calProfileName(CalProfile p);        // "standard" | "high" | "custom"
bool        calParseProfile(const char* name, CalProfile* out);   // case-insensitive
bool        calHasCustom();                      // custom flow values are stored
bool        calSetProfile(CalProfile p);         // CUSTOM needs stored custom values; saves choice

bool        calSetFlow(uint8_t channelIdx, float mlPerPulse);     // validates, saves as CUSTOM, activates CUSTOM
bool        calSetTds(float factor);                              // validates and saves
void        calReset();                          // STANDARD profile, custom values and TDS factor erased
bool        calIsCustom();                       // CUSTOM profile active, or TDS factor changed
