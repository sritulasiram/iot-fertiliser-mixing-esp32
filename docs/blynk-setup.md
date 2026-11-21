# Blynk Setup

Needed only if `USE_BLYNK 1`. Uses Blynk IoT (the current Blynk platform, not "Blynk Legacy").

## 1. Create the template

In the Blynk Console, *Developers → Templates → New Template*:

- **Name:** `Fertilizer System Monitoring` (the firmware sends this name)
- **Hardware:** ESP32 · **Connection:** WiFi

Copy the **Template ID** into `BLYNK_TMPL_ID` in `config.h`.

## 2. Datastreams

Create these virtual pin datastreams:

| Pin | Name | Type | Range / unit | Direction |
|---|---|---|---|---|
| V1 | Dispense volume | Integer | 0 – `MAX_DISPENSE_ML` ml | app → device |
| V2 | Total dispensed | Integer | ml | device → app |
| V3 | TDS | Double | 0 – 2000 ppm | device → app |
| V4 | Water level 1 | Integer | 0 – 4095 | device → app |
| V5 | Water level 2 | Integer | 0 – 4095 | device → app |
| V6 | E-stop | Integer | 0 – 1 | app → device |
| V7 | Acknowledge fault | Integer | 0 – 1 | app → device |
| V8 | State | String | none | device → app |
| V9 | Message | String | none | device → app |
| V10 | Flow profile | Integer | 0 – 2 | app → device (echoed back) |

## 3. Dashboard widgets

| Widget | Datastream | Notes |
|---|---|---|
| Numeric Input | V1 | "Send on release". Volume is **per channel** |
| Value Display | V2 | Sum of both channels |
| Gauge | V3 | TDS in ppm |
| LED + Value Display | V4, V5 | The firmware changes the LED colour: red Low, yellow Medium, green High |
| Button (switch) | V6 | Send `1` on press. Make it prominent and red |
| Button (push) | V7 | Send `1` on press: acknowledges a fault |
| Label / Value Display | V8 | `IDLE`, `DISPENSING` or `FAULT` |
| Segmented Switch (3 options) | V10 | `standard` / `high` / `custom`: choose the flow factors (see [calibration.md](calibration.md)). Selecting `custom` before any calibration is saved is rejected and the switch snaps back |
| Label / Value Display | V9 | Last message, such as `FAULT NO_FLOW (ch 1)` or `Dispense rejected: TANK_LOW` |

## 4. Create the device and get the token

*Devices → New Device → From template*. Copy the **Auth Token** into `BLYNK_TOKEN` in `config.h`.

## 5. Behaviour to know about

- The device **rejects** volumes of 0 (silently, treated as a widget reset), negative values, and anything above `MAX_DISPENSE_ML`. Rejections show in V9.
- A second dispense command while one is running is rejected as `BUSY`.
- After any fault the device refuses new dispenses until V7 acknowledges it and the cause has cleared.
- E-stop (V6 = 1) stops everything immediately and latches a fault.
- Changing the flow profile (V10) is refused while a dispense is running.
- If Blynk is offline, dispensing still works locally but results are not sent to the app.
