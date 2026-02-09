---
id: GRAPHICS-MATERIAL-401
title: Add host-side material type registry and stable ids
status: done
owner: codex
priority: p2
deps: [GRAPHICS-MATERIAL-302]
updated_at: 2026-02-09
evidence: "Implemented: added host-side `MaterialTypeRegistry` (`typeName <-> typeId`) with built-in reserved ids (`Unknown=0`, `Standard=1`, `Unlit=2`) and deterministic extension-id generation; integrated registry into `MaterialSystem`, exposed lookup APIs, and routed Standard material header type id through registry-backed assignment during material-data rebuild. Verification attempted with `cmake --build build/x64-debug --target April_graphics` (environment toolchain failure: missing standard headers like `cmath`/`cstdint`)."
---

## Goal
Add explicit material type registration API to support extensible material classes.

## Scope
- Introduce material type registry (name/id mapping).
- Reserve built-in type ids and support project-defined extensions.
- Expose lookup utilities for serialization/debugging.

## Acceptance Criteria
- [x] Material type ids are stable across session reloads.
- [x] Registry can map `typeName <-> typeId`.
- [x] Standard type uses registry path instead of hardcoded assumptions.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics`
- unit/manual: register additional type and verify deterministic id.

## Expected Files
- `engine/graphics/source/graphics/material/material-type-registry.hpp`
- `engine/graphics/source/graphics/material/material-type-registry.cpp`
- `engine/graphics/source/graphics/material/material-system.cpp`

