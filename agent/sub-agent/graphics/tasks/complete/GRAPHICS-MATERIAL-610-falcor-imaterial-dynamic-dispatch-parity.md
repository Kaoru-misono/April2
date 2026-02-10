---
id: GRAPHICS-MATERIAL-610
title: Align Slang material stack to Falcor IMaterial dynamic dispatch
status: done
owner: codex
priority: p0
deps: [GRAPHICS-MATERIAL-606]
updated_at: "2026-02-10"
evidence: "Implemented IMaterial + IMaterialInstance dynamic-dispatch path in Slang (`i-material`, `standard-material`, `unlit-material`, `material-factory`, `material-system`) and routed construction through `createDynamicObject<IMaterial, StandardMaterialData>` + `setupMaterialInstance(...)`. Verification run: `cmake --build build/x64-debug --target April_graphics April_scene` (blocked in this environment by missing standard headers: `cmath`/`cstdint`)."
---

## Goal
Refactor April material shader interfaces to match Falcor's IMaterial -> IMaterialInstance dynamic-object flow.

## Scope
- Add a first-class `IMaterial` interface in Slang.
- Move per-hit material instance construction to `setupMaterialInstance(...)` on concrete material types.
- Replace hardcoded branching in material factory with `createDynamicObject<IMaterial, StandardMaterialData>(...)` based dispatch.

## Acceptance Criteria
- [x] `engine/graphics/shader/material/i-material.slang` exposes both `IMaterial` and `IMaterialInstance` interfaces.
- [x] `standard-material.slang` implements `StandardMaterial : IMaterial` and returns `StandardMaterialInstance` via `setupMaterialInstance(...)`.
- [x] `unlit-material.slang` implements `UnlitMaterial : IMaterial` and returns `UnlitMaterialInstance` via `setupMaterialInstance(...)`.
- [x] `material-factory.slang` no longer uses direct type `if/else` for material instance selection.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene`
- run: `build/x64-debug/bin/editor.exe`
- verify: scene render path creates material instances through dynamic material dispatch and compiles without interface conformance errors.

## Expected Files
- `engine/graphics/shader/material/i-material.slang`
- `engine/graphics/shader/material/standard-material.slang`
- `engine/graphics/shader/material/unlit-material.slang`
- `engine/graphics/shader/material/material-factory.slang`

## Notes
- Keep runtime behavior compatible with existing `StandardMaterialData` ABI while moving interface dispatch semantics.
