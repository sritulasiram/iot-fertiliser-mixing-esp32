# MQTT Contract (draft v0.1)

Single source of truth for firmware and backend. Payloads are JSON, UTF-8. Change it here first.

Base topic: `fert/{device_id}/…` (`device_id` = `fm-` + last 6 hex digits of the ESP32 MAC).

## Device → cloud

| Topic | QoS | Retained | Payload |
|---|---|---|---|
| `status` | 1 | yes | `{"online":true,"fw":"1.4.0","state":"IDLE","flow_profile":"standard","fault":null}`. Last-will publishes `{"online":false}`. |
| `telemetry` | 0 | no | `{"ts":1767225600,"tds_ppm":812,"temp_c":23.4,"level1":"HIGH","level2":"MEDIUM","level1_raw":2100,"level2_raw":900}`, every 5 s, or on change |
| `event/dispense` | 1 | no | `{"id":"<cmd id>","ts":…,"requested_ml":100,"ch1_ml":99.8,"ch2_ml":101.2,"result":"OK"}`. `result` is `OK` or a fault code. |
| `event/calibration` | 0 | no | `{"id":"<cmd id>","ts":…,"channel":1,"pulses":87,"duration_ms":5000}`, after a calibration run |
| `event/fault` | 1 | no | `{"ts":…,"code":"NO_FLOW","channel":1}` |
| `resp/{cmd id}` | 1 | no | `{"ok":true}` or `{"ok":false,"error":"…"}` |

## Cloud → device

| Topic | QoS | Payload |
|---|---|---|
| `cmd/dispense` | 1 | `{"id":"uuid","volume_ml":100,"channels":[1,2]}` |
| `cmd/estop` | 1 | `{"id":"uuid"}` |
| `cmd/ack_fault` | 1 | `{"id":"uuid"}` |
| `cmd/calibrate_run` | 1 | `{"id":"uuid","channel":1,"duration_s":5}`: run one pump for a fixed time and count pulses |
| `cmd/calibrate` | 1 | `{"id":"uuid","channel":1,"measured_ml":456}` (flow) or `{"id":"uuid","target":"tds","reference_ppm":1000}`. `resp` carries the new `factor` |
| `cmd/calibrate_profile` | 1 | `{"id":"uuid","profile":"standard"}` (or `"high"`, `"custom"`): choose the flow-factor profile |
| `cmd/calibrate_reset` | 1 | `{"id":"uuid"}`: `standard` profile, erase custom values and TDS factor |
| `cmd/config` | 1 | Always rejected with `LOCAL_ONLY`: safety limits and level thresholds can only be changed from the serial console (SAF-12) |

## Rules

- Commands carry a unique `id`; the device ignores duplicates it has already handled.
- The device validates every command against [safety.md](safety.md) and answers on `resp/{id}`.
- Per-device broker credentials; ACL restricts a device to `fert/{its id}/#`.
- TLS required. No credentials in topics or payloads.
- Timestamps: Unix seconds UTC (NTP-synced). If unsynced, `ts` is omitted and the backend stamps receipt time.

## Implementation notes (firmware 1.2.0)

- PubSubClient publishes at QoS 0 only; events are therefore best-effort until the client is swapped for one that supports QoS 1.
- `temp_c` is published only when `ENABLE_DS18B20 1` and the sensor is reading.
- `cmd/config` replies `{"ok":false,"error":"LOCAL_ONLY"}` by design.
- `state` in `status` is `IDLE`, `DISPENSING`, `CALIBRATING` or `FAULT`.
- `resp` errors use the firmware's `CmdResult` names: `BUSY`, `FAULTED`, `ESTOP_ACTIVE`, `BAD_VOLUME`, `BAD_CHANNELS`, `TANK_LOW`, `NOTHING_TO_ACK`, `CAUSE_NOT_CLEARED`, `NO_CAL_RUN`, `BAD_VALUE`, `NO_CUSTOM`.
- `cmd/dispense` `volume_ml` is per channel; the `event/dispense` `result` is `OK` or a fault code.
- Duplicate command ids are remembered for the last 8 commands.
