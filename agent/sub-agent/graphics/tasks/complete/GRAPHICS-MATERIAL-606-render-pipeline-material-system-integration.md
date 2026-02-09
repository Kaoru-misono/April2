---
id: GRAPHICS-MATERIAL-606
title: Phase 6 - Integrate rebuilt material system into render pipeline
status: done
owner: codex
priority: p0
deps: [GRAPHICS-MATERIAL-605]
updated_at: "2026-02-10"
evidence: "Validated render-pipeline integration against current scene path: `scene-renderer.cpp` consumes `MaterialSystem` shader defines/type conformances, updates GPU buffers each frame, and binds material buffer + descriptor tables before draw. Added explicit `material.i_material` import in `engine/graphics/shader/scene/scene-mesh.slang` so fragment path references `IMaterialInstance` contract directly. Verification: `cmake --build build/vs2022 --config Debug --target April_graphics April_scene` (passed), `./build/vs2022/bin/Debug/test-suite.exe -tc=\"*conformance*\"` (passed), `./build/vs2022/bin/Debug/test-suite.exe -tc=\"*compilation*\"` (passed)."
---

## Goal
Integrate the rebuilt material framework into the scene render pipeline end-to-end.

## Scope
- Update scene renderer and render resource registry material data flow.
- Update shader parameter binding and draw path to use rebuilt material tables and interfaces.
- Remove obsolete bridging logic superseded by the new material system.

## Acceptance Criteria
- [x] Scene rendering uses rebuilt MaterialSystem as the authoritative material path.
- [x] No legacy material binding path is required for normal rendering.
- [x] Editor runtime path renders with mixed material types without regressions.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene April_runtime April_editor`
- run: `build/x64-debug/bin/editor.exe`
- verify: frame rendering path compiles/links and draws materialized meshes correctly.

## Expected Files
- `engine/scene/source/scene/renderer/scene-renderer.cpp`
- `engine/scene/source/scene/renderer/render-resource-registry.*`
- `engine/graphics/shader/scene/scene-mesh.slang`
- `engine/graphics/source/graphics/material/material-system.*`
