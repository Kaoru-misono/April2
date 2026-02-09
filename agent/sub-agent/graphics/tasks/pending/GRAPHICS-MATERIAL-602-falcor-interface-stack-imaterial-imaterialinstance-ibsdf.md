---
id: GRAPHICS-MATERIAL-602
title: Phase 2 - Rebuild material interface stack IMaterial IMaterialInstance IBSDF
status: todo
owner: codex
priority: p0
deps: [GRAPHICS-MATERIAL-601]
updated_at: "2026-02-10"
evidence: ""
---

## Goal
Implement a Falcor-aligned material interface hierarchy across host and shader with clear ownership between IMaterial, IMaterialInstance, and IBSDF.

## Scope
- Define and align host `IMaterial` responsibilities for lifecycle, parameter serialization, and conformance exposure.
- Define shader-side `IMaterialInstance` and `IBSDF` contracts for shading evaluation paths.
- Wire material factory and type conformance registration to avoid hardcoded per-pass branching.

## Acceptance Criteria
- [ ] Host and shader interfaces match the intended architecture boundaries.
- [ ] Type conformance wiring covers all active material types.
- [ ] Missing conformance cases fail with actionable diagnostics before runtime draw.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene`
- run: `build/x64-debug/bin/editor.exe`
- verify: interface specialization and link stages succeed for scene material shaders.

## Expected Files
- `engine/graphics/source/graphics/material/i-material.hpp`
- `engine/graphics/shader/material/i-material.slang`
- `engine/graphics/shader/material/material-factory.slang`
- `engine/graphics/source/graphics/program/program*.{hpp,cpp}`
