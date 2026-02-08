---
id: GRAPHICS-MATERIAL-301
title: Implement Slang MaterialSystem and MaterialFactory modules
status: todo
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-201]
updated_at: 2026-02-09
evidence: ""
---

## Goal
Introduce Slang-side material system and factory modules aligned with Falcor architecture.

## Scope
- Add `material/material-system.slang` and `material/material-factory.slang`.
- Implement `getMaterial(materialID)` and `getMaterialInstance(...)`.
- Provide bridge helpers for shading data and texture sampling.

## Acceptance Criteria
- [ ] Slang modules compile and are importable by scene shaders.
- [ ] Dynamic material creation path (`createDynamicObject`) is wired for at least Standard type.
- [ ] Shader code can query material instance through factory API.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics`
- run: shader compilation path succeeds for scene program.

## Expected Files
- `engine/graphics/shader/material/material-system.slang`
- `engine/graphics/shader/material/material-factory.slang`
- `engine/graphics/shader/material/i-material.slang`
- `engine/graphics/shader/material/shading-data.slang`

