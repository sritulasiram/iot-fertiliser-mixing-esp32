# Development History

How the system was built, reconstructed from the prototype sketches in [`legacy/prototypes/`](../legacy/prototypes/) (ordered by file date). It explains why some values in the code are what they are.

| # | Sketch | Date | What it tested | Notes |
|---|---|---|---|---|
| 1 | `01_TDS_esp8266` | Apr 2024 | TDS probe on an ESP8266 (`A0`, 10-bit ADC) with Blynk | First TDS pipeline: 30-sample median filter, 40 ms sampling, 800 ms publish, cubic polynomial, temperature compensation |
| 2 | `02_Motor_relay_test` | Apr 2024 | Switch 2 pumps + 2 solenoid valves on a 5 s on / 5 s off cycle | Confirms loads are driven through **relays**; sets the pin map (16, 17, 22, 23) |
| 3 | `03_flowsensortest` | May 2024 | Flow sensor pulse counting and flow-rate maths | Used a different pin layout (some invalid ESP32 GPIOs): exploratory only |
| 4 | `04_MotorTest_serial_calibration` | May 2024 | Dispense a volume from Serial; **serial calibration routine** | Starting factors 5.0 and 7.0 ml/pulse; calibration = known volume ÷ pulse count |
| 5 | `05_Blynkmotorcontrol` | May 2024 | Dispense from the Blynk app (V1) | Factors 4.5 / 4.5; first Blynk control |
| 6 | `06_TDSwithLevel` | May 2024 | TDS plus two level sensors on ESP32 | Level sensors on GPIO 26/27 (ADC2) |
| 7 | `07_TDSESP32` | May 2024 | TDS moved to ESP32 (12-bit ADC, 3.3 V) | TDS calibration factor 2.165 |
| 8 | `08_Monitoring_TDSwithLEVEL` | Jun 2024 | Monitoring half of the system | Level sensors moved to GPIO 35/34 (ADC1) |
| 9 | `Fertiliser_Mixing_code_old` | Mar 2025 | Integrated dispensing + monitoring (paper prototype, v1.0.0) | Final factors 5.25 / 5.50 ml/pulse, TDS factor 2.3, level thresholds 700 / 1200 |

## Calibration trail

| Quantity | Early | Final (paper) |
|---|---|---|
| Flow factor, channel 1 | 5.0 → 4.5 | 5.25 ml/pulse |
| Flow factor, channel 2 | 7.0 → 4.5 | 5.50 ml/pulse |
| TDS factor | 2.165 | 2.3 |

**Report vs. code.** The *FYP 2 Final Report* (Table 3.17) lists flow factors **4.25** (sensor 1) and **5.25** (sensor 2), and its accuracy tests ([results.md](results.md)) were run with those. The final integrated sketch in this repository (`Fertiliser_Mixing_code_old`) has **5.25** and **5.50**. Both pairs are kept in the firmware as selectable profiles, `standard` (4.25 / 5.25, the report's) and `high` (5.25 / 5.50, the sketch's), because some builds need the higher factors; a calibration of your own units ([calibration.md](calibration.md)) becomes the `custom` profile and is authoritative.

The spread (4.5 to 7.0 ml/pulse between runs and between channels) shows why each unit must be calibrated individually ([calibration.md](calibration.md)).

## Lessons carried into the current firmware

| Observation in the prototypes | Change in v1.2.0 |
|---|---|
| Dispensing ran in a blocking `while` loop, so Blynk and the watchdog starved | Non-blocking state machine (already in v1.1.0) |
| Loop stopped when the *first* channel hit its target (`&&`), so the other under-dispensed | Each channel stops on its own target (v1.1.0) |
| No time limit, no flow check, no E-stop | Safety layer ([safety.md](safety.md)) |
| Level sensors first on ADC2, later moved | ADC1-only rule documented ([hardware.md](hardware.md)) |
| Real WiFi passwords and Blynk tokens hard-coded in every sketch | Secrets live in a git-ignored `config.h`; archived sketches redacted |
| Serial calibration routine existed only in `04_MotorTest_serial_calibration` | Not yet ported: on the [roadmap](roadmap.md) |
