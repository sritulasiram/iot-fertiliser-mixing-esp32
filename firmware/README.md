# Firmware

ESP32 firmware built and flashed with the **Arduino IDE**. Version 1.5.0: Blynk kept, MQTT as an option, local safety layer, selectable flow-factor profiles and on-device calibration, serial console, optional DS18B20.

> **Compiles; its new features are not yet bench-tested.** The legacy prototype it builds on (same pins, sensors and dispensing logic) was tested on the actual hardware: see [results.md](../docs/results.md). Built with `arduino-cli` 1.6.0, esp32 core 4.0.0 (board `esp32:esp32:esp32`), Blynk 1.4.0, PubSubClient 2.8, ArduinoJson 7.4.3, OneWire + DallasTemperature, in several `USE_BLYNK` / `USE_MQTT` / `ENABLE_DS18B20` combinations (about 74-79% of flash, no warnings from this code). It has never run on hardware. Work through [docs/bench-tests.md](../docs/bench-tests.md) with water only before connecting nutrient reservoirs.

## Layout

```
firmware/fertilizer_system/        ← open this folder in Arduino IDE
├── fertilizer_system.ino   setup()/loop(), watchdog
├── config.h.example        copy to config.h (git-ignored): WiFi, transports, secrets
├── settings.h              pins, calibration, SAFETY LIMITS
├── dispenser.h/.cpp        dispense + calibration-run state machine, all safety logic
├── calibration.h/.cpp      flow-factor profiles + TDS factor, saved in flash (NVS)
├── safety_limits.h/.cpp    safety limits + level thresholds, saved in flash, serial-only to change
├── faultstore.h/.cpp       latched fault remembered across resets
├── logic.h                 pure functions, unit-tested on a PC (../../tests)
├── serial_console.h/.cpp   text commands over USB serial (works offline)
├── level.h/.cpp            water level sampling, tank-low detection
├── tds.h/.cpp              TDS sampling, conversion and calibration
├── temperature.h/.cpp      optional DS18B20 (ENABLE_DS18B20)
├── net.h/.cpp              WiFi + fan-out to enabled transports
├── net_blynk.cpp           Blynk transport   (USE_BLYNK)
└── net_mqtt.cpp            MQTT transport    (USE_MQTT)
```

## Arduino IDE setup

1. Install **Arduino IDE 2.x**.
2. Boards Manager → add `https://espressif.github.io/arduino-esp32/package_esp32_index.json`, install **esp32 by Espressif Systems** (2.0.x or 3.x).
3. Board: *ESP32 Dev Module* (or your board).
4. Library Manager, install what your transports need:

   | Transport | Libraries |
   |---|---|
   | Blynk (`USE_BLYNK 1`) | **Blynk** ≥ 1.3.0 |
   | MQTT (`USE_MQTT 1`) | **PubSubClient** (Nick O'Leary), **ArduinoJson** 7.x |
   | DS18B20 (`ENABLE_DS18B20 1`) | **OneWire**, **DallasTemperature** |

5. Copy `config.h.example` to `config.h`, then set WiFi and choose transports:
   ```cpp
   #define USE_BLYNK 1   // Blynk app
   #define USE_MQTT  0   // MQTT broker (set to 1 to enable; both can be on)
   ```
6. Open Serial Monitor at 115200 baud.

With both transports on, every event goes to both. Commands from either are validated by the same safety code.

## Blynk virtual pins

| Pin | Dir | Description |
|---|---|---|
| V1 | in | Desired volume per channel (ml). Rejected if 0, negative or above `MAX_DISPENSE_ML` |
| V2 | out | Total dispensed, both channels (ml) |
| V3 | out | TDS (ppm) |
| V4, V5 | out | Water level 1 / 2 (raw value, LED colour) |
| **V6** | in | **E-stop button** (send 1; use a push/switch widget) |
| **V7** | in | **Acknowledge fault** (send 1) |
| **V8** | out | State: `IDLE`, `DISPENSING`, `CALIBRATING`, `FAULT` (Label/Value widget) |
| **V9** | out | Message / fault text (Label/Value widget) |
| **V10** | in/out | Flow-factor profile: 0 standard, 1 high, 2 custom |

V6–V10 are new: add them to your Blynk template.

## Serial console

Type `help` in the Serial Monitor (115200 baud). Works with no network.

```
status                       state, sensors, calibration
dispense <ml> [1|2]          dispense ml per channel
estop / ack                  emergency stop / acknowledge a fault
cal run <1|2> [seconds]      run one pump to count flow pulses
cal set <1|2> <measured_ml>  save ml/pulse from the volume you collected
cal tds <reference_ppm>      calibrate TDS against a reference solution
cal profile <standard|high|custom>   choose flow factors (4.25/5.25, 5.25/5.50, or your own)
cal show / cal reset
limits / limits reset        show or reset the safety limits
set <key> <value>            max_dispense_ml, min_flow, no_flow_ms, level_low, level_med (serial only)
```

Calibration walk-through: [docs/calibration.md](../docs/calibration.md).

## MQTT

Topics and payloads: [docs/mqtt-contract.md](../docs/mqtt-contract.md). Quick test with Mosquitto (replace the device id with the one printed at boot):

```bash
mosquitto_sub -h broker.local -t 'fert/fm-a1b2c3/#' -v
mosquitto_pub -h broker.local -t 'fert/fm-a1b2c3/cmd/dispense' -m '{"id":"t1","volume_ml":50}'
mosquitto_pub -h broker.local -t 'fert/fm-a1b2c3/cmd/estop' -m '{"id":"t2"}'
```

## Safety limits to tune

All in [`settings.h`](fertilizer_system/settings.h). The defaults are **placeholders**: set them from your actual pump flow rate before unattended use.

| Setting | Default | Purpose |
|---|---|---|
| `MAX_DISPENSE_ML` | 500 | Largest accepted request (per channel) |
| `MIN_FLOW_ML_PER_S` | 1.0 | Slowest acceptable flow; sets the run-time limit |
| `NO_FLOW_TIMEOUT_MS` | 4000 | Stop if the pump is on and no pulses arrive |
| `TANK_SENSOR_FOR_CHANNEL` | {0, 1} | Which level sensor guards which pump's tank (assumed 1:1) |

Requirements and fault codes: [docs/safety.md](../docs/safety.md).

## Known limitations of this release

- None of the new features has run on hardware yet (run [bench-tests.md](../docs/bench-tests.md)).
- MQTT uses PubSubClient: publishes are QoS 0 (the contract asks for QoS 1 on events).
- DS18B20 support is compile-tested only. Without the sensor, TDS temperature is fixed at 23 °C.
- No OTA, no offline event log. See [docs/roadmap.md](../docs/roadmap.md).

The original sketches are in [`../legacy/`](../legacy/).
