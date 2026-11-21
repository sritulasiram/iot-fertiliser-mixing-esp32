#pragma once
/*
 * temperature — optional DS18B20 water temperature (ENABLE_DS18B20 = 1).
 * When disabled or the sensor fails, tempC() falls back to TEMPERATURE_FALLBACK_C
 * and tempValid() is false.
 */
#include <Arduino.h>

void  tempBegin();
void  tempLoop();     // non-blocking; call every loop iteration
bool  tempValid();    // a recent real reading exists
float tempC();        // live reading if valid, else the fallback
