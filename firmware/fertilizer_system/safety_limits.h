#pragma once
/*
 * limits — safety limits and level thresholds that can be changed at runtime and are saved in flash.
 * Defaults come from settings.h. They can ONLY be changed from the serial console (physical access):
 * a remote command must never be able to relax a safety limit (docs/adr/0002).
 */
#include <Arduino.h>

void          limitsBegin();
unsigned long limMaxDispenseMl();   // SAF-04
float         limMinFlow();         // SAF-01, ml/s
unsigned long limNoFlowMs();        // SAF-02
int           limLevelLow();        // raw ADC, <= this is Low
int           limLevelMed();        // raw ADC, <= this is Medium, above is High

// Keys: max_dispense_ml, min_flow, no_flow_ms, level_low, level_med.
// Returns nullptr on success, otherwise a short error message. Nothing changes on error.
const char*   limitsSet(const char* key, float value);
void          limitsReset();
void          limitsPrint();
