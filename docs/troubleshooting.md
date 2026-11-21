# Troubleshooting

## Build and upload

| Symptom | Cause / fix |
|---|---|
| `config.h: No such file` | Copy `config.h.example` to `config.h` in the sketch folder |
| `BlynkSimpleEsp32.h: No such file` | Install the **Blynk** library, or set `USE_BLYNK 0` |
| `PubSubClient.h` / `ArduinoJson.h` not found | Install them, or set `USE_MQTT 0` |
| `#error MQTT_USE_TLS = 1 requires MQTT_CA_CERT` | Add the broker's CA certificate to `config.h` or set `MQTT_USE_TLS 0` |
| Upload stuck at "Connecting…" | Hold **BOOT** while uploading; try another USB cable (some are charge-only) |
| Brownout resets during upload or pump start | Weak USB supply or pumps sharing the ESP32 supply. Use a separate load supply |

## Connectivity

| Symptom | Cause / fix |
|---|---|
| Blynk never connects | 2.4 GHz WiFi only. Check `BLYNK_TMPL_ID` and `BLYNK_TOKEN` match the device. Template name must be `Fertilizer System Monitoring` |
| Blynk shows offline but Serial works | Wait up to 5 s per retry. Retries are paused while dispensing |
| MQTT `failed (rc=-2)` | Broker host/port unreachable |
| MQTT `failed (rc=5)` | Bad username/password |
| MQTT connects but no commands arrive | The device subscribes to `fert/<id>/cmd/#`. Check the id in Serial and that commands carry a unique `id` field |
| Same command runs only once | Intended: duplicate ids are ignored. Use a fresh id each time |

## Dispensing

| Symptom | Cause / fix |
|---|---|
| Fault `NO_FLOW` right after start | Flow sensor unplugged or not wired to the right GPIO, pump not primed, supply hose out of liquid, or `NO_FLOW_TIMEOUT_MS` too short for your priming time |
| Fault `TIMEOUT` | Flow is slower than `MIN_FLOW_ML_PER_S`. Raise the pump speed or lower the setting |
| Fault `TANK_LOW` | Level reads Low for the channel's tank. Check `TANK_SENSOR_FOR_CHANNEL` and the Low threshold in `settings.h` |
| Rejected `BAD_VOLUME` | 0, negative, or above `MAX_DISPENSE_ML` |
| Rejected `FAULTED` / `ESTOP_ACTIVE` | Acknowledge the fault first (V7 or `cmd/ack_fault`) |
| `CAUSE_NOT_CLEARED` on ack | Tank still reads Low, or the E-stop button is still pressed |
| Device boots straight into `FAULT` | A fault was latched before the reset and is restored on purpose (SAF-11). Fix the cause, then `ack` |
| `set` is rejected | Value out of range, or `level_low` not below `level_med`: the message says which. Nothing was changed |
| Dispensed volume is off by a few ml | One flow pulse is about 5 ml, and pumps coast after switch-off. The firmware learns the run-on over the first few dispenses, so the first one can be a few ml over. If it persists, recalibrate ([calibration.md](calibration.md)) |
| Volume consistently wrong by a fixed % | Calibration factor is wrong for that channel |
| Device sits in `FAULT: ESTOP` after boot | `ENABLE_ESTOP_BUTTON 1` but no normally-closed button wired. Set it to 0 or wire the button |

## Calibration

| Symptom | Cause / fix |
|---|---|
| `NO_CUSTOM` on `cal profile custom` | No calibration saved yet. Run `cal run` / `cal set` first, or choose `standard` or `high` |
| Volume systematically too high or too low | Try the other profile (`cal profile high` lowers the volume); for a real fix, calibrate your own sensors |
| `NO_CAL_RUN` on `cal set` | Run `cal run <channel>` first. Each run supports one `cal set` |
| `BAD_VALUE` on `cal set` | Result outside 0.1–50 ml/pulse, fewer than 10 pulses, or measured ml ≤ 0. Check the measurement, run longer |
| `cal run` ends with `NO_FLOW` | Flow sensor unplugged or pump not primed; nothing was stored |
| `cal tds` says BAD_VALUE | Probe not in solution (reading under 10 ppm) or reference ≤ 0 |
| Calibration lost after reflashing | Erasing all flash in the IDE (Tools → Erase All Flash) wipes it; recalibrate |
| `temp_c` missing in MQTT telemetry | `ENABLE_DS18B20` is 0, or the sensor/pull-up is not working. TDS then uses a fixed 23 °C |

## Sensors

| Symptom | Cause / fix |
|---|---|
| TDS reads 0 or garbage | Sensor on an ADC2 pin (use ADC1 GPIO 32–39), probe not submerged, poor ground |
| TDS drifts | Fixed 23 °C is assumed; real temperature differs. Calibrate at the operating temperature |
| Level colour flickers | Splashing or a noisy probe: adjust thresholds in `settings.h` |
| Analog readings erratic when WiFi is on | A sensor is on ADC2. Move it to ADC1 |

## Serial log cheat-sheet

| Line | Meaning |
|---|---|
| `Dispensing 50 ml per channel (mask 3, max run … ms)` | Request accepted |
| `Pumps started.` | Valve lead time elapsed |
| `Channel 1 done: 51.3 ml` | That channel reached its target |
| `Dispense complete: ch1 …, ch2 …` | Both done |
| `FAULT: NO_FLOW (channel 1)` | Fault latched. All outputs off |
| `Fault acknowledged.` | Back to IDLE |
