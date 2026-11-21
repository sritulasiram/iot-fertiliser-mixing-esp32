#pragma once
#include <Arduino.h>

enum LevelState : uint8_t { LVL_LOW, LVL_MEDIUM, LVL_HIGH };

void         levelBegin();
void         levelSample();                 // rate-limited internally; call every loop
int          levelRaw(uint8_t idx);
LevelState   levelOf(uint8_t idx);
bool         tankLowNow(uint8_t idx);       // latest sample is Low
bool         tankLowConfirmed(uint8_t idx); // Low for LEVEL_LOW_CONFIRM samples in a row
const char*  levelName(LevelState s);       // "LOW" | "MEDIUM" | "HIGH"
const char*  levelColor(LevelState s);      // hex colour for Blynk LED
