---
id: GRAPHICS-MATERIAL-406
title: Phase 0 - Correctness hardening for material type resolution and conformance checks
status: todo
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-405]
updated_at: 2026-02-09
evidence: ""
---

## Goal
Fix structural correctness issues early in the current render path before adding more material features.

## Scope
- Add explicit `materialType` metadata in material asset/runtime contract.
- Remove runtime material type inference from asset-path string heuristics.
- Add program preflight validation for missing `IMaterialInstance` type conformances.

## Acceptance Criteria
- [ ] Material type is loaded from asset metadata, not path parsing.
- [ ] Missing material type conformance is detected before shader link and reported clearly.
- [ ] Existing Standard/Unlit scene path remains functional with no path-based branching.

## Test Plan
- build: `cmake --build build/x64-debug --target April_asset April_graphics April_scene`
- manual: import scene with Standard/Unlit materials and verify type mapping in inspector.

## Expected Files
- `engine/asset/source/asset/material-asset.hpp`
- `engine/scene/source/scene/renderer/render-resource-registry.cpp`
- `engine/graphics/source/graphics/program/program.cpp`
- `engine/editor/source/editor/window/inspector-window.cpp`
