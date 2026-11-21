#include "level.h"
#include "settings.h"
#include "safety_limits.h"
#include "logic.h"

static const uint8_t PINS[2] = {WATER_SENSOR1_PIN, WATER_SENSOR2_PIN};
static int     rawVal[2]  = {0, 0};
static uint8_t lowCount[2] = {0, 0};

static LevelState classify(int v) {
  return (LevelState)logic::classifyLevel(v, limLevelLow(), limLevelMed());
}

static void sampleNow() {
  for (uint8_t i = 0; i < 2; i++) {
    rawVal[i] = analogRead(PINS[i]);
    if (classify(rawVal[i]) == LVL_LOW) {
      if (lowCount[i] < 255) lowCount[i]++;
    } else {
      lowCount[i] = 0;
    }
  }
}

void levelBegin() {
  pinMode(WATER_SENSOR1_PIN, INPUT);
  pinMode(WATER_SENSOR2_PIN, INPUT);
  // Prime so a Low tank is already "confirmed" before the first dispense is allowed
  for (uint8_t k = 0; k < LEVEL_LOW_CONFIRM; k++) { sampleNow(); delay(5); }
}

void levelSample() {
  static unsigned long last = 0;
  unsigned long now = millis();
  if (now - last < LEVEL_SAMPLE_MS) return;
  last = now;
  sampleNow();
}

int        levelRaw(uint8_t idx)            { return rawVal[idx & 1]; }
LevelState levelOf(uint8_t idx)             { return classify(rawVal[idx & 1]); }
bool       tankLowNow(uint8_t idx)          { return lowCount[idx & 1] > 0; }
bool       tankLowConfirmed(uint8_t idx)    { return lowCount[idx & 1] >= LEVEL_LOW_CONFIRM; }

const char* levelName(LevelState s) {
  switch (s) {
    case LVL_LOW:    return "LOW";
    case LVL_MEDIUM: return "MEDIUM";
    default:         return "HIGH";
  }
}

const char* levelColor(LevelState s) {
  switch (s) {
    case LVL_LOW:    return "#FF0000";
    case LVL_MEDIUM: return "#FFFF00";
    default:         return "#00FF00";
  }
}
