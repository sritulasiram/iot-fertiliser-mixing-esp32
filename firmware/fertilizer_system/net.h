#pragma once
/*
 * net — WiFi plus the enabled transports (Blynk and/or MQTT).
 * Nothing here may block while a dispense is running: network loss never
 * changes pump state (SAF-10).
 */
#include <Arduino.h>
#include "dispenser.h"

void        netBegin();
void        netLoop();

// Fan-out to every enabled transport. All are safe to call when offline.
void        netPublishStatus();                               // state or fault changed
void        netPublishDispense(const DispenseResult& r);      // completed or aborted
void        netPublishFault(FaultCode code, uint8_t channel);
void        netPublishCalRun(const CalRunResult& r);          // calibration run finished

const char* netDeviceId();   // "fm-" + last 3 MAC bytes (hex)
bool        netTimeSynced(); // NTP time valid
