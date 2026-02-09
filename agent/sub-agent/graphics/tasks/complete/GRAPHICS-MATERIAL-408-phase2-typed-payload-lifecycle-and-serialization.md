---
id: GRAPHICS-MATERIAL-408
title: Phase 2 - Typed payload model, material lifecycle flags, and parameter serialization
status: done
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-407]
updated_at: 2026-02-09
evidence: "build: cmake --build build/vs2022 --config Debug --target April_graphics April_scene (passed)"
---

## Goal
Move from transitional single-payload material data to a modern, type-safe and tool-friendly material architecture.

## Scope
- Introduce typed material payload model (common header + type-specific payload).
- Add per-material update flags and selective GPU upload path.
- Add parameter layout metadata and serialization round-trip contract for material editing/tooling.

## Acceptance Criteria
- [x] Material data model no longer forces all types into `StandardMaterialData` payload.
- [x] MaterialSystem supports selective updates based on per-material change flags.
- [x] At least one material type supports parameter serialize/deserialize round-trip.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene test-suite`
- test: add/extend tests for payload decode correctness and serialization round-trip.

## Expected Files
- `engine/graphics/shader/material/material-data.slang`
- `engine/graphics/source/graphics/material/i-material.hpp`
- `engine/graphics/source/graphics/material/material-system.hpp`
- `engine/graphics/source/graphics/material/material-system.cpp`
- `engine/graphics/source/graphics/material/*-material.*`
- `engine/test/*`
