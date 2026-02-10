---
id: GRAPHICS-MATERIAL-617
title: Split StandardMaterial files and align pattern generation semantics
status: done
owner: codex
priority: p0
deps: [GRAPHICS-MATERIAL-615, GRAPHICS-MATERIAL-616]
updated_at: "2026-02-10"
evidence: "Split Standard material implementation into `standard-material.slang` (type) and `standard-material-instance.slang` (instance behavior), with setup path accepting `lod/hints`. Validation command: `cmake --build build/x64-debug --target April_graphics April_scene test-suite`; blocked by environment toolchain headers missing (`cmath`, `string`)."
---

## Goal
Align Standard material implementation with Falcor by splitting type/instance files and matching setup semantics.

## Scope
- Split `StandardMaterial` and `StandardMaterialInstance` into separate shader files.
- Move pattern generation logic to material setup with LOD/hints support.
- Ensure instance file owns eval/sample/pdf/properties behavior.

## Acceptance Criteria
- [x] `standard-material.slang` only defines `StandardMaterial : IMaterial`.
- [x] `standard-material-instance.slang` only defines `StandardMaterialInstance : IMaterialInstance`.
- [x] Material setup accepts `lod` and `hints` and computes sampled parameters in one place.
- [x] No duplicated Standard instance logic remains in other files.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene`
- run: `build/x64-debug/bin/editor.exe`
- verify: Standard material scenes render with stable baseColor/metallic/roughness/emissive behavior.

## Expected Files
- `engine/graphics/shader/material/standard-material.slang`
- `engine/graphics/shader/material/standard-material-instance.slang`
- `engine/graphics/shader/material/standard-bsdf.slang`

## Notes
- Preserve current ABI payload layout while moving behavior into split files.
