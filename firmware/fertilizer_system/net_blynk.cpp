/*
 * Blynk transport (optional: USE_BLYNK = 1 in config.h).
 *
 * Virtual pins
 *   V1 in   desired volume per channel (ml)        V6 in   E-stop button (1 = stop)
 *   V2 out  total dispensed, both channels (ml)    V7 in   acknowledge fault (1 = ack)
 *   V3 out  TDS (ppm)                              V8 out  state text
 *   V4 out  water level 1 (raw, LED colour)        V9 out  message / fault text
 *   V5 out  water level 2 (raw, LED colour)
 *   V10 in/out  flow profile: 0 standard, 1 high, 2 custom (set from the app, echoed by the device)
 */
#include "settings.h"

#if USE_BLYNK

// Blynk reads these macros at include time, and only ONE .cpp may include its header.
#define BLYNK_TEMPLATE_ID   BLYNK_TMPL_ID
#define BLYNK_TEMPLATE_NAME "Fertilizer System Monitoring"
#define BLYNK_AUTH_TOKEN    BLYNK_TOKEN

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include "dispenser.h"
#include "level.h"
#include "tds.h"
#include "net.h"

#define VPIN_DISPENSE_CMD V1
#define VPIN_TOTAL_VOL    V2
#define VPIN_TDS          V3
#define VPIN_WATER_LVL1   V4
#define VPIN_WATER_LVL2   V5
#define VPIN_ESTOP        V6
#define VPIN_ACK_FAULT    V7
#define VPIN_STATE        V8
#define VPIN_MESSAGE      V9
#define VPIN_FLOW_PROFILE V10

void blynkPublishStatus();   // defined below; used by BLYNK_CONNECTED

static BlynkTimer timer;
static int lastLevelPub[2] = {-1, -1};

static void say(const char* msg) {
  Serial.println(msg);
  if (Blynk.connected()) Blynk.virtualWrite(VPIN_MESSAGE, msg);
}

// ─── Periodic publishers (run only while connected) ──────────────────────────
static void publishTDS() {
  float ppm = tdsPpm();
  Serial.printf("TDS: %.0f ppm\n", ppm);
  Blynk.virtualWrite(VPIN_TDS, ppm);
}

static void publishLevels() {
  const uint8_t vpins[2] = {VPIN_WATER_LVL1, VPIN_WATER_LVL2};
  for (uint8_t i = 0; i < 2; i++) {
    LevelState s = levelOf(i);
    Blynk.virtualWrite(vpins[i], levelRaw(i));
    if ((int)s != lastLevelPub[i]) {            // colour only when it changes
      Blynk.setProperty(vpins[i], "color", levelColor(s));
      lastLevelPub[i] = (int)s;
    }
  }
}

// ─── Incoming ────────────────────────────────────────────────────────────────
BLYNK_CONNECTED() {
  lastLevelPub[0] = lastLevelPub[1] = -1;
  blynkPublishStatus();
}

BLYNK_WRITE(VPIN_DISPENSE_CMD) {
  int v = param.asInt();
  if (v == 0) return;                           // widget reset, ignore
  CmdResult r = (v < 0) ? CmdResult::BAD_VOLUME : dispenserRequest((unsigned long)v);
  if (r != CmdResult::OK) {
    char m[48];
    snprintf(m, sizeof(m), "Dispense rejected: %s", resultName(r));
    say(m);
  }
}

BLYNK_WRITE(VPIN_FLOW_PROFILE) {
  int v = param.asInt();
  if (v < 0 || v > 2) return;
  CmdResult r = dispenserSetCalProfile((CalProfile)v);
  if (r != CmdResult::OK) {
    char m[48];
    snprintf(m, sizeof(m), "Profile change rejected: %s", resultName(r));
    say(m);
    blynkPublishStatus();                 // put the widget back to the real value
  }
}

BLYNK_WRITE(VPIN_ESTOP) {
  if (param.asInt() == 1) dispenserEstop();
}

BLYNK_WRITE(VPIN_ACK_FAULT) {
  if (param.asInt() != 1) return;
  CmdResult r = dispenserAckFault();
  if (r != CmdResult::OK) {
    char m[48];
    snprintf(m, sizeof(m), "Ack failed: %s", resultName(r));
    say(m);
  }
}

// ─── Hooks called by net.cpp ─────────────────────────────────────────────────
void blynkBegin() {
  Blynk.config(BLYNK_AUTH_TOKEN);               // no blocking connect here
  timer.setInterval(800L,  publishTDS);
  timer.setInterval(1000L, publishLevels);
}

void blynkLoop() {
  static unsigned long lastAttempt = 0;
  if (WiFi.status() != WL_CONNECTED) return;

  if (Blynk.connected()) {
    Blynk.run();
    timer.run();
    return;
  }
  // Connecting can block for the timeout below, so never do it mid-dispense
  if (dispenserState() == DispState::DISPENSING) return;
  unsigned long now = millis();
  if (now - lastAttempt >= 5000UL) {
    lastAttempt = now;
    Blynk.connect(2000);
  }
}

void blynkPublishStatus() {
  if (!Blynk.connected()) return;
  Blynk.virtualWrite(VPIN_STATE, stateName(dispenserState()));
  Blynk.virtualWrite(VPIN_FLOW_PROFILE, (int)calProfile());
  if (dispenserFault() != FaultCode::NONE) Blynk.virtualWrite(VPIN_MESSAGE, faultName(dispenserFault()));
}

void blynkPublishDispense(const DispenseResult& r) {
  if (!Blynk.connected()) return;
  if (r.result == FaultCode::NONE) {
    Blynk.virtualWrite(VPIN_TOTAL_VOL, (unsigned long)(r.ml[0] + r.ml[1]));
  } else {
    char m[48];
    snprintf(m, sizeof(m), "Aborted (%s): ch1 %.0f ml, ch2 %.0f ml",
             faultName(r.result), r.ml[0], r.ml[1]);
    Blynk.virtualWrite(VPIN_MESSAGE, m);
  }
}

void blynkPublishCalRun(const CalRunResult& r) {
  if (!Blynk.connected()) return;
  char m[64];
  snprintf(m, sizeof(m), "Cal run ch%u: %lu pulses in %.1f s", r.channel, r.pulses, r.durationMs / 1000.0f);
  Blynk.virtualWrite(VPIN_MESSAGE, m);   // finish calibration from the serial console or MQTT
}

void blynkPublishFault(FaultCode code, uint8_t channel) {
  if (!Blynk.connected()) return;
  char m[40];
  snprintf(m, sizeof(m), "FAULT %s (ch %u)", faultName(code), channel);
  Blynk.virtualWrite(VPIN_MESSAGE, m);
}

#endif // USE_BLYNK
