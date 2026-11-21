#include "dispenser.h"
#include "settings.h"
#include "calibration.h"
#include "safety_limits.h"
#include "logic.h"
#include "faultstore.h"
#include "level.h"
#include "net.h"

// ─── Per-channel data ────────────────────────────────────────────────────────
struct Channel {
  uint8_t       pumpPin;
  uint8_t       solPin;
  float         calib;        // ml per pulse (from the active flow profile)
  bool          running;      // part of the active job and not yet finished
  bool          solClosing;   // pump is off, valve scheduled to close (SAF-09 lag)
  unsigned long solCloseAt;
  unsigned long lastCount;    // last pulse count seen by the main loop
  unsigned long lastChangeMs; // when lastCount last changed (no-flow detection)
  float         dispensedMl;  // volume so far; final (including run-on) once the valve has closed
  float         runOnMl;      // learned: liquid that still flows after the pump is switched off
  unsigned long stopPulses;   // pulse count at pump switch-off, to measure the run-on
  bool          learning;     // measure run-on when the valve closes
};

static Channel ch[2] = {
  {PUMP1_PIN, SOLENOID1_PIN, FLOW_STANDARD_1, false, false, 0, 0, 0, 0.0f, 0.0f, 0, false},
  {PUMP2_PIN, SOLENOID2_PIN, FLOW_STANDARD_2, false, false, 0, 0, 0, 0.0f, 0.0f, 0, false},
};

// ─── Flow ISR state ──────────────────────────────────────────────────────────
// portMUX spinlock: noInterrupts() only masks the current core on a dual-core ESP32.
static portMUX_TYPE flowMux = portMUX_INITIALIZER_UNLOCKED;
static volatile unsigned long pulseCount[2]  = {0, 0};
static volatile unsigned long lastPulseUs[2] = {0, 0};

static void IRAM_ATTR flowIsr(uint8_t i) {
  unsigned long now = micros();
  portENTER_CRITICAL_ISR(&flowMux);
  if (now - lastPulseUs[i] >= FLOW_DEBOUNCE_US) {
    pulseCount[i] = pulseCount[i] + 1;   // inside the spinlock, so a plain read-modify-write is safe
    lastPulseUs[i] = now;
  }
  portEXIT_CRITICAL_ISR(&flowMux);
}
static void IRAM_ATTR onFlow1() { flowIsr(0); }
static void IRAM_ATTR onFlow2() { flowIsr(1); }

static unsigned long readPulses(uint8_t i) {
  portENTER_CRITICAL(&flowMux);
  unsigned long p = pulseCount[i];
  portEXIT_CRITICAL(&flowMux);
  return p;
}

// ─── State ───────────────────────────────────────────────────────────────────
static DispState     state        = DispState::IDLE;
static FaultCode     fault        = FaultCode::NONE;
static uint8_t       faultChannel = 0;
static unsigned long desiredMl    = 0;
static unsigned long startMs      = 0;
static unsigned long pumpStartMs  = 0;
static unsigned long maxRunMs     = 0;
static bool          pumpsStarted = false;
static char          activeCmdId[40] = "";

// Calibration run bookkeeping
static bool          calMode       = false;
static uint8_t       calChannel    = 0;      // 0-based channel of the active/last run
static unsigned long calDurationMs = 0;
static unsigned long calPulses[2]  = {0, 0}; // pulses from the last completed run, awaiting a measured volume
static unsigned long calRunMs[2]   = {0, 0}; // how long that run lasted (to report the flow rate)

// ─── Helpers ─────────────────────────────────────────────────────────────────
static void stopAllOutputs() {
  for (uint8_t i = 0; i < 2; i++) {
    digitalWrite(ch[i].pumpPin, LOW);   // pump first, then valve
    digitalWrite(ch[i].solPin,  LOW);
    ch[i].running    = false;
    ch[i].solClosing = false;
    ch[i].learning   = false;           // an aborted job teaches nothing
  }
}

static void resetRunOn() { ch[0].runOnMl = ch[1].runOnMl = 0.0f; }   // factors changed: forget what was learned

static void fillResult(DispenseResult& r, FaultCode result) {
  strlcpy(r.id, activeCmdId, sizeof(r.id));
  r.requestedMl = desiredMl;
  r.ml[0] = ch[0].dispensedMl;
  r.ml[1] = ch[1].dispensedMl;
  r.result = result;
}

static void raiseFault(FaultCode code, uint8_t channelNum) {
  bool wasDispensing = (state == DispState::DISPENSING) && !calMode;
  stopAllOutputs();                       // fail safe first, report after
  calMode = false;
  state = DispState::FAULT;
  fault = code;
  faultChannel = channelNum;
  faultStoreSave((uint8_t)code, channelNum);   // survives a reset until acknowledged
  Serial.printf("FAULT: %s (channel %u)\n", faultName(code), channelNum);

  if (wasDispensing) {
    DispenseResult r;
    fillResult(r, code);
    netPublishDispense(r);
  }
  netPublishFault(code, channelNum);
  netPublishStatus();
}

static void completeDispense() {
  DispenseResult r;
  fillResult(r, FaultCode::NONE);
  state = DispState::IDLE;
  Serial.printf("Dispense complete: ch1 %.1f ml, ch2 %.1f ml (run-on comp %.1f / %.1f ml)\n",
                r.ml[0], r.ml[1], ch[0].runOnMl, ch[1].runOnMl);
  netPublishDispense(r);
  netPublishStatus();
  desiredMl = 0;
  activeCmdId[0] = '\0';
}

static void completeCalRun() {
  CalRunResult r;
  strlcpy(r.id, activeCmdId, sizeof(r.id));
  r.channel    = calChannel + 1;
  r.pulses     = calPulses[calChannel];
  r.durationMs = calDurationMs;
  calRunMs[calChannel] = calDurationMs;
  calMode = false;
  state = DispState::IDLE;
  Serial.printf("Calibration run done: channel %u, %lu pulses in %lu ms. Measure the collected ml, then set it.\n",
                r.channel, r.pulses, r.durationMs);
  netPublishCalRun(r);
  netPublishStatus();
  activeCmdId[0] = '\0';
}

// ─── E-stop button (optional hardware) ───────────────────────────────────────
// Normally-closed to GND: pressed or wire broken => pin reads HIGH.
static bool estopButtonLatched = false;

static void serviceEstopButton(unsigned long now) {
#if ENABLE_ESTOP_BUTTON
  static unsigned long highSince = 0;
  if (digitalRead(ESTOP_BUTTON_PIN) == HIGH) {
    if (highSince == 0) highSince = now ? now : 1;
    if (now - highSince >= 50UL) estopButtonLatched = true;   // debounced
  } else {
    highSince = 0;
    estopButtonLatched = false;
  }
  if (estopButtonLatched && !(state == DispState::FAULT && fault == FaultCode::ESTOP)) {
    raiseFault(FaultCode::ESTOP, 0);
  }
#else
  (void)now;
#endif
}

// Close valves whose lag has elapsed. This is also where the volume becomes final: flow that
// continued after the pump stopped (coasting, valve closing) is counted, so the reported volume,
// the calibration pulse count and the run-on estimate all include it.
static void serviceSolenoids(unsigned long now) {
  for (uint8_t i = 0; i < 2; i++) {
    if (!(ch[i].solClosing && (long)(now - ch[i].solCloseAt) >= 0)) continue;
    digitalWrite(ch[i].solPin, LOW);
    ch[i].solClosing = false;

    unsigned long p = readPulses(i);
    ch[i].dispensedMl = p * ch[i].calib;
    if (calMode) {
      calPulses[i] = p;                                          // collected volume includes the run-on
    } else if (ch[i].learning) {
      float extraMl = (float)(p - ch[i].stopPulses) * ch[i].calib;
      ch[i].runOnMl = logic::updateRunOn(ch[i].runOnMl, extraMl, (float)desiredMl, RUNON_MAX_ML);
      ch[i].learning = false;
    }
  }
}

// Shared start sequence for dispenses and calibration runs
static void beginJob(uint8_t mask, unsigned long now) {
  portENTER_CRITICAL(&flowMux);
  pulseCount[0] = 0;
  pulseCount[1] = 0;
  portEXIT_CRITICAL(&flowMux);

  for (uint8_t i = 0; i < 2; i++) {
    ch[i].running      = (mask & (1 << i)) != 0;
    ch[i].solClosing   = false;
    ch[i].learning     = false;
    ch[i].lastCount    = 0;
    ch[i].lastChangeMs = now;
    ch[i].dispensedMl  = 0.0f;
    if (ch[i].running) digitalWrite(ch[i].solPin, HIGH);   // valve first, pump after LEAD
  }
  startMs      = now;
  pumpsStarted = false;
  state        = DispState::DISPENSING;
}

// ─── Job update ──────────────────────────────────────────────────────────────
static void updateDispense(unsigned long now) {
  // SAF-09: valves have been open for SOLENOID_LEAD_MS before pumps start
  if (!pumpsStarted) {
    if (now - startMs < SOLENOID_LEAD_MS) return;
    pumpsStarted = true;
    pumpStartMs = now;
    for (uint8_t i = 0; i < 2; i++) {
      if (ch[i].running) { digitalWrite(ch[i].pumpPin, HIGH); ch[i].lastChangeMs = now; }
    }
    Serial.println("Pumps started.");
  }

  // SAF-01: overall time limit
  if (now - startMs > maxRunMs) { raiseFault(FaultCode::TIMEOUT, 0); return; }

  bool anyRunning = false;
  for (uint8_t i = 0; i < 2; i++) {
    if (!ch[i].running) continue;

    unsigned long p = readPulses(i);
    if (p != ch[i].lastCount) { ch[i].lastCount = p; ch[i].lastChangeMs = now; }
    ch[i].dispensedMl = p * ch[i].calib;

    // Done: target reached, counting the learned run-on (dispense) or run time elapsed (calibration)
#if ENABLE_RUNON_COMP
    const float runOn = ch[i].runOnMl;
#else
    const float runOn = 0.0f;
#endif
    bool done = calMode ? (now - pumpStartMs >= calDurationMs)
                        : logic::reachedTarget(ch[i].dispensedMl, runOn, (float)desiredMl);
    if (done) {
      digitalWrite(ch[i].pumpPin, LOW);       // pump off now, valve closes after SOLENOID_LAG_MS
      ch[i].running    = false;
      ch[i].solClosing = true;
      ch[i].solCloseAt = now + SOLENOID_LAG_MS;
      ch[i].stopPulses = p;
#if ENABLE_RUNON_COMP
      ch[i].learning   = !calMode;
#endif
      if (!calMode) Serial.printf("Channel %u pump off at %.1f ml\n", i + 1, ch[i].dispensedMl);
      continue;
    }

    // SAF-03: source tank ran low
    if (tankLowConfirmed(TANK_SENSOR_FOR_CHANNEL[i])) { raiseFault(FaultCode::TANK_LOW, i + 1); return; }

    // SAF-02: pump on but no flow pulses (also catches a calibration run with a dead sensor)
    if (now - ch[i].lastChangeMs > limNoFlowMs()) { raiseFault(FaultCode::NO_FLOW, i + 1); return; }

    anyRunning = true;
  }

  if (!anyRunning) {
    // Wait until every valve has closed so the reported volume is final
    for (uint8_t i = 0; i < 2; i++) if (ch[i].solClosing) return;
    if (calMode) completeCalRun(); else completeDispense();
  }
}

// ─── Public API ──────────────────────────────────────────────────────────────
void dispenserSafeOutputs() {
  const uint8_t outs[4] = {PUMP1_PIN, PUMP2_PIN, SOLENOID1_PIN, SOLENOID2_PIN};
  for (uint8_t k = 0; k < 4; k++) {
    digitalWrite(outs[k], LOW);   // set level before enabling the driver: no glitch
    pinMode(outs[k], OUTPUT);
    digitalWrite(outs[k], LOW);
  }
}

void dispenserBegin() {
  ch[0].calib = calFlow(0);       // active profile (saved on the device)
  ch[1].calib = calFlow(1);
  pinMode(FLOW_SENSOR1_PIN, INPUT_PULLUP);
  pinMode(FLOW_SENSOR2_PIN, INPUT_PULLUP);
#if ENABLE_ESTOP_BUTTON
  pinMode(ESTOP_BUTTON_PIN, INPUT_PULLUP);
#endif
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR1_PIN), onFlow1, RISING);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR2_PIN), onFlow2, RISING);

  // A fault latched before a reset stays latched: outputs are already off, now refuse to run
  uint8_t code = 0, chan = 0;
  if (faultStoreLoad(&code, &chan) && code <= (uint8_t)FaultCode::ESTOP) {
    state = DispState::FAULT;
    fault = (FaultCode)code;
    faultChannel = chan;
    Serial.printf("Restored latched fault after reset: %s (channel %u). Acknowledge it ('ack') to continue.\n",
                  faultName(fault), chan);
  }
}

void dispenserLoop() {
  unsigned long now = millis();
  levelSample();
  serviceEstopButton(now);
  serviceSolenoids(now);
  if (state == DispState::DISPENSING) updateDispense(now);
}

// Common pre-start checks. Returns OK if a job may start for this mask.
static CmdResult checkCanStart(uint8_t mask) {
  if (state == DispState::FAULT)
    return fault == FaultCode::ESTOP ? CmdResult::ESTOP_ACTIVE : CmdResult::FAULTED;
  if (state == DispState::DISPENSING) return CmdResult::BUSY;
  if (mask == 0) return CmdResult::BAD_CHANNELS;
  // SAF-03: refuse to start from a Low tank
  for (uint8_t i = 0; i < 2; i++) {
    if ((mask & (1 << i)) && tankLowNow(TANK_SENSOR_FOR_CHANNEL[i])) return CmdResult::TANK_LOW;
  }
  return CmdResult::OK;
}

CmdResult dispenserRequest(unsigned long volumeMl, uint8_t mask, const char* cmdId) {
  mask &= 0x03;
  CmdResult pre = checkCanStart(mask);
  if (pre != CmdResult::OK) return pre;
  if (volumeMl == 0 || volumeMl > limMaxDispenseMl()) return CmdResult::BAD_VOLUME;   // SAF-04

  unsigned long now = millis();
  calMode      = false;
  desiredMl    = volumeMl;
  maxRunMs     = logic::maxRunMs(volumeMl, limMinFlow(), TIMEOUT_GRACE_MS);
  strlcpy(activeCmdId, cmdId ? cmdId : "", sizeof(activeCmdId));
  beginJob(mask, now);

  Serial.printf("Dispensing %lu ml per channel (mask %u, max run %lu ms)\n", volumeMl, mask, maxRunMs);
  netPublishStatus();
  return CmdResult::OK;
}

CmdResult dispenserCalRun(uint8_t channel, unsigned long durationMs, const char* cmdId) {
  if (channel != 1 && channel != 2) return CmdResult::BAD_CHANNELS;
  uint8_t mask = 1 << (channel - 1);
  CmdResult pre = checkCanStart(mask);
  if (pre != CmdResult::OK) return pre;

  if (durationMs == 0) durationMs = CAL_DEFAULT_RUN_MS;
  if (durationMs > CAL_MAX_RUN_MS) durationMs = CAL_MAX_RUN_MS;   // bounded, like every run

  unsigned long now = millis();
  calMode        = true;
  calChannel     = channel - 1;
  calDurationMs  = durationMs;
  calPulses[calChannel] = 0;
  desiredMl      = 0;
  maxRunMs       = durationMs + SOLENOID_LEAD_MS + SOLENOID_LAG_MS + TIMEOUT_GRACE_MS;
  strlcpy(activeCmdId, cmdId ? cmdId : "", sizeof(activeCmdId));
  beginJob(mask, now);

  Serial.printf("Calibration run: channel %u for %lu ms. Collect the output in a graduated cylinder.\n",
                channel, durationMs);
  netPublishStatus();
  return CmdResult::OK;
}

CmdResult dispenserCalSetFlow(uint8_t channel, float measuredMl, float* factorOut) {
  if (channel != 1 && channel != 2) return CmdResult::BAD_CHANNELS;
  if (state == DispState::DISPENSING) return CmdResult::BUSY;
  uint8_t i = channel - 1;
  if (calPulses[i] < CAL_MIN_PULSES) return CmdResult::NO_CAL_RUN;   // need a run with enough pulses first
  if (!(measuredMl > 0.0f)) return CmdResult::BAD_VALUE;

  float factor = measuredMl / (float)calPulses[i];
  if (!calSetFlow(i, factor)) return CmdResult::BAD_VALUE;           // implausible: nothing is changed

  ch[0].calib = calFlow(0);                                          // custom profile is now active
  ch[1].calib = calFlow(1);
  resetRunOn();
  if (factorOut) *factorOut = factor;
  Serial.printf("Channel %u calibrated: %.4f ml/pulse (saved, profile custom)\n", channel, factor);
  if (calRunMs[i] > 0) {
    float rate = measuredMl / (calRunMs[i] / 1000.0f);
    Serial.printf("Measured flow about %.1f ml/s. Suggested safety limit: set min_flow %.1f\n",
                  rate, logic::suggestMinFlow(rate));
  }
  calPulses[i] = 0;                                                  // one run, one calibration
  netPublishStatus();
  return CmdResult::OK;
}

void dispenserCalReset() {
  calReset();
  ch[0].calib = calFlow(0);
  ch[1].calib = calFlow(1);
  calPulses[0] = calPulses[1] = 0;
  resetRunOn();
  Serial.println("Calibration reset: standard profile, custom values and TDS factor erased.");
  netPublishStatus();
}

CmdResult dispenserSetCalProfile(CalProfile p) {
  if (state == DispState::DISPENSING) return CmdResult::BUSY;       // never change factors mid-job
  if (!calSetProfile(p)) return CmdResult::NO_CUSTOM;               // custom chosen but none saved
  ch[0].calib = calFlow(0);
  ch[1].calib = calFlow(1);
  calPulses[0] = calPulses[1] = 0;                                   // pending run belonged to the old profile
  resetRunOn();
  Serial.printf("Flow profile: %s (ch1 %.3f, ch2 %.3f ml/pulse)\n",
                calProfileName(p), ch[0].calib, ch[1].calib);
  netPublishStatus();
  return CmdResult::OK;
}

float dispenserFlowFactor(uint8_t channel) { return ch[(channel - 1) & 1].calib; }
unsigned long dispenserLastCalPulses(uint8_t channel) { return calPulses[(channel - 1) & 1]; }
float dispenserRunOnMl(uint8_t channel) { return ch[(channel - 1) & 1].runOnMl; }

void dispenserEstop() { raiseFault(FaultCode::ESTOP, 0); }

CmdResult dispenserAckFault() {
  if (state != DispState::FAULT) return CmdResult::NOTHING_TO_ACK;
  if (fault == FaultCode::ESTOP && estopButtonLatched) return CmdResult::CAUSE_NOT_CLEARED;
  if (fault == FaultCode::TANK_LOW && faultChannel >= 1 &&
      tankLowNow(TANK_SENSOR_FOR_CHANNEL[faultChannel - 1])) return CmdResult::CAUSE_NOT_CLEARED;

  fault = FaultCode::NONE;
  faultChannel = 0;
  state = DispState::IDLE;
  faultStoreClear();
  Serial.println("Fault acknowledged.");
  netPublishStatus();
  return CmdResult::OK;
}

DispState dispenserState()        { return state; }
FaultCode dispenserFault()        { return fault; }
uint8_t   dispenserFaultChannel() { return faultChannel; }

const char* stateName(DispState s) {
  switch (s) {
    case DispState::IDLE:       return "IDLE";
    case DispState::DISPENSING: return calMode ? "CALIBRATING" : "DISPENSING";
    default:                    return "FAULT";
  }
}

const char* faultName(FaultCode f) {
  switch (f) {
    case FaultCode::NO_FLOW:  return "NO_FLOW";
    case FaultCode::TIMEOUT:  return "TIMEOUT";
    case FaultCode::TANK_LOW: return "TANK_LOW";
    case FaultCode::ESTOP:    return "ESTOP";
    default:                  return "NONE";
  }
}

const char* resultName(CmdResult r) {
  switch (r) {
    case CmdResult::OK:                return "OK";
    case CmdResult::BUSY:              return "BUSY";
    case CmdResult::FAULTED:           return "FAULTED";
    case CmdResult::ESTOP_ACTIVE:      return "ESTOP_ACTIVE";
    case CmdResult::BAD_VOLUME:        return "BAD_VOLUME";
    case CmdResult::BAD_CHANNELS:      return "BAD_CHANNELS";
    case CmdResult::TANK_LOW:          return "TANK_LOW";
    case CmdResult::NOTHING_TO_ACK:    return "NOTHING_TO_ACK";
    case CmdResult::CAUSE_NOT_CLEARED: return "CAUSE_NOT_CLEARED";
    case CmdResult::NO_CAL_RUN:        return "NO_CAL_RUN";
    case CmdResult::NO_CUSTOM:         return "NO_CUSTOM";
    default:                           return "BAD_VALUE";
  }
}
