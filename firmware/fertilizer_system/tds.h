#pragma once
#include <Arduino.h>

void  tdsBegin();
void  tdsSample();   // rate-limited internally (every 40 ms); call every loop
float tdsPpm();      // median-filtered, temperature-compensated reading

// Calibrate against a reference solution of known TDS (ppm): scales the stored factor so the
// current reading matches. Returns false if the reading is too low or the result implausible.
bool  tdsCalibrate(float referencePpm, float* newFactorOut);
