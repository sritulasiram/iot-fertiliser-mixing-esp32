#include "tds.h"
#include "settings.h"
#include "calibration.h"
#include "temperature.h"
#include "logic.h"

#define VREF   3.3f   // ESP32 ADC reference (V)
#define SCOUNT 30     // median window

static int analogBuffer[SCOUNT];
static int bufferIndex = 0;

void tdsBegin() {
  pinMode(TDS_SENSOR_PIN, INPUT);
  // Fill the whole window so the first readings are real, not zeros
  for (int i = 0; i < SCOUNT; i++) { analogBuffer[i] = analogRead(TDS_SENSOR_PIN); delay(5); }
}

void tdsSample() {
  static unsigned long last = 0;
  unsigned long now = millis();
  if (now - last < 40UL) return;
  last = now;
  analogBuffer[bufferIndex] = analogRead(TDS_SENSOR_PIN);
  bufferIndex = (bufferIndex + 1) % SCOUNT;
}

float tdsPpm() {
  float voltage = logic::medianOf(analogBuffer, SCOUNT) * VREF / 4096.0f;
  float comp    = 1.0f + 0.02f * (tempC() - 25.0f);   // normalise to 25 °C
  float v       = voltage / comp;
  return (133.42f * v * v * v - 255.86f * v * v + 857.39f * v) * 0.5f * calTds();
}

bool tdsCalibrate(float referencePpm, float* newFactorOut) {
  float current = tdsPpm();
  if (current < 10.0f || referencePpm <= 0.0f) return false;   // probe dry or bad input
  float f = calTds() * referencePpm / current;                  // reading is linear in the factor
  if (!calSetTds(f)) return false;
  if (newFactorOut) *newFactorOut = f;
  return true;
}
