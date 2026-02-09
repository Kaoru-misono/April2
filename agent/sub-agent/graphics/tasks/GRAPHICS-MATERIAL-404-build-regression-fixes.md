---
id: GRAPHICS-MATERIAL-404
title: Fix post-migration build regressions for Unlit and material path handling
status: done
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-402, GRAPHICS-MATERIAL-403]
updated_at: 2026-02-09
evidence: "Implemented in commit `f943c4f`: fixed registry path-lowercasing bug (`assetPath` is already `std::string`, removed erroneous `.string()` call), added regression-prevention notes to graphics knowledge, and regenerated `engine/graphics/source/graphics/generated/material/material-types.generated.hpp` locally using `python engine/graphics/scripts/slang-codegen.py --input engine/graphics/shader/material/material-types.slang --output engine/graphics/source/graphics/generated/material/material-types.generated.hpp` to sync `MaterialType::Unlit`. Verification command `cmake --build build/x64-debug --` now no longer reports prior `Unlit`/`std::transform` compile errors; build still fails due environment/toolchain standard header resolution (`cmath`/`string`/`functional`)."
---

## Goal
Resolve compilation errors introduced by the recent material extensibility changes and capture prevention notes.

## Scope
- Fix `Unlit` enum/type usage build break in host code.
- Fix material asset path lowercase conversion logic in render resource registry.
- Record root causes and prevention guidance in graphics knowledge.

## Acceptance Criteria
- [ ] `April_graphics` and `April_scene` compile in local environment.
- [x] Unlit material host code no longer references missing generated enum entries.
- [x] Cause + prevention are documented in `knowledge.md`.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene`

## Expected Files
- `engine/scene/source/scene/renderer/render-resource-registry.cpp`
- `engine/graphics/source/graphics/material/unlit-material.cpp`
- `agent/sub-agent/graphics/knowledge.md`
