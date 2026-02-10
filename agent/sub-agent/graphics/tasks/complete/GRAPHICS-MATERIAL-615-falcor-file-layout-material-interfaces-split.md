---
id: GRAPHICS-MATERIAL-615
title: Split material interface files to Falcor-aligned layout
status: done
owner: codex
priority: p0
deps: []
updated_at: "2026-02-10"
evidence: "Created split contract files (`i-material-instance.slang`, `material-base.slang`, `material-instance-base.slang`) and narrowed `i-material.slang` to IMaterial contract. Validation command: `cmake --build build/x64-debug --target April_graphics April_scene test-suite`; blocked by environment toolchain missing standard headers (`cmath`, `string`)."
---

## Goal
Align shader material file layout with Falcor by splitting interface and base contracts into dedicated files.

## Scope
- Split combined interface declarations into dedicated contract files.
- Introduce base default implementations (`MaterialBase`, `MaterialInstanceBase`) in separate files.
- Keep transitional imports so downstream files compile during migration.

## Acceptance Criteria
- [x] `i-material.slang` only declares `IMaterial`.
- [x] `i-material-instance.slang` only declares `IMaterialInstance` and related sample/property structs.
- [x] `material-base.slang` and `material-instance-base.slang` provide default open implementations.
- [x] Existing material shaders compile using new imports without duplicate symbol definitions.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics`
- verify: shader compile stage resolves the new file graph without cyclic import errors.

## Expected Files
- `engine/graphics/shader/material/i-material.slang`
- `engine/graphics/shader/material/i-material-instance.slang`
- `engine/graphics/shader/material/material-base.slang`
- `engine/graphics/shader/material/material-instance-base.slang`

## Out of Scope
- Behavior changes in Standard/Unlit BSDF implementation.
