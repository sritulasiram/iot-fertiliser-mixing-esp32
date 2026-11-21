#include "net.h"
#include "settings.h"
#include <WiFi.h>
#include <time.h>

#if USE_BLYNK
void blynkBegin();
void blynkLoop();
void blynkPublishStatus();
void blynkPublishDispense(const DispenseResult& r);
void blynkPublishFault(FaultCode code, uint8_t channel);
void blynkPublishCalRun(const CalRunResult& r);
#endif

#if USE_MQTT
void mqttBegin();
void mqttLoop();
void mqttPublishStatus();
void mqttPublishDispense(const DispenseResult& r);
void mqttPublishFault(FaultCode code, uint8_t channel);
void mqttPublishCalRun(const CalRunResult& r);
#endif

static char deviceId[16] = "fm-000000";

void netBegin() {
  // Last three MAC bytes live in bits 24..47 of the eFuse MAC (byte 0 is the LSB)
  uint32_t tail = (uint32_t)(ESP.getEfuseMac() >> 24) & 0xFFFFFF;
  snprintf(deviceId, sizeof(deviceId), "fm-%06x", (unsigned)tail);
  Serial.printf("Device ID: %s\n", deviceId);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);   // non-blocking: connects in the background

#if USE_MQTT
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  mqttBegin();
#endif
#if USE_BLYNK
  blynkBegin();
#endif
}

void netLoop() {
#if USE_BLYNK
  blynkLoop();
#endif
#if USE_MQTT
  mqttLoop();
#endif
}

void netPublishStatus() {
#if USE_BLYNK
  blynkPublishStatus();
#endif
#if USE_MQTT
  mqttPublishStatus();
#endif
}

void netPublishDispense(const DispenseResult& r) {
#if USE_BLYNK
  blynkPublishDispense(r);
#endif
#if USE_MQTT
  mqttPublishDispense(r);
#endif
  (void)r;
}

void netPublishFault(FaultCode code, uint8_t channel) {
#if USE_BLYNK
  blynkPublishFault(code, channel);
#endif
#if USE_MQTT
  mqttPublishFault(code, channel);
#endif
  (void)code; (void)channel;
}

void netPublishCalRun(const CalRunResult& r) {
#if USE_BLYNK
  blynkPublishCalRun(r);
#endif
#if USE_MQTT
  mqttPublishCalRun(r);
#endif
  (void)r;
}

const char* netDeviceId() { return deviceId; }

bool netTimeSynced() { return time(nullptr) > 1700000000L; }
