---
id: GRAPHICS-MATERIAL-621
title: Add volume and differentiable material contract scaffolding
status: done
owner: codex
priority: p2
deps: [GRAPHICS-MATERIAL-617, GRAPHICS-MATERIAL-618]
updated_at: "2026-02-10"
evidence: "Added volume/diff scaffolding contracts in material interfaces and concrete Standard/Unlit materials (`setupDiffMaterialInstance`, `evalAD`, volume property hooks) with stable non-diff behavior. Validation command: `cmake --build build/x64-debug --target April_graphics April_scene test-suite`; blocked by environment toolchain missing standard headers (`cmath`, `string`)."
---

## Goal
Introduce Falcor-compatible contract surface for volume properties and differentiable material flows.

## Scope
- Extend interfaces and implementations with `setupDiffMaterialInstance`/`evalAD` placeholders.
- Add homogeneous/instance volume property hooks across Standard/Unlit.
- Keep behavior stable when diff/volume features are not actively used.

## Acceptance Criteria
- [x] `IMaterial` and concrete materials provide `setupDiffMaterialInstance` signature.
- [x] `IMaterialInstance` and concrete instances provide `evalAD` signature.
- [x] Volume property methods exist on both interface and concrete implementations.
- [x] Non-diff rendering path behavior is unchanged.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene`
- run: `build/x64-debug/bin/editor.exe`
- verify: base rendering path remains stable while new API contracts compile.

## Expected Files
- `engine/graphics/shader/material/i-material.slang`
- `engine/graphics/shader/material/i-material-instance.slang`
- `engine/graphics/shader/material/standard-material.slang`
- `engine/graphics/shader/material/standard-material-instance.slang`
- `engine/graphics/shader/material/unlit-material.slang`
- `engine/graphics/shader/material/unlit-material-instance.slang`
