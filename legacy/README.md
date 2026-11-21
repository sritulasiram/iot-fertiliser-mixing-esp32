# Legacy: prototype history

Frozen copies of the original development sketches, kept for reproducibility of the published work. **Do not modify. Fixes go in `firmware/`.** The story of how they fit together is in [docs/development-history.md](../docs/development-history.md).

| Path | What it is |
|---|---|
| `fertilizer_system/` | v1.1.0: non-blocking dispense state machine, credentials in `config.h` |
| `Fertiliser_Mixing_code_old/` | v1.0.0: integrated paper prototype (blocking dispense loop) |
| `prototypes/` | The eight earlier test sketches (TDS, relays, flow sensor, serial calibration, level sensors) |
| `README.original.md` | The original project README |

**Credentials:** the original sketches had WiFi passwords and Blynk tokens written into the code. They were replaced with `YOUR_…` placeholders before being added to this repo. Treat the original values as exposed and revoke them.

These sketches are **not** meant to be run as they are: they target a mix of boards (one is ESP8266) and pin layouts.
