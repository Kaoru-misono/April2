---
id: GRAPHICS-MATERIAL-302
title: Wire host-side type conformance aggregation for material interfaces
status: todo
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-301]
updated_at: 2026-02-09
evidence: ""
---

## Goal
Ensure programs that use Slang material interfaces receive correct host-side type conformance bindings.

## Scope
- Aggregate type conformances from active materials in MaterialSystem.
- Inject conformances into scene program compilation path.
- Validate default behavior when only Standard material exists.

## Acceptance Criteria
- [ ] Scene shader using `IMaterial` interface compiles with host-provided conformances.
- [ ] Missing conformance produces clear diagnostic.
- [ ] Program cache key reflects conformance set changes.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene`
- manual: switch/enable material types and verify recompilation with updated conformances.

## Expected Files
- `engine/graphics/source/graphics/material/material-system.cpp`
- `engine/graphics/source/graphics/program/program.cpp`
- `engine/scene/source/scene/renderer/scene-renderer.cpp`

