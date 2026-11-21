#pragma once
/*
 * dispenser — dispense state machine, calibration runs and local safety (docs/safety.md).
 * Transport-agnostic: Blynk, MQTT and the serial console all call the functions below.
 */
#include <Arduino.h>
#include "calibration.h"

enum class DispState : uint8_t { IDLE, DISPENSING, FAULT };

enum class FaultCode : uint8_t { NONE, NO_FLOW, TIMEOUT, TANK_LOW, ESTOP };

enum class CmdResult : uint8_t {
  OK, BUSY, FAULTED, ESTOP_ACTIVE, BAD_VOLUME, BAD_CHANNELS,
  TANK_LOW, NOTHING_TO_ACK, CAUSE_NOT_CLEARED,
  NO_CAL_RUN, BAD_VALUE, NO_CUSTOM
};

struct DispenseResult {
  char          id[40];       // command id (empty for Blynk / serial)
  unsigned long requestedMl;  // per channel
  float         ml[2];        // dispensed per channel
  FaultCode     result;       // NONE = completed OK
};

// Outcome of a calibration run: the pump ran for a fixed time and these pulses were counted.
struct CalRunResult {
  char          id[40];
  uint8_t       channel;      // 1 or 2
  unsigned long pulses;
  unsigned long durationMs;
};

void dispenserSafeOutputs();   // SAF-06: call first thing in setup()
void dispenserBegin();         // call after calBegin()
void dispenserLoop();          // call every loop iteration

// mask bit0 = channel 1, bit1 = channel 2. volume is per channel.
CmdResult dispenserRequest(unsigned long volumeMl, uint8_t mask = 0x03, const char* cmdId = "");
void      dispenserEstop();    // SAF-05
CmdResult dispenserAckFault(); // SAF-08

// ── Flow calibration (two steps, like the original MotorTest sketch) ─────────
// 1) dispenserCalRun: run ONE channel for a fixed time (default CAL_DEFAULT_RUN_MS, 0 = default)
//    into a graduated cylinder. Same safety checks as a normal dispense.
// 2) dispenserCalSetFlow: tell it how many ml you actually collected. factor = ml / pulses.
CmdResult dispenserCalRun(uint8_t channel, unsigned long durationMs = 0, const char* cmdId = "");
CmdResult dispenserCalSetFlow(uint8_t channel, float measuredMl, float* factorOut = nullptr);
void      dispenserCalReset(); // standard profile; erases custom values and the TDS factor
CmdResult dispenserSetCalProfile(CalProfile p);   // switch flow profile (standard / high / custom); saved

float         dispenserFlowFactor(uint8_t channel);        // ml per pulse, channel 1 or 2
unsigned long dispenserLastCalPulses(uint8_t channel);     // 0 if no run waiting
float         dispenserRunOnMl(uint8_t channel);           // learned run-on compensation (ml), 0 until learned

DispState dispenserState();
FaultCode dispenserFault();
uint8_t   dispenserFaultChannel();  // 0 = n/a, else 1 or 2

const char* stateName(DispState s);
const char* faultName(FaultCode f);     // "NO_FLOW", ...; "NONE" for NONE
const char* resultName(CmdResult r);
