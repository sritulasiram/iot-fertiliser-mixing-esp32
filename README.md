<div align="center">

# 🌱 IoT-Based Fertiliser Mixing System for Precision Farming

**ESP32 firmware for automated hydroponic fertiliser dispensing and real-time monitoring**

[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![Framework](https://img.shields.io/badge/Framework-Arduino-teal.svg)](https://www.arduino.cc/)
[![Blynk](https://img.shields.io/badge/IoT-Blynk-purple.svg)](https://blynk.io/)
[![Journal](https://img.shields.io/badge/Published-IJIE%202026-orange.svg)](https://penerbit.uthm.edu.my/ojs/index.php/ijie/article/view/24386)

</div>

---

## 📄 Publication

> **Sri Tulasi Ram Rajalingam**, Muhammad Muzakkir Mohd Nadzri, Afandi Ahmad, & Mohamad Khairi Ishak.
> *IoT-Based Fertiliser Mixing for Precision Farming.*
> **International Journal of Integrated Engineering (IJIE)**, Vol. 18(1), pp. 186–201, 2026.
> 🔗 [View Paper](https://penerbit.uthm.edu.my/ojs/index.php/ijie/article/view/24386)

If you use this code or build upon this work, please cite the paper above.

---

## 📌 Overview

This repository contains the ESP32 firmware developed for a hydroponic precision farming prototype that automates fertiliser mixing and delivers real-time monitoring via the Blynk IoT platform.

**Key capabilities:**

| Feature | Detail |
|---|---|
| 💧 Dual-pump dispensing | Two independent pump-solenoid circuits with flow feedback |
| 📊 TDS monitoring | Real-time ppm readings via 30-point median-filtered ADC pipeline |
| 🪣 Water level sensing | Two sensors with colour-coded Low / Medium / High indicators |
| 📱 Remote dashboard | Live data streamed to Blynk app over WiFi |
| ⚙️ Non-blocking design | State machine architecture — WDT-safe, Blynk connection maintained |

---

## 🗂️ Repository Structure

```
iot-fertiliser-mixing-esp32/
├── fertilizer_system.ino   — Main firmware (Arduino sketch)
├── config.h.example        — Credentials template (copy → config.h)
├── .gitignore              — Excludes config.h and build artifacts
├── LICENSE                 — MIT License
└── README.md               — This file
```

> `config.h` is **excluded from version control** — your credentials stay local.

---

## 🛠️ Hardware

### Components & Pin Mapping

| Component | GPIO | Notes |
|---|---|---|
| Microcontroller | — | ESP32 (any dev board with ADC1) |
| Pump 1 | 16 | OUTPUT |
| Pump 2 | 17 | OUTPUT |
| Solenoid Valve 1 | 22 | OUTPUT |
| Solenoid Valve 2 | 23 | OUTPUT |
| Flow Sensor 1 | 18 | INPUT\_PULLUP, interrupt-capable |
| Flow Sensor 2 | 19 | INPUT\_PULLUP, interrupt-capable |
| TDS Sensor | 36 | ADC1 — VP pin |
| Water Level Sensor 1 | 34 | ADC1 |
| Water Level Sensor 2 | 35 | ADC1 |

> ⚠️ **ADC Note:** All analog sensors must use **ADC1** (GPIO 32–39).
> ADC2 shares silicon with the WiFi radio and returns unreliable readings when WiFi is active.

---

## 📱 Blynk Dashboard — Virtual Pin Mapping

| Pin | Direction | Widget | Description |
|---|---|---|---|
| V1 | Input | Numeric Input | Desired dispense volume (ml) |
| V2 | Output | Value Display | Total volume dispensed (ml) |
| V3 | Output | Gauge / Value Display | TDS reading (ppm) |
| V4 | Output | LED + Value Display | Water level sensor 1 |
| V5 | Output | LED + Value Display | Water level sensor 2 |

---

## ⚙️ Setup & Configuration

### 1. Install Dependencies

Via **Arduino Library Manager** or **PlatformIO**:

| Library | Version |
|---|---|
| [Blynk](https://github.com/blynkkk/blynk-library) | ≥ 1.3.0 |
| [Arduino ESP32 Core](https://github.com/espressif/arduino-esp32) | ≥ 2.0.0 |

### 2. Configure Credentials

```bash
# Copy the template
cp config.h.example config.h
```

Then open `config.h` and fill in your values:

```cpp
#define WIFI_SSID      "your_wifi_name"
#define WIFI_PASS      "your_wifi_password"
#define BLYNK_TMPL_ID  "your_template_id"
#define BLYNK_TOKEN    "your_auth_token"
```

> 🔒 `config.h` is listed in `.gitignore` — it will never be committed.

### 3. Calibrate Flow Sensors

Each flow sensor has a unique pulses-per-ml factor. To find yours:

1. Run the pump for a **known volume** (e.g. 100 ml measured in a graduated cylinder)
2. Read the pulse count from Serial Monitor
3. Calculate: `CALIB_FACTOR = measured_ml / pulse_count`
4. Update in the sketch:

```cpp
const float CALIB_FACTOR1 = 5.25;  // Sensor 1
const float CALIB_FACTOR2 = 5.50;  // Sensor 2
```

### 4. Calibrate TDS Sensor

```cpp
const float TEMPERATURE      = 23.0f;  // °C — use DS18B20 for live readings
const float TDS_CALIB_FACTOR = 2.3f;   // Tune against a known reference solution (e.g. 1413 µS/cm)
```

### 5. Adjust Water Level Thresholds

Default ADC thresholds (0–4095 range on ESP32):

```cpp
if (value <= 700)  return "Low";     // → Red LED
if (value <= 1200) return "Medium";  // → Yellow LED
return "High";                       // → Green LED
```

Tune these values by reading your sensor dry, half-submerged, and fully submerged via Serial Monitor.

---

## 🔄 System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                        BOOT                             │
│   WiFi → Blynk → Pin init → ISR attach → Timer setup   │
└────────────────────────┬────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│                    MAIN LOOP                            │
│                                                         │
│  Blynk.run() ──── maintains cloud connection            │
│  timer.run() ──── readWaterLevels()  every 1000 ms      │
│               └── publishTDS()       every  800 ms      │
│  ADC sample  ──── TDS circular buffer every  40 ms      │
│  updateDispense() state machine tick (when DISPENSING)  │
└────────────────────────┬────────────────────────────────┘
                         │
          ┌──────────────▼──────────────┐
          │   DISPENSE STATE MACHINE    │
          │                             │
          │  IDLE ──[V1 trigger]──►     │
          │                             │
          │  DISPENSING                 │
          │   ├─ Atomic ISR read        │
          │   ├─ Pump 1: off when done  │
          │   ├─ Pump 2: off when done  │
          │   └─ Both done → V2 → IDLE  │
          └─────────────────────────────┘
```

---

## 🐛 Known Limitations

- **Fixed temperature compensation** — `TEMPERATURE` is a constant (`23.0°C`). Integrating a DS18B20 temperature sensor would improve TDS accuracy in variable environments.
- **Hardware-specific calibration** — `CALIB_FACTOR1` / `CALIB_FACTOR2` must be measured for each physical unit; the defaults are from the paper prototype only.
- **Research prototype** — not hardened for unattended outdoor or production deployment. No OTA updates, no persistent error logging.
- **Single Blynk session** — a second dispense command while one is in progress is silently ignored; no queuing.

---

## 📜 License

Released under the **MIT License** — see [`LICENSE`](LICENSE) for full terms.

**Academic use:** If you build on this work, please cite the original paper.

---

## 👤 Author

**Sri Tulasi Ram**
