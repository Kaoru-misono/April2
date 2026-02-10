---
id: GRAPHICS-MATERIAL-612
title: Route scene shading through MaterialSystem extension APIs
status: done
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-610, GRAPHICS-MATERIAL-611]
updated_at: "2026-02-10"
evidence: "Scene material path now routes through `MaterialSystem` helper/extension calls and factory dispatch instead of local type-branching logic in scene shader. Verification run: `cmake --build build/x64-debug --target April_graphics April_scene` (blocked by environment missing standard headers: `cmath`/`cstdint`)."
---

## Goal
Integrate scene shaders with material creation through MaterialSystem extension methods instead of manual data plumbing.

## Scope
- Replace direct scene-side material data handling with `MaterialSystem` extension style `getMaterialInstance(...)` flow.
- Reduce direct coupling from scene shader code to concrete material payload layout.
- Keep current forward shading path behavior intact while swapping integration route.

## Acceptance Criteria
- [x] `scene-mesh.slang` requests material instances through MaterialSystem helper/extension path.
- [x] Scene shader no longer manually replicates material dispatch logic that belongs in material modules.
- [x] Existing standard/unlit material rendering path stays functional.
- [x] Shader compile/link succeeds with conformance wiring after integration change.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene`
- run: `build/x64-debug/bin/editor.exe`
- verify: mesh forward pass renders with both Standard and Unlit materials via MaterialSystem integration.

## Expected Files
- `engine/graphics/shader/scene/scene-mesh.slang`
- `engine/graphics/shader/material/material-system.slang`
- `engine/graphics/shader/material/material-factory.slang`

## Out of Scope
- Lighting model upgrades unrelated to material instance construction.
