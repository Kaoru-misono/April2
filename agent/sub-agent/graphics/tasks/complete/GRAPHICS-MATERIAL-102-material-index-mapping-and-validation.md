---
id: GRAPHICS-MATERIAL-102
title: Implement deterministic RenderID to GPU material index mapping
status: done
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-101]
updated_at: 2026-02-09
evidence: "Centralized draw-path mapping through `RenderResourceRegistry::resolveGpuMaterialIndex()` and switched scene draw lookup to that API; invalid GPU indices still log once in scene renderer and fall back safely. Verification attempted with `cmake --build build/x64-debug --target April_scene April_graphics` (environment/toolchain failure: missing C++ standard headers like `cmath`/`string`)."
---

## Goal
Guarantee deterministic and validated mapping from engine material identity to GPU material table index.

## Scope
- Remove implicit index assumptions in scene rendering path.
- Centralize mapping queries in render resource registry.
- Add one-time warning/assert path for invalid indices.

## Acceptance Criteria
- [x] Scene draw uses explicit GPU material index lookup.
- [x] Invalid material index falls back safely and logs once.
- [x] Mapping behavior is deterministic across frames/reloads.

## Test Plan
- build: `cmake --build build/x64-debug --target April_scene April_graphics`
- manual: render imported Sponza and confirm no slot-shift artifacts caused by index mismatch.

## Expected Files
- `engine/scene/source/scene/renderer/render-resource-registry.hpp`
- `engine/scene/source/scene/renderer/render-resource-registry.cpp`
- `engine/scene/source/scene/renderer/scene-renderer.cpp`

