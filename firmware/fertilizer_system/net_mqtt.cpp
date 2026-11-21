/*
 * MQTT transport (optional: USE_MQTT = 1 in config.h).
 * Implements docs/mqtt-contract.md. Libraries: PubSubClient, ArduinoJson 7.
 */
#include "settings.h"

#if USE_MQTT

#include <WiFi.h>
#include <time.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "dispenser.h"
#include "level.h"
#include "tds.h"
#include "temperature.h"
#include "net.h"

#if MQTT_USE_TLS
  #include <WiFiClientSecure.h>
  #ifndef MQTT_CA_CERT
    #error "MQTT_USE_TLS = 1 requires MQTT_CA_CERT in config.h"
  #endif
  static WiFiClientSecure netClient;
#else
  static WiFiClient netClient;
#endif

static PubSubClient mqtt(netClient);
void mqttPublishStatus();   // defined below; used by connectBroker
static char prefix[32];                 // "fert/fm-xxxxxx/"

constexpr unsigned long TELEMETRY_MS = 5000;

// ─── Helpers ─────────────────────────────────────────────────────────────────
static void publishDoc(const char* suffix, const JsonDocument& doc, bool retained) {
  if (!mqtt.connected()) return;
  char topic[96];
  snprintf(topic, sizeof(topic), "%s%s", prefix, suffix);
  char buf[384];
  size_t n = serializeJson(doc, buf, sizeof(buf));
  if (n == 0 || n >= sizeof(buf)) return;   // too large: drop rather than send truncated JSON
  mqtt.publish(topic, buf, retained);
}

static void addTs(JsonDocument& d) {
  if (netTimeSynced()) d["ts"] = (uint32_t)time(nullptr);   // omitted when unsynced (contract)
}

static void respond(const char* id, bool ok, const char* err, float factor = NAN) {
  char suffix[64];
  snprintf(suffix, sizeof(suffix), "resp/%s", id);
  JsonDocument d;
  d["ok"] = ok;
  if (!ok) d["error"] = err;
  else if (!isnan(factor)) d["factor"] = factor;   // calibration result
  publishDoc(suffix, d, false);
}

static bool validId(const char* id) {
  size_t n = strlen(id);
  if (n == 0 || n >= 40) return false;
  for (size_t i = 0; i < n; i++) {
    char c = id[i];
    if (c == '/' || c == '+' || c == '#') return false;   // keep it a single topic level
  }
  return true;
}

// Ignore command ids we have already handled (QoS 1 can redeliver)
static char seenIds[8][40];
static uint8_t seenNext = 0;
static bool seenBefore(const char* id) {
  for (uint8_t i = 0; i < 8; i++) if (strcmp(seenIds[i], id) == 0) return true;
  return false;
}
static void remember(const char* id) {
  strlcpy(seenIds[seenNext], id, sizeof(seenIds[0]));
  seenNext = (seenNext + 1) % 8;
}

// ─── Incoming commands ───────────────────────────────────────────────────────
static void onMessage(char* topic, byte* payload, unsigned int len) {
  size_t pl = strlen(prefix);
  if (strncmp(topic, prefix, pl) != 0) return;
  const char* sfx = topic + pl;

  JsonDocument d;
  if (deserializeJson(d, payload, len)) { Serial.println("MQTT: bad JSON ignored"); return; }

  const char* id = d["id"] | "";
  if (!validId(id)) { Serial.println("MQTT: command without valid id ignored"); return; }
  if (seenBefore(id)) return;
  remember(id);

  CmdResult r;
  if (!strcmp(sfx, "cmd/dispense")) {
    long vol = d["volume_ml"] | 0L;
    uint8_t mask = 0x03;
    if (d["channels"].is<JsonArray>()) {
      mask = 0;
      for (JsonVariant v : d["channels"].as<JsonArray>()) {
        int c = v.as<int>();
        if (c == 1 || c == 2) mask |= (1 << (c - 1));
      }
    }
    r = (vol <= 0) ? CmdResult::BAD_VOLUME : dispenserRequest((unsigned long)vol, mask, id);
  } else if (!strcmp(sfx, "cmd/estop")) {
    dispenserEstop();
    r = CmdResult::OK;
  } else if (!strcmp(sfx, "cmd/ack_fault")) {
    r = dispenserAckFault();
  } else if (!strcmp(sfx, "cmd/calibrate_run")) {
    int c = d["channel"] | 0;
    float secs = d["duration_s"] | 0.0f;
    r = dispenserCalRun((uint8_t)c, (unsigned long)(secs * 1000.0f), id);
  } else if (!strcmp(sfx, "cmd/calibrate")) {
    float factor = NAN;
    const char* target = d["target"] | "flow";
    if (!strcmp(target, "tds")) {
      float ref = d["reference_ppm"] | 0.0f;
      r = tdsCalibrate(ref, &factor) ? CmdResult::OK : CmdResult::BAD_VALUE;
    } else {
      int c = d["channel"] | 0;
      float ml = d["measured_ml"] | 0.0f;
      r = dispenserCalSetFlow((uint8_t)c, ml, &factor);
    }
    respond(id, r == CmdResult::OK, resultName(r), factor);
    return;
  } else if (!strcmp(sfx, "cmd/calibrate_profile")) {
    CalProfile p;
    r = calParseProfile(d["profile"] | "", &p) ? dispenserSetCalProfile(p) : CmdResult::BAD_VALUE;
  } else if (!strcmp(sfx, "cmd/calibrate_reset")) {
    dispenserCalReset();
    r = CmdResult::OK;
  } else if (!strcmp(sfx, "cmd/config")) {
    respond(id, false, "LOCAL_ONLY");           // safety limits are changed from the serial console only
    return;
  } else {
    respond(id, false, "UNKNOWN_COMMAND");
    return;
  }
  respond(id, r == CmdResult::OK, resultName(r));
}

// ─── Connection ──────────────────────────────────────────────────────────────
static void connectBroker() {
  char willTopic[96];
  snprintf(willTopic, sizeof(willTopic), "%sstatus", prefix);
  const char* user = strlen(MQTT_USER) ? MQTT_USER : nullptr;
  const char* pass = strlen(MQTT_USER) ? MQTT_PASSWORD : nullptr;

  Serial.print("MQTT connecting... ");
  // PubSubClient publishes at QoS 0 only; the will itself can be QoS 1.
  if (mqtt.connect(netDeviceId(), user, pass, willTopic, 1, true, "{\"online\":false}")) {
    Serial.println("OK");
    char sub[48];
    snprintf(sub, sizeof(sub), "%scmd/#", prefix);
    mqtt.subscribe(sub, 1);
    mqttPublishStatus();
  } else {
    Serial.printf("failed (rc=%d)\n", mqtt.state());
  }
}

// ─── Hooks called by net.cpp ─────────────────────────────────────────────────
void mqttBegin() {
  snprintf(prefix, sizeof(prefix), "fert/%s/", netDeviceId());
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
  mqtt.setBufferSize(512);
  mqtt.setKeepAlive(15);
  mqtt.setSocketTimeout(2);
  netClient.setTimeout(2);                 // seconds: bounds a failed connect
#if MQTT_USE_TLS
  netClient.setCACert(MQTT_CA_CERT);
#endif
}

void mqttLoop() {
  static unsigned long lastAttempt = 0, lastTelemetry = 0;
  if (WiFi.status() != WL_CONNECTED) return;
  unsigned long now = millis();

  if (!mqtt.connected()) {
    if (dispenserState() == DispState::DISPENSING) return;   // never block mid-dispense
    if (now - lastAttempt >= 5000UL) { lastAttempt = now; connectBroker(); }
    return;
  }
  mqtt.loop();

  if (now - lastTelemetry >= TELEMETRY_MS) {
    lastTelemetry = now;
    JsonDocument d;
    addTs(d);
    d["tds_ppm"]   = (int)roundf(tdsPpm());
    d["level1"]    = levelName(levelOf(0));
    d["level2"]    = levelName(levelOf(1));
    d["level1_raw"] = levelRaw(0);
    d["level2_raw"] = levelRaw(1);
    if (tempValid()) d["temp_c"] = roundf(tempC() * 10.0f) / 10.0f;   // only with a live DS18B20
    publishDoc("telemetry", d, false);
  }
}

void mqttPublishStatus() {
  JsonDocument d;
  d["online"] = true;
  d["fw"]     = FW_VERSION;
  d["state"]  = stateName(dispenserState());
  d["flow_profile"] = calProfileName(calProfile());
  if (dispenserFault() == FaultCode::NONE) d["fault"] = nullptr;
  else d["fault"] = faultName(dispenserFault());
  publishDoc("status", d, true);
}

void mqttPublishDispense(const DispenseResult& r) {
  JsonDocument d;
  d["id"] = r.id;
  addTs(d);
  d["requested_ml"] = r.requestedMl;
  d["ch1_ml"] = roundf(r.ml[0] * 10.0f) / 10.0f;
  d["ch2_ml"] = roundf(r.ml[1] * 10.0f) / 10.0f;
  d["result"] = (r.result == FaultCode::NONE) ? "OK" : faultName(r.result);
  publishDoc("event/dispense", d, false);
}

void mqttPublishCalRun(const CalRunResult& r) {
  JsonDocument d;
  d["id"] = r.id;
  addTs(d);
  d["channel"]     = r.channel;
  d["pulses"]      = r.pulses;
  d["duration_ms"] = r.durationMs;
  publishDoc("event/calibration", d, false);
}

void mqttPublishFault(FaultCode code, uint8_t channel) {
  JsonDocument d;
  addTs(d);
  d["code"] = faultName(code);
  d["channel"] = channel;
  publishDoc("event/fault", d, false);
}

#endif // USE_MQTT
