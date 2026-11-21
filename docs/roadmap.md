# Roadmap and Known Limitations

## Done (firmware 1.5.0)

- Modular firmware (dispenser, level, tds, limits, calibration, net, transports)
- Local safety layer SAF-01…SAF-12 ([safety.md](safety.md))
- Blynk kept; MQTT added as an option ([mqtt-contract.md](mqtt-contract.md))
- Selectable flow-factor profiles, on-device calibration, serial console, MQTT calibration commands
- Optional DS18B20 temperature compensation for TDS (`ENABLE_DS18B20`)
- Adaptive run-on compensation and a final volume that includes flow until the valves close
- Faults that survive a reset; safety limits and level thresholds saved on the device, serial-only to change
- Host-side unit tests (`tests/`) and a GitHub Actions build + test workflow
- Full documentation set and bench-test checklist

## Known limitations

| Limitation | Effect | Planned fix |
|---|---|---|
| New firmware features not yet bench-tested (the legacy prototype was tested on hardware) | Safety, calibration, profile, run-on and MQTT logic unverified in practice | Run [bench-tests.md](bench-tests.md) |
| GitHub Actions workflow never run | The build check may need small fixes on first run | Push, watch the first run |
| Run-on compensation untested on hardware | Effect on accuracy unknown until measured | Bench tests 36-38, then re-measure as in [results.md](results.md) |
| Blynk cannot start a calibration or change limits | Use the serial console (limits are serial-only by design); calibration also over MQTT | Add Blynk pins for calibration if wanted |
| Fixed 23 °C for TDS compensation unless a DS18B20 is fitted | TDS error when water temperature differs | Fit the sensor and set `ENABLE_DS18B20 1` (compile-tested only) |
| MQTT events are QoS 0 | A dropped publish is lost | Switch client library for QoS 1 |
| No OTA updates | Reflash over USB | ArduinoOTA with a strong password and a deliberate security review |
| No offline event log | Results lost if no transport is up | Ring buffer in LittleFS, sync on reconnect |
| Mixing pump is not under firmware control | E-stop does not stop it | By design (it runs continuously); isolate mains separately |
| Valve/pump pairing and GPIO 2 relay need checking against the real wiring | A crossed pairing faults with `NO_FLOW` | [hardware.md](hardware.md) open points, bench test 30 |

## Ideas

- Closed-loop dosing toward a target ppm
- Named recipes and schedules
- Web dashboard and history store (the MQTT interface is designed for this)
- Captive-portal WiFi provisioning instead of `config.h`
