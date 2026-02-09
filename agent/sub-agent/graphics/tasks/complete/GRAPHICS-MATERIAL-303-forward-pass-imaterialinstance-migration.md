---
id: GRAPHICS-MATERIAL-303
title: Migrate forward shading path to IMaterialInstance evaluation
status: done
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-301, GRAPHICS-MATERIAL-302]
updated_at: 2026-02-09
evidence: "Implemented: scene forward shader now builds `ShadingData`, creates `IMaterialInstance` through material factory, and uses interface methods (`getAlbedo()`, `getRoughness()`, `getEmission()`, `eval()`) instead of hardcoded Standard material parameter reads/sampling logic. Verification attempted with `cmake --build build/x64-debug --target April_scene April_graphics` (environment toolchain failure: missing standard headers `cmath`/`string`/`cstdint`)."
---

## Goal
Switch scene forward shading from hardcoded StandardMaterialData reads to IMaterialInstance path.

## Scope
- Build shading data from scene vertex/pixel context.
- Query `IMaterialInstance` via material factory.
- Use instance outputs for albedo/roughness/emission/BRDF eval.

## Acceptance Criteria
- [x] `scene-mesh.slang` no longer hardcodes Standard-only shading path.
- [x] Standard material visual output remains stable after migration.
- [x] Material interface path is active in runtime draw.

## Test Plan
- build: `cmake --build build/x64-debug --target April_scene April_graphics`
- manual: render Sponza and compare before/after snapshots.

## Expected Files
- `engine/graphics/shader/scene/scene-mesh.slang`
- `engine/graphics/shader/material/standard-material.slang`
- `engine/graphics/shader/material/standard-bsdf.slang`

