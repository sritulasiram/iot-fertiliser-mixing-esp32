#pragma once
/*
 * faultstore — remembers a latched fault in flash so a reset (watchdog, brown-out, power cut)
 * cannot silently clear it: the device boots into FAULT until the fault is acknowledged.
 */
#include <Arduino.h>

void faultStoreSave(uint8_t code, uint8_t channel);
bool faultStoreLoad(uint8_t* code, uint8_t* channel);   // true if a fault was stored
void faultStoreClear();
