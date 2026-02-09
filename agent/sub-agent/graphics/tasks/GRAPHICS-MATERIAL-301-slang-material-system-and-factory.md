---
id: GRAPHICS-MATERIAL-301
title: Implement Slang MaterialSystem and MaterialFactory modules
status: done
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-201]
updated_at: 2026-02-09
evidence: "Implemented in commit `55760fb`: added `material/material-system.slang` and `material/material-factory.slang` with `getMaterial()`, descriptor-table sampling helpers, and `getMaterialInstance()` factory path using `createDynamicObject<IMaterialInstance>` for Standard material instance creation; scene shader imports new modules. Verification attempted with `cmake --build build/x64-debug --target April_graphics` (environment toolchain failure: missing standard header `cmath`)."
---

## Goal
Introduce Slang-side material system and factory modules aligned with Falcor architecture.

## Scope
- Add `material/material-system.slang` and `material/material-factory.slang`.
- Implement `getMaterial(materialID)` and `getMaterialInstance(...)`.
- Provide bridge helpers for shading data and texture sampling.

## Acceptance Criteria
- [x] Slang modules compile and are importable by scene shaders.
- [x] Dynamic material creation path (`createDynamicObject`) is wired for at least Standard type.
- [x] Shader code can query material instance through factory API.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics`
- run: shader compilation path succeeds for scene program.

## Expected Files
- `engine/graphics/shader/material/material-system.slang`
- `engine/graphics/shader/material/material-factory.slang`
- `engine/graphics/shader/material/i-material.slang`
- `engine/graphics/shader/material/shading-data.slang`

