---
id: GRAPHICS-MATERIAL-202
title: Bind MaterialSystem once per scene pass and remove per-draw texture globals
status: todo
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-201]
updated_at: 2026-02-09
evidence: ""
---

## Goal
Refactor scene forward pass to consume a unified MaterialSystem binding model.

## Scope
- Replace per-draw `baseColorTexture/normalTexture/...` globals in scene shader.
- Bind material system parameter block once per pass.
- Keep draw-time work focused on indices/instance data only.

## Acceptance Criteria
- [ ] Scene pass no longer binds per-material texture globals every draw.
- [ ] Material resources are fetched through MaterialSystem binding.
- [ ] Visual parity for Standard material path is maintained.

## Test Plan
- build: `cmake --build build/x64-debug --target April_scene April_graphics`
- manual: render Sponza and compare with pre-change result.

## Expected Files
- `engine/graphics/shader/scene/scene-mesh.slang`
- `engine/scene/source/scene/renderer/scene-renderer.cpp`
- `engine/scene/source/scene/renderer/render-resource-registry.cpp`

