---
id: GRAPHICS-MATERIAL-203
title: Add descriptor handle bounds checks and fallback policy
status: done
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-201]
updated_at: 2026-02-09
evidence: "Implemented in commit `311c811`: host-side descriptor handle bounds validation added in MaterialSystem rebuild path for each descriptor slot (`baseColor`, `metallicRoughness`, `normal`, `occlusion`, `emissive`, `sampler`, `buffer`) with fallback to handle `0` and warning logs that include material index and slot name. Shader-side descriptor indexing is clamped in scene pass (`materialTextures[]`/`materialSamplers[]`) from prior task. Verification attempted with `cmake --build build/x64-debug --target April_scene April_graphics` (environment toolchain failure: missing standard headers `cmath`/`string`)."
---

## Goal
Ensure material descriptor accesses are always safe and debuggable.

## Scope
- Validate descriptor ids on host and shader side where possible.
- Add fallback behavior for invalid handles.
- Add concise diagnostics for out-of-range descriptors.

## Acceptance Criteria
- [x] Invalid descriptor handles do not crash render path.
- [x] Fallback resources are consistently used on invalid handles.
- [x] Diagnostics identify material id and failing descriptor slot.

## Test Plan
- build: `cmake --build build/x64-debug --target April_scene April_graphics`
- manual: inject invalid handles in debug and confirm fallback + warning.

## Expected Files
- `engine/graphics/shader/material/material-system.slang`
- `engine/scene/source/scene/renderer/scene-renderer.cpp`
- `engine/graphics/source/graphics/material/material-system.cpp`

