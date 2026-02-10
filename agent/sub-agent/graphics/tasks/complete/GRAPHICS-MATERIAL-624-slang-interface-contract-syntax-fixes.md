---
id: GRAPHICS-MATERIAL-624
title: Fix Slang interface contract syntax and import regressions
status: done
owner: codex
priority: p0
deps: [GRAPHICS-MATERIAL-615, GRAPHICS-MATERIAL-616]
updated_at: "2026-02-10"
evidence: "Fixed `MaterialSystemContext` ambiguity and missing type imports after file split. Validation commands: `slangc -no-codegen` on `material/i-material.slang`, `material/material-system.slang`, `material/material-factory.slang`, `material/standard-material-instance.slang`, `material/unlit-material-instance.slang`, `scene/scene-mesh.slang` all pass."
---

## Goal
Resolve Slang compile errors introduced by material file-split refactor while keeping Falcor-compatible interface direction.

## Scope
- Remove ambiguous context declarations and cyclic type ambiguity points.
- Ensure all split files import required symbols (`IMaterialInstance`, `IBSDF`, `BSDFContext`, sample generator types).
- Add missing Slang interface attributes that Falcor relies on (`anyValueSize` for key interfaces).

## Acceptance Criteria
- [x] No ambiguous type reference errors for material-system context types.
- [x] `standard-material-instance.slang` and `unlit-material-instance.slang` resolve all BSDF symbols.
- [x] `scene-mesh.slang` resolves `IMaterialInstance` after contract split.
- [x] `i-material.slang`, `i-material-instance.slang`, and `texture-sampler.slang` use valid Slang interface attributes.

## Test Plan
- `build/x64-debug/_deps/slang-src/bin/slangc.exe -no-codegen -I engine/graphics/shader engine/graphics/shader/material/i-material.slang`
- `build/x64-debug/_deps/slang-src/bin/slangc.exe -no-codegen -I engine/graphics/shader engine/graphics/shader/material/material-system.slang`
- `build/x64-debug/_deps/slang-src/bin/slangc.exe -no-codegen -I engine/graphics/shader engine/graphics/shader/material/material-factory.slang`
- `build/x64-debug/_deps/slang-src/bin/slangc.exe -no-codegen -I engine/graphics/shader engine/graphics/shader/material/standard-material-instance.slang`
- `build/x64-debug/_deps/slang-src/bin/slangc.exe -no-codegen -I engine/graphics/shader engine/graphics/shader/material/unlit-material-instance.slang`
- `build/x64-debug/_deps/slang-src/bin/slangc.exe -no-codegen -I engine/graphics/shader engine/graphics/shader/scene/scene-mesh.slang`

## Expected Files
- `engine/graphics/shader/material/i-material.slang`
- `engine/graphics/shader/material/i-material-instance.slang`
- `engine/graphics/shader/material/texture-sampler.slang`
- `engine/graphics/shader/material/standard-material-instance.slang`
- `engine/graphics/shader/material/unlit-material-instance.slang`
- `engine/graphics/shader/scene/scene-mesh.slang`
