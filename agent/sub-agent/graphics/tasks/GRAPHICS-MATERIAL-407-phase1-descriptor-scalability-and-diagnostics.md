---
id: GRAPHICS-MATERIAL-407
title: Phase 1 - Descriptor scalability and material diagnostics
status: todo
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-406]
updated_at: 2026-02-09
evidence: ""
---

## Goal
Remove descriptor-table scalability bottlenecks and improve observability for material/resource pressure.

## Scope
- Replace fixed shader descriptor-table constants with host-provided define/config values.
- Add descriptor overflow policy (debug fail-fast + runtime fallback + warning telemetry).
- Add material system diagnostics surface: counts by type, descriptor usage/capacity, invalid-handle counters.

## Acceptance Criteria
- [ ] Descriptor table capacities are configured by host defines and synchronized across shaders/host bind path.
- [ ] Descriptor overflow behavior is deterministic and visible in diagnostics.
- [ ] Inspector/debug surface shows material and descriptor usage stats.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene April_editor`
- manual: stress scene with high texture/material count and verify diagnostics/overflow handling.

## Expected Files
- `engine/graphics/shader/material/material-system.slang`
- `engine/graphics/shader/scene/scene-mesh.slang`
- `engine/graphics/source/graphics/material/material-system.cpp`
- `engine/scene/source/scene/renderer/scene-renderer.cpp`
- `engine/editor/source/editor/window/inspector-window.cpp`
