#include "temperature.h"
#include "settings.h"

#if ENABLE_DS18B20

#include <OneWire.h>
#include <DallasTemperature.h>

static OneWire           oneWire(DS18B20_PIN);
static DallasTemperature sensors(&oneWire);
static float             lastC       = NAN;
static unsigned long     lastGoodMs  = 0;
static unsigned long     requestedAt = 0;
static unsigned long     lastCycleMs = 0;
static bool              pending     = false;

void tempBegin() {
  sensors.begin();
  sensors.setWaitForConversion(false);   // never block the loop for the ~750 ms conversion
  sensors.requestTemperatures();
  requestedAt = millis();
  pending = true;
}

void tempLoop() {
  unsigned long now = millis();
  if (pending) {
    if (now - requestedAt >= 800UL) {
      float c = sensors.getTempCByIndex(0);
      if (c != DEVICE_DISCONNECTED_C && c > -20.0f && c < 80.0f) { lastC = c; lastGoodMs = now; }
      pending = false;
      lastCycleMs = now;
    }
  } else if (now - lastCycleMs >= 2000UL) {
    sensors.requestTemperatures();
    requestedAt = now;
    pending = true;
  }
}

bool  tempValid() { return !isnan(lastC) && (millis() - lastGoodMs) < 30000UL; }
float tempC()     { return tempValid() ? lastC : TEMPERATURE_FALLBACK_C; }

#else

void  tempBegin() {}
void  tempLoop()  {}
bool  tempValid() { return false; }
float tempC()     { return TEMPERATURE_FALLBACK_C; }

#endif
