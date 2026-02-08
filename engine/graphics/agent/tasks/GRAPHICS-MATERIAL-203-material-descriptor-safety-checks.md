---
id: GRAPHICS-MATERIAL-203
title: Add descriptor handle bounds checks and fallback policy
status: todo
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-201]
updated_at: 2026-02-09
evidence: ""
---

## Goal
Ensure material descriptor accesses are always safe and debuggable.

## Scope
- Validate descriptor ids on host and shader side where possible.
- Add fallback behavior for invalid handles.
- Add concise diagnostics for out-of-range descriptors.

## Acceptance Criteria
- [ ] Invalid descriptor handles do not crash render path.
- [ ] Fallback resources are consistently used on invalid handles.
- [ ] Diagnostics identify material id and failing descriptor slot.

## Test Plan
- build: `cmake --build build/x64-debug --target April_scene April_graphics`
- manual: inject invalid handles in debug and confirm fallback + warning.

## Expected Files
- `engine/graphics/shader/material/material-system.slang`
- `engine/scene/source/scene/renderer/scene-renderer.cpp`
- `engine/graphics/source/graphics/material/material-system.cpp`

