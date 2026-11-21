# Contributing

## Branches and commits
- `main` is always buildable. Work on feature branches: `feat/…`, `fix/…`, `docs/…`.
- Use [Conventional Commits](https://www.conventionalcommits.org/): `feat(firmware): add no-flow fault`.
- Changes touching dosing or safety logic need a test and an update to [docs/safety.md](docs/safety.md).

## Secrets
Never commit `config.h`, `.env`, certificates or tokens. Use the `*.example` templates.

## Docs
Significant design decisions get an ADR in [docs/adr/](docs/adr/) (copy `0000-template.md`).
Keep [docs/mqtt-contract.md](docs/mqtt-contract.md) in sync with firmware and backend; it is the single source of truth for the interface.

## Hardware safety
Dosing hardware moves liquids and mains-adjacent power. Test new firmware with water and pumps disconnected from nutrient reservoirs first.

## Tests and CI
Pure logic lives in `firmware/fertilizer_system/logic.h` and is tested on a PC:

```bash
c++ -std=c++17 -Wall -Wextra -I firmware/fertilizer_system tests/test_logic.cpp -o /tmp/test_logic && /tmp/test_logic
```

GitHub Actions (`.github/workflows/firmware.yml`) runs those tests and compiles the sketch for four Blynk/MQTT/DS18B20 combinations on every push. Put new decision logic in `logic.h` with a test, and keep Arduino calls out of it.

## Charts and diagrams
Result charts are generated from the data in `docs/results.md` by `python3 tools/make_charts.py` (no dependencies). Edit the data there, then regenerate. Flowcharts and state diagrams are Mermaid blocks inside the Markdown, which GitHub renders; keep labels quoted.

## Publishing this repository
The local history was cleaned of credentials. If an older copy of this project is already on GitHub, its history may still contain real WiFi passwords and Blynk tokens: **revoke those tokens first**, then decide whether to replace the remote history (a force-push that rewrites it) or start a fresh repository. Do not push until you have checked `git log -p` for anything secret.
