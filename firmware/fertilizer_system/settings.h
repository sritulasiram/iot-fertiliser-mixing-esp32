#pragma once
/*
 * settings.h — pins, calibration and safety limits.
 * Secrets and feature switches live in config.h (see config.h.example).
 */
#include <Arduino.h>
#include "config.h"

#define FW_VERSION "1.5.0"

// ─── Feature defaults (config.h may override) ────────────────────────────────
#ifndef USE_BLYNK
#define USE_BLYNK 1
#endif
#ifndef USE_MQTT
#define USE_MQTT 0
#endif
#ifndef MQTT_USE_TLS
#define MQTT_USE_TLS 0
#endif
#ifndef ENABLE_ESTOP_BUTTON
#define ENABLE_ESTOP_BUTTON 0
#endif
#ifndef ENABLE_RUNON_COMP
#define ENABLE_RUNON_COMP 1  // 1 = learn pump run-on and stop early so the final volume hits the target
#endif
#ifndef ENABLE_DS18B20
#define ENABLE_DS18B20 0     // 1 = live water temperature for TDS (needs OneWire + DallasTemperature)
#endif

// ─── Pins ────────────────────────────────────────────────────────────────────
#define PUMP1_PIN          16
#define PUMP2_PIN          17
#define SOLENOID1_PIN      22
#define SOLENOID2_PIN      23
#define FLOW_SENSOR1_PIN   18
#define FLOW_SENSOR2_PIN   19
#define TDS_SENSOR_PIN     36   // ADC1 only; ADC2 is unusable with WiFi
#define WATER_SENSOR1_PIN  34
#define WATER_SENSOR2_PIN  35
#define ESTOP_BUTTON_PIN   27   // used only if ENABLE_ESTOP_BUTTON = 1
#define DS18B20_PIN         4   // used only if ENABLE_DS18B20 = 1 (4.7k pull-up to 3.3 V)

// ─── Calibration defaults ────────────────────────────────────────────────────
// Presets only. The device boots into the "standard" profile unless you chose another
// (the choice is saved). Calibrate your own hardware with the serial console or MQTT:
// the result is saved on the device as the "custom" profile. See docs/calibration.md.
// Flow-factor PROFILES (ml per pulse). Switch at runtime: serial "cal profile <name>",
// MQTT cmd/calibrate_profile, or Blynk V10. A measured calibration becomes the CUSTOM profile.
constexpr float FLOW_STANDARD_1 = 4.25f;   // "standard": values used in the project report's tests
constexpr float FLOW_STANDARD_2 = 5.25f;
constexpr float FLOW_HIGH_1     = 5.25f;   // "high": values in the final prototype sketch
constexpr float FLOW_HIGH_2     = 5.50f;
constexpr float TDS_CALIB_DEFAULT = 2.3f;

constexpr unsigned long CAL_DEFAULT_RUN_MS = 5000;   // calibration run length
constexpr unsigned long CAL_MAX_RUN_MS     = 15000;
constexpr unsigned long CAL_MIN_PULSES     = 10;     // refuse to calibrate from fewer pulses
constexpr float CAL_MIN_ML_PER_PULSE       = 0.1f;   // plausibility range for a result
constexpr float CAL_MAX_ML_PER_PULSE       = 50.0f;
constexpr float CAL_MIN_TDS_FACTOR         = 0.1f;
constexpr float CAL_MAX_TDS_FACTOR         = 20.0f;

// Used for TDS compensation when no live temperature is available
constexpr float TEMPERATURE_FALLBACK_C = 23.0f;

// ─── Safety limits (docs/safety.md) ──────────────────────────────────────────
// DEFAULTS. The first four can be changed at runtime from the serial console ("set min_flow 40")
// and are then saved in flash (see safety_limits.h). Tune to your pumps and plumbing before unattended use.
constexpr unsigned long MAX_DISPENSE_ML    = 500;   // SAF-04: per channel, per command
constexpr float         MIN_FLOW_ML_PER_S  = 1.0f;  // SAF-01: slowest acceptable flow
constexpr unsigned long TIMEOUT_GRACE_MS   = 5000;  // SAF-01: added to computed max run time
constexpr unsigned long NO_FLOW_TIMEOUT_MS = 4000;  // SAF-02: no pulses while pump on
constexpr unsigned long SOLENOID_LEAD_MS   = 150;   // SAF-09: valve opens this long before pump
constexpr unsigned long SOLENOID_LAG_MS    = 300;   // SAF-09: valve closes this long after pump
constexpr unsigned long FLOW_DEBOUNCE_US   = 1000;  // ignore pulses closer than this
constexpr float         RUNON_MAX_ML       = 20.0f; // upper bound on the learned run-on compensation
constexpr int           WDT_TIMEOUT_S      = 8;     // SAF-07

// ─── Water level (raw ADC, 0-4095) ───────────────────────────────────────────
// Defaults (the paper's ranges 0-700, 701-1200, 1201+); changeable with "set level_low/level_med".
constexpr int           LEVEL_LOW_MAX      = 700;   // <= Low
constexpr int           LEVEL_MED_MAX      = 1200;  // <= Medium, above = High
constexpr unsigned long LEVEL_SAMPLE_MS    = 100;
constexpr uint8_t       LEVEL_LOW_CONFIRM  = 5;     // consecutive Low samples to stop a running pump

// Which level sensor guards which pump's source tank. Per the paper, the sensors monitor the
// fertiliser AB level in reservoirs A and B: sensor 1 (GPIO 34) = A = pump 1, sensor 2 (GPIO 35) = B = pump 2.
constexpr uint8_t TANK_SENSOR_FOR_CHANNEL[2] = {0, 1};
