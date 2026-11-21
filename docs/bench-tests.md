# Bench Tests (firmware 1.2.0)

Run with **water only**, pumps plumbed water-to-water, before connecting nutrients. Watch Serial Monitor (115200). Record pass/fail and date.

| # | Req | Procedure | Expected | Result |
|---|---|---|---|---|
| 1 | SAF-06 | Press reset / power-cycle while pumps are running | Pumps and valves OFF immediately and stay off | |
| 2 | SAF-04 | Request 0, -5, and `MAX_DISPENSE_ML + 1` | Each rejected `BAD_VOLUME`; nothing starts | |
| 3 | SAF-02 | Start a dispense with a flow sensor unplugged | Fault `NO_FLOW` after ~`NO_FLOW_TIMEOUT_MS`; all outputs off | |
| 4 | SAF-02 | Start with a pump's supply tube out of the water | Fault `NO_FLOW`, same as above | |
| 5 | SAF-01 | Temporarily set `NO_FLOW_TIMEOUT_MS` very high, restrict flow to a trickle | Fault `TIMEOUT` at the computed limit | |
| 6 | SAF-03 | Dry the level sensor for a tank, request a dispense | Rejected `TANK_LOW`; nothing starts | |
| 7 | SAF-03 | Start a dispense, then lift the level sensor out of the water | Fault `TANK_LOW` within ~0.5 s | |
| 8 | SAF-05 | During a dispense, send E-stop (Blynk V6 or `cmd/estop`) | Fault `ESTOP`; everything off at once | |
| 9 | SAF-05 | If button fitted (`ENABLE_ESTOP_BUTTON 1`): press during a dispense | Same as above | |
| 10 | SAF-08 | After any fault, request a dispense | Rejected (`FAULTED` / `ESTOP_ACTIVE`) | |
| 11 | SAF-08 | Ack while cause persists (tank still dry, button still pressed) | `CAUSE_NOT_CLEARED` | |
| 12 | SAF-08 | Clear cause, ack | State returns to IDLE; dispense works again | |
| 13 | SAF-09 | Scope or listen: start and finish a dispense | Valve opens ~150 ms before pump; closes ~300 ms after pump stops | |
| 14 | SAF-10 | Start a dispense, then turn off the WiFi router | Dispense still completes (or faults) locally | |
| 15 | SAF-10 | Boot with the router off | Device boots, pumps off, reconnects when WiFi returns | |
| 16 | SAF-07 | (Optional) add `while(true){}` in `loop()` temporarily | Device resets after `WDT_TIMEOUT_S`; outputs off | |
| 17 | Accuracy | Dispense 100 ml into a graduated cylinder, 3 times per channel | Record actual ml; use to tune calibration | |
| 18 | Blynk+MQTT | Enable both, dispense from each | Both see the events; commands validated identically | |
| 19 | MQTT | Send the same `cmd/dispense` id twice | Executed once | |
| 20 | MQTT | Send a command with bad JSON / no id | Ignored, device unaffected | |
| 21 | Calib | `cal run 1`, collect output, `cal set 1 <ml>` | Factor printed; a following 100 ml dispense lands within tolerance | |
| 22 | Calib | Reboot after calibrating; `cal show` | Factor is kept (saved in flash) | |
| 23 | Calib | `cal set 1 500` with no prior run | `NO_CAL_RUN`; nothing changes | |
| 24 | Calib | `cal run 1` with the flow sensor unplugged | Fault `NO_FLOW`; no factor stored | |
| 25 | Calib | `cal set 1 0.001` after a good run (implausible) | `BAD_VALUE`; factor unchanged | |
| 26 | Calib | `cal reset`, `cal show` | Factory defaults restored | |
| 27 | Calib | TDS probe in a reference solution, `cal tds <ppm>` | Reading then matches the reference | |
| 28 | Serial | `dispense 50`, `estop`, `ack`, `status`, `help` | Each behaves as documented | |
| 29 | DS18B20 | With `ENABLE_DS18B20 1`, warm the probe; unplug it | `temp_c` follows; after ~30 s unplugged it falls back to 23 °C | |
| 30 | Wiring | `cal run 1` with water, then `cal run 2` | Flow pulses appear on the matching channel; no `NO_FLOW`. If a channel faults with pump running, the valve/pump pairing is crossed ([hardware.md](hardware.md) open point 1) | |
| 31 | Profile | `cal profile high`, reboot, `cal show` | Profile `high` (5.25 / 5.50) kept after reboot | |
| 32 | Profile | Dispense 100 ml per channel under `standard`, then under `high` | `high` dispenses less volume than `standard`, in proportion to the factors | |
| 33 | Profile | `cal profile custom` before any `cal set` | `NO_CUSTOM`; profile unchanged | |
| 34 | Profile | `cal set` after a run, then `cal profile standard` and back to `custom` | Each switch restores that profile's factors; presets unchanged | |
| 35 | Profile | Send `cal profile ...` or Blynk V10 during a dispense | Rejected `BUSY`; factors unchanged | |
| 36 | Run-on | Dispense 100 ml per channel 5 times in a row, collecting each time | Overshoot shrinks over the first runs and then stays small (about a pulse); `status` shows the learned run-on | |
| 37 | Run-on | Same, with `ENABLE_RUNON_COMP 0` | Larger, steady overshoot (the baseline) | |
| 38 | Final volume | Dispense 100 ml and compare the reported ml with the cylinder | Reported volume matches within a pulse or two (it includes flow until the valve closes) | |
| 39 | Fault persistence | Cause a fault (unplug a flow sensor), then press reset | Boots with "Restored latched fault"; dispense rejected; `ack` after fixing clears it | |
| 40 | Fault persistence | After `ack`, reset again | Boots into IDLE | |
| 41 | Limits | `set min_flow 40`, reset, `limits` | Value kept after reset | |
| 42 | Limits | `set no_flow_ms 10`, `set level_low 5000`, `set level_low 800` with `level_med` 700 | Each rejected with a message; nothing changes | |
| 43 | Limits | `limits reset` | Defaults from `settings.h` restored | |
| 44 | Limits | Send MQTT `cmd/config` | Rejected `LOCAL_ONLY`; no limit changes | |
| 45 | Level | `set level_low` / `level_med` to your measured dry/half/full values | Blynk LED colours and the tank-low interlock follow the new thresholds | |
