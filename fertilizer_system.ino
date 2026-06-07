/*
 * ============================================================================
 * IoT-Based Fertiliser Mixing System for Precision Farming
 * ----------------------------------------------------------------------------
 * Authors : Sri Tulasi Ram Rajalingam, Muhammad Muzakkir Mohd Nadzri,
 *           Afandi Ahmad, Mohamad Khairi Ishak
 * Journal : International Journal of Integrated Engineering, 18(1), 186-201
 * Year    : 2026
 * DOI     : https://penerbit.uthm.edu.my/ojs/index.php/ijie/article/view/24386
 * ----------------------------------------------------------------------------
 * Platform   : ESP32
 * Framework  : Arduino
 * Dependencies: Blynk (≥ 1.3.0), ESP32 Arduino Core (≥ 2.0.0)
 * ----------------------------------------------------------------------------
 * MIT License — see LICENSE file for full terms.
 * ============================================================================
 *
 * SETUP INSTRUCTIONS
 * ------------------
 * 1. Copy config.h.example → config.h
 * 2. Fill in your WiFi credentials and Blynk auth token in config.h
 * 3. Adjust CALIB_FACTOR1 / CALIB_FACTOR2 after physical flow sensor calibration
 * 4. Flash to ESP32 via Arduino IDE or PlatformIO
 *
 * CHANGELOG
 * ---------
 * v1.1.0 — Code quality & safety improvements
 *   - Non-blocking dispense state machine (WDT-safe, Blynk heartbeat maintained)
 *   - Atomic ISR variable reads via noInterrupts() / interrupts()
 *   - Independent per-pump shutoff (no volume overshoot)
 *   - ISR handlers marked IRAM_ATTR for WiFi-safe interrupt reliability
 *   - Removed blocking delay() from dispense path
 *   - Removed redundant ISR totalVolume tracking
 *   - Credentials moved to config.h (excluded from version control)
 *
 * v1.0.0 — Initial prototype (paper submission version)
 * ============================================================================
 */

// ─── User Configuration (credentials & secrets) ──────────────────────────────
#include "config.h"

// ─── Blynk Config ────────────────────────────────────────────────────────────
#define BLYNK_TEMPLATE_ID   BLYNK_TMPL_ID
#define BLYNK_TEMPLATE_NAME "Fertilizer System Monitoring"
#define BLYNK_AUTH_TOKEN    BLYNK_TOKEN

#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// ─── Pin Definitions ─────────────────────────────────────────────────────────
#define PUMP1_PIN         16
#define PUMP2_PIN         17
#define SOLENOID1_PIN     22
#define SOLENOID2_PIN     23
#define FLOW_SENSOR1_PIN  18
#define FLOW_SENSOR2_PIN  19
#define TDS_SENSOR_PIN    36  // GPIO36 (VP) — ADC1 only; ADC2 unusable with WiFi
#define WATER_SENSOR1_PIN 34
#define WATER_SENSOR2_PIN 35

// ─── Blynk Virtual Pins ──────────────────────────────────────────────────────
#define VPIN_DISPENSE_CMD V1  // Input:  desired volume (ml) from app
#define VPIN_TOTAL_VOL    V2  // Output: total volume dispensed (ml)
#define VPIN_TDS          V3  // Output: TDS value (ppm)
#define VPIN_WATER_LVL1   V4  // Output: water level sensor 1
#define VPIN_WATER_LVL2   V5  // Output: water level sensor 2

// ─── LED Colour Constants ─────────────────────────────────────────────────────
#define COLOR_RED    "#FF0000"
#define COLOR_YELLOW "#FFFF00"
#define COLOR_GREEN  "#00FF00"

// ─── Flow Sensor Calibration ──────────────────────────────────────────────────
// Units: ml per pulse
// To calibrate: run pump for a known volume, count pulses via Serial Monitor,
// then set factor = measured_ml / pulse_count
const float CALIB_FACTOR1 = 5.25;
const float CALIB_FACTOR2 = 5.50;

// ─── ISR Variables ───────────────────────────────────────────────────────────
// volatile: modified inside interrupt context, read in main loop
volatile unsigned long pulseCount1 = 0;
volatile unsigned long pulseCount2 = 0;

// ─── Dispense State Machine ───────────────────────────────────────────────────
enum DispenseState { IDLE, DISPENSING };
DispenseState dispenseState = IDLE;
unsigned long desiredVolume = 0;
bool pump1Running           = false;
bool pump2Running           = false;

// ─── TDS Sensor ──────────────────────────────────────────────────────────────
#define VREF   3.3f   // ESP32 ADC reference voltage (V)
#define SCOUNT 30     // Median filter window size

int   analogBuffer[SCOUNT];
int   analogBufferIndex = 0;
float tdsValue          = 0.0f;

// Temperature compensation — replace TEMPERATURE with a live sensor reading
// (e.g. DS18B20) for improved accuracy in variable-temperature environments
const float TEMPERATURE      = 23.0f;  // °C
const float TDS_CALIB_FACTOR = 2.3f;   // Adjust against a known reference solution

// ─── Timer ───────────────────────────────────────────────────────────────────
BlynkTimer timer;

// ─── Function Prototypes ─────────────────────────────────────────────────────
void   IRAM_ATTR onFlow1();
void   IRAM_ATTR onFlow2();
void   startDispense(unsigned long volume);
void   updateDispense();
void   readWaterLevels();
void   publishTDS();
int    getMedianNum(int bArray[], int iFilterLen);
String getLevel(int value);
String getLedColor(const String& level);


// ═════════════════════════════════════════════════════════════════════════════
//  SETUP
// ═════════════════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);

  // ── Pin Modes ──────────────────────────────────────────────────────────────
  pinMode(PUMP1_PIN,         OUTPUT);
  pinMode(PUMP2_PIN,         OUTPUT);
  pinMode(SOLENOID1_PIN,     OUTPUT);
  pinMode(SOLENOID2_PIN,     OUTPUT);
  pinMode(FLOW_SENSOR1_PIN,  INPUT_PULLUP);
  pinMode(FLOW_SENSOR2_PIN,  INPUT_PULLUP);
  pinMode(TDS_SENSOR_PIN,    INPUT);
  pinMode(WATER_SENSOR1_PIN, INPUT);
  pinMode(WATER_SENSOR2_PIN, INPUT);

  // Ensure actuators start in safe OFF state
  digitalWrite(PUMP1_PIN,     LOW);
  digitalWrite(PUMP2_PIN,     LOW);
  digitalWrite(SOLENOID1_PIN, LOW);
  digitalWrite(SOLENOID2_PIN, LOW);

  // ── Flow Sensor Interrupts ────────────────────────────────────────────────
  // IRAM_ATTR ensures ISRs run from internal RAM — reliable even during
  // flash cache misses caused by WiFi activity
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR1_PIN), onFlow1, RISING);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR2_PIN), onFlow2, RISING);

  // ── Network ───────────────────────────────────────────────────────────────
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println(" OK");

  Serial.print("Connecting to Blynk");
  Blynk.begin(BLYNK_TOKEN, WIFI_SSID, WIFI_PASS);
  while (!Blynk.connected()) { delay(500); Serial.print("."); }
  Serial.println(" OK");

  // ── Periodic Tasks ────────────────────────────────────────────────────────
  timer.setInterval(1000L, readWaterLevels);  // water levels every 1 s
  timer.setInterval(800L,  publishTDS);       // TDS publish every 0.8 s

  Serial.println("System ready.");
}


// ═════════════════════════════════════════════════════════════════════════════
//  LOOP
// ═════════════════════════════════════════════════════════════════════════════
void loop() {
  Blynk.run();
  timer.run();

  // ── TDS ADC Sampling — non-blocking, every 40 ms ──────────────────────────
  static unsigned long lastSample = 0;
  if (millis() - lastSample >= 40UL) {
    lastSample = millis();
    analogBuffer[analogBufferIndex] = analogRead(TDS_SENSOR_PIN);
    analogBufferIndex = (analogBufferIndex + 1) % SCOUNT;
  }

  // ── Dispense State Machine ────────────────────────────────────────────────
  if (dispenseState == DISPENSING) {
    updateDispense();
  }
}


// ═════════════════════════════════════════════════════════════════════════════
//  ISR — Flow Sensors
// ═════════════════════════════════════════════════════════════════════════════
void IRAM_ATTR onFlow1() { pulseCount1++; }
void IRAM_ATTR onFlow2() { pulseCount2++; }


// ═════════════════════════════════════════════════════════════════════════════
//  BLYNK — Receive Dispense Command (V1)
// ═════════════════════════════════════════════════════════════════════════════
BLYNK_WRITE(VPIN_DISPENSE_CMD) {
  unsigned long vol = param.asInt();
  if (vol == 0) return;

  if (dispenseState == DISPENSING) {
    Serial.println("Warning: dispense already in progress — command ignored.");
    return;
  }

  Serial.print("Dispense requested: ");
  Serial.print(vol);
  Serial.println(" ml per side");

  startDispense(vol);
}


// ═════════════════════════════════════════════════════════════════════════════
//  DISPENSING — Start
// ═════════════════════════════════════════════════════════════════════════════
void startDispense(unsigned long volume) {
  desiredVolume = volume;

  // Atomically reset pulse counters
  noInterrupts();
  pulseCount1 = 0;
  pulseCount2 = 0;
  interrupts();

  pump1Running = true;
  pump2Running = true;

  // Open solenoid valves before starting pumps
  digitalWrite(SOLENOID1_PIN, HIGH);
  digitalWrite(SOLENOID2_PIN, HIGH);
  digitalWrite(PUMP1_PIN,     HIGH);
  digitalWrite(PUMP2_PIN,     HIGH);

  dispenseState = DISPENSING;
  Serial.println("Dispensing started.");
}


// ═════════════════════════════════════════════════════════════════════════════
//  DISPENSING — Non-blocking Update (called every loop iteration)
// ═════════════════════════════════════════════════════════════════════════════
void updateDispense() {
  // Atomic read — prevents torn reads of multi-byte values modified in ISR
  noInterrupts();
  unsigned long p1 = pulseCount1;
  unsigned long p2 = pulseCount2;
  interrupts();

  float dispensed1 = p1 * CALIB_FACTOR1;  // ml
  float dispensed2 = p2 * CALIB_FACTOR2;  // ml

  // Each pump shuts off independently when its own target is reached
  if (pump1Running && dispensed1 >= (float)desiredVolume) {
    digitalWrite(PUMP1_PIN,     LOW);
    digitalWrite(SOLENOID1_PIN, LOW);
    pump1Running = false;
    Serial.printf("Pump 1 done: %.1f ml\n", dispensed1);
  }

  if (pump2Running && dispensed2 >= (float)desiredVolume) {
    digitalWrite(PUMP2_PIN,     LOW);
    digitalWrite(SOLENOID2_PIN, LOW);
    pump2Running = false;
    Serial.printf("Pump 2 done: %.1f ml\n", dispensed2);
  }

  // Both done — report total and return to IDLE
  if (!pump1Running && !pump2Running) {
    unsigned long totalVolume = (unsigned long)(dispensed1 + dispensed2);

    Serial.printf("Total Volume Dispensed: %lu ml\n", totalVolume);
    Serial.println("Dispensing complete.");

    Blynk.virtualWrite(VPIN_TOTAL_VOL, totalVolume);

    dispenseState = IDLE;
    desiredVolume = 0;
  }
}


// ═════════════════════════════════════════════════════════════════════════════
//  TDS — Median-Filtered Reading → Blynk
// ═════════════════════════════════════════════════════════════════════════════
void publishTDS() {
  // Snapshot the ADC buffer before filtering
  int tempBuf[SCOUNT];
  for (int i = 0; i < SCOUNT; i++) tempBuf[i] = analogBuffer[i];

  // Convert ADC median to voltage
  float voltage = getMedianNum(tempBuf, SCOUNT) * VREF / 4096.0f;

  // Temperature compensation: normalise to 25 °C reference
  float compensationCoeff  = 1.0f + 0.02f * (TEMPERATURE - 25.0f);
  float compensatedVoltage = voltage / compensationCoeff;

  // Polynomial voltage-to-TDS conversion with calibration factor
  tdsValue = (133.42f * compensatedVoltage * compensatedVoltage * compensatedVoltage
            - 255.86f * compensatedVoltage * compensatedVoltage
            + 857.39f * compensatedVoltage) * 0.5f * TDS_CALIB_FACTOR;

  Serial.printf("TDS: %.0f ppm\n", tdsValue);
  Blynk.virtualWrite(VPIN_TDS, tdsValue);
}


// ═════════════════════════════════════════════════════════════════════════════
//  WATER LEVELS — Read & Push to Blynk with Colour Indicators
// ═════════════════════════════════════════════════════════════════════════════
void readWaterLevels() {
  int val1 = analogRead(WATER_SENSOR1_PIN);
  int val2 = analogRead(WATER_SENSOR2_PIN);

  String lvl1 = getLevel(val1);
  String lvl2 = getLevel(val2);

  Serial.printf("Water | Sensor 1: %d (%s)  Sensor 2: %d (%s)\n",
                val1, lvl1.c_str(), val2, lvl2.c_str());

  Blynk.virtualWrite(VPIN_WATER_LVL1, val1);
  Blynk.setProperty(VPIN_WATER_LVL1, "color", getLedColor(lvl1));

  Blynk.virtualWrite(VPIN_WATER_LVL2, val2);
  Blynk.setProperty(VPIN_WATER_LVL2, "color", getLedColor(lvl2));
}


// ═════════════════════════════════════════════════════════════════════════════
//  HELPERS
// ═════════════════════════════════════════════════════════════════════════════
String getLevel(int value) {
  if (value <= 700)  return "Low";
  if (value <= 1200) return "Medium";
  return "High";
}

String getLedColor(const String& level) {
  if (level == "Low")    return COLOR_RED;
  if (level == "Medium") return COLOR_YELLOW;
  return COLOR_GREEN;
}

// Median filter: operates on a local copy; does not modify original array
int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (int i = 0; i < iFilterLen; i++) bTab[i] = bArray[i];

  for (int j = 0; j < iFilterLen - 1; j++) {
    for (int i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        int tmp      = bTab[i];
        bTab[i]      = bTab[i + 1];
        bTab[i + 1]  = tmp;
      }
    }
  }

  return (iFilterLen & 1)
    ? bTab[(iFilterLen - 1) / 2]
    : (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
}
