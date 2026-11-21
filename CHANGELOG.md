# Changelog

Format: [Keep a Changelog](https://keepachangelog.com/). Versioning: [SemVer](https://semver.org/).

## [Unreleased]
### Added
- Firmware 1.5.0:
  - Adaptive run-on compensation (learns post-switch-off flow and stops early); a dispense completes only after the valves close, so the reported volume and calibration pulse count include run-on flow. `ENABLE_RUNON_COMP`.
  - Faults persist across resets (SAF-11). Safety limits and level thresholds saved on the device and changeable from the serial console only (`set`, `limits`; SAF-12); MQTT `cmd/config` always answers `LOCAL_ONLY`.
  - `cal set` prints the measured flow rate and a suggested `min_flow`.
  - Pure logic split into `logic.h` with host-side unit tests (`tests/`); GitHub Actions workflow builds and tests.
  - Prototype photo, BibTeX citation, roadmap and contributing/publishing notes.
  - Five result charts (SVG, light/dark aware) generated from the measured data by `tools/make_charts.py`, embedded in `docs/results.md`.
  - Mermaid diagrams: state machine, dispense flowchart (architecture), fault handling (safety), calibration decision flow, bring-up flow (getting started), request flow (README).
- Firmware 1.4.0:
  - Selectable flow-factor profiles `standard` (4.25 / 5.25), `high` (5.25 / 5.50) and `custom` (from your own calibration), saved on the device; switch from Serial (`cal profile`), MQTT (`cmd/calibrate_profile`) or Blynk (V10). `flow_profile` added to the MQTT status; new result `NO_CUSTOM`. Default is `standard`.
- Firmware 1.3.0:
  - Flow and TDS calibration stored in flash; two-step flow calibration (`cal run`, `cal set`) ported from the original `MotorTest` sketch; MQTT `cmd/calibrate_run`, `cmd/calibrate`, `cmd/calibrate_reset`, `event/calibration`.
  - Serial console (`help`, `status`, `dispense`, `estop`, `ack`, `cal ...`).
  - Optional DS18B20 water temperature for TDS (`ENABLE_DS18B20`, off by default); `temp_c` in MQTT telemetry when live.
  - New state `CALIBRATING`; new result codes `NO_CAL_RUN`, `BAD_VALUE`.
- Firmware 1.2.0 (`firmware/fertilizer_system/`), compiles in all transport combinations (esp32 core 4.0.0), not yet bench-tested:
  - Safety layer: SAF-01 timeout, SAF-02 no-flow, SAF-03 tank-low interlock, SAF-04 volume limit, SAF-05 E-stop, SAF-06 safe boot, SAF-07 watchdog, SAF-08 latched faults with ack, SAF-09 valve lead/lag.
  - Non-blocking WiFi/Blynk/MQTT connection; network loss no longer affects pumps (SAF-10).
  - Optional MQTT transport (`USE_MQTT`) alongside Blynk (`USE_BLYNK`), per the MQTT contract.
  - New Blynk pins V6 (E-stop), V7 (ack), V8 (state), V9 (message).
  - Spinlock-protected, debounced flow ISRs; TDS buffer pre-filled at boot.
- `docs/bench-tests.md`.
- Documentation set: getting started, architecture, Blynk setup, troubleshooting, roadmap, ADRs.
- Rewritten README.
- Circuit diagram (Fritzing, report Fig. 3.32) used as the hardware image; wiring table decoded from the Fritzing project; mixing pump documented as continuous; level-sensor placement taken from the paper; paper DOI added; bench test 30 for valve/pump pairing.
- Real hardware documented from the FYP report: parts and specifications (`hardware/bom.csv`), system layout and flowchart images, Fritzing project, relay and solenoid notes; `docs/results.md` with the measured accuracy and response-time data.
- `legacy/prototypes/`: the eight earlier test sketches, credentials redacted; `docs/development-history.md`.
- Repository layout: `firmware/`, `hardware/`, `docs/`, `legacy/`.
- Design docs: architecture, safety requirements, MQTT contract, hardware, calibration, roadmap, ADRs.
- `.gitignore`.

### Changed
- v1 sketches and original README moved to `legacy/`. Credentials hard-coded in the v1.0.0 sketch were replaced with placeholders.

## [1.1.0] - legacy
Non-blocking dispense state machine, atomic ISR reads, per-pump shutoff, credentials in `config.h`.

## [1.0.0] - legacy
Initial prototype (paper submission version).
