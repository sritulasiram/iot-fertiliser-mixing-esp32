# ADR 0001: Keep Blynk, add MQTT as an optional transport

- Status: accepted
- Date: 2026-11-21

## Context
Blynk is easy to set up and was used in the paper prototype, but it is closed, rate-limited and offers no durable history. Some users want an open protocol and their own backend.

## Decision
Keep Blynk. Add MQTT as a second transport, selected at compile time (`USE_BLYNK`, `USE_MQTT`). Both can run together. Transports are thin: they translate messages and call the same dispenser API.

## Consequences
- Existing Blynk users lose nothing.
- Any MQTT tooling (Node-RED, Home Assistant, Grafana) can integrate through the documented contract.
- Two transports to maintain, so behaviour must stay identical (enforced by routing both through `dispenser.cpp`).
