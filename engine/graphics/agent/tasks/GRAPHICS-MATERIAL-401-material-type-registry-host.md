---
id: GRAPHICS-MATERIAL-401
title: Add host-side material type registry and stable ids
status: todo
owner: codex
priority: p2
deps: [GRAPHICS-MATERIAL-302]
updated_at: 2026-02-09
evidence: ""
---

## Goal
Add explicit material type registration API to support extensible material classes.

## Scope
- Introduce material type registry (name/id mapping).
- Reserve built-in type ids and support project-defined extensions.
- Expose lookup utilities for serialization/debugging.

## Acceptance Criteria
- [ ] Material type ids are stable across session reloads.
- [ ] Registry can map `typeName <-> typeId`.
- [ ] Standard type uses registry path instead of hardcoded assumptions.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics`
- unit/manual: register additional type and verify deterministic id.

## Expected Files
- `engine/graphics/source/graphics/material/material-type-registry.hpp`
- `engine/graphics/source/graphics/material/material-type-registry.cpp`
- `engine/graphics/source/graphics/material/material-system.cpp`

