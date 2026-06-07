# 🌱 IoT-Based Fertiliser Mixing System for Precision Farming

> Research prototype firmware for an ESP32-based automated fertiliser dispensing and monitoring system, developed as part of a published academic study.

---

## 📄 Publication

**Sri Tulasi Ram Rajalingam**, Muhammad Muzakkir Mohd Nadzri, Afandi Ahmad, & Mohamad Khairi Ishak.
*IoT-Based Fertiliser Mixing for Precision Farming.*
**International Journal of Integrated Engineering**, 18(1), 186–201, 2026.
🔗 [https://penerbit.uthm.edu.my/ojs/index.php/ijie/article/view/24386](https://penerbit.uthm.edu.my/ojs/index.php/ijie/article/view/24386)

If you use this code or reference this work, please cite the paper above.

---

## 📌 Overview

This repository contains the ESP32 firmware for a hydroponic fertiliser mixing prototype that:

- Dispenses precise volumes of fertiliser from two independent pump-solenoid circuits
- Monitors **TDS (Total Dissolved Solids)** concentration in real time using a median-filtered ADC pipeline
- Reads **two water level sensors** with colour-coded status indicators
- Streams all sensor data to the **Blynk IoT dashboard** via WiFi
- Uses a **non-blocking state machine** for dispensing — keeping the Blynk connection and watchdog timer healthy throughout

---

## 🛠️ Hardware

| Component | Details |
|---|---|
| Microcontroller | ESP32 (any dev board with ADC1) |
| Pump 1 | GPIO 16 |
| Pump 2 | GPIO 17 |
| Solenoid Valve 1 | GPIO 22 |
| Solenoid Valve 2 | GPIO 23 |
| Flow Sensor 1 | GPIO 18 (interrupt-capable) |
| Flow Sensor 2 | GPIO 19 (interrupt-capable) |
| TDS Sensor | GPIO 36 (ADC1 — VP pin) |
| Water Level Sensor 1 | GPIO 34 |
| Water Level Sensor 2 | GPIO 35 |

> ⚠️ **Note:** TDS sensor must be on ADC1 (GPIO 32–39). ADC2 is unusable while WiFi is active on ESP32.

---

## 📱 Blynk Virtual Pin Mapping

| Virtual Pin | Direction | Description |
|---|---|---|
| V1 | Input | Desired dispense volume (ml) from app |
| V2 | Output | Total volume dispensed (ml) |
| V3 | Output | TDS value (ppm) |
| V4 | Output | Water level sensor 1 (raw ADC + LED colour) |
| V5 | Output | Water level sensor 2 (raw ADC + LED colour) |

---

## ⚙️ Setup

### 1. Dependencies

Install via Arduino Library Manager or PlatformIO:

- [Blynk](https://github.com/blynkkk/blynk-library) `≥ 1.3.0`
- Arduino ESP32 core `≥ 2.0.0`

### 2. Configuration

Open `fertilizer_system.ino` and replace the placeholder values:

```cpp
// WiFi credentials
const char ssid[] = "YOUR_WIFI_SSID";
const char pass[] = "YOUR_WIFI_PASSWORD";

// Blynk
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"
```

### 3. Flow Sensor Calibration

Adjust the calibration factors to match your specific flow sensors (pulses per litre):

```cpp
const float CALIB_FACTOR1 = 5.25;  // Sensor 1 — update after physical calibration
const float CALIB_FACTOR2 = 5.50;  // Sensor 2 — update after physical calibration
```

To calibrate: run each pump for a measured volume, count the pulses via Serial Monitor, then divide `measured_ml / pulse_count`.

### 4. TDS Calibration

```cpp
const float TEMPERATURE      = 23.0;  // Replace with live temperature sensor reading if available
const float TDS_CALIB_FACTOR = 2.3;   // Adjust against a known-concentration reference solution
```

### 5. Water Level Thresholds

```cpp
// In getLevel()
if (value <= 700)  return "Low";     // Red
if (value <= 1200) return "Medium";  // Yellow
return "High";                       // Green
```
Adjust thresholds based on your sensor's output range.

---

## 🔄 System Flow

```
Boot
 └─ WiFi connect → Blynk connect → Pin init → ISR attach → Timer setup
        │
        ▼
Main Loop (non-blocking)
 ├─ Blynk.run()         — maintains cloud connection
 ├─ timer.run()         — triggers readWaterLevels() every 1s
 │                        triggers publishTDS() every 0.8s
 ├─ TDS ADC sample      — reads ADC every 40ms into circular buffer
 └─ updateDispense()    — state machine tick (only active during dispensing)

Dispense Trigger (via Blynk V1)
 └─ startDispense(vol)
      └─ Reset pulse counters (atomic)
      └─ Open solenoids → Start pumps → State = DISPENSING
           └─ updateDispense() polls each pump independently
           └─ Each pump shuts off exactly when its volume target is reached
           └─ Both done → report total to V2 → State = IDLE
```

---

## 🐛 Known Limitations

- Temperature compensation uses a fixed constant (`23.0°C`) — for higher accuracy, integrate a live temperature sensor (e.g. DS18B20)
- Flow sensor calibration factors are hardware-specific and must be measured physically for each unit
- This is a research prototype — not hardened for production deployment

---

## 📜 License

This code is released under the **MIT License** for research and educational use.
See [`LICENSE`](LICENSE) for full terms.

**If you build on this work, please cite the original paper.**

---

## 👤 Author

**Sri Tulasi Ram Rajalingam**
Support Engineer & Researcher
Penang ⇌ Ipoh, Malaysia
