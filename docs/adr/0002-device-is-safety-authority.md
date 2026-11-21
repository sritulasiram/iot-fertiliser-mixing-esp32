# ADR 0002: The device is the safety authority

- Status: accepted
- Date: 2026-11-21

## Context
Networks fail. A cloud-side check cannot stop a pump that has lost its connection.

## Decision
All safety limits and interlocks (see docs/safety.md) are enforced in firmware. Remote commands are requests that the device validates.

## Consequences
- Safety behaviour is testable without a backend.
- Backend limits are advisory UX only.
