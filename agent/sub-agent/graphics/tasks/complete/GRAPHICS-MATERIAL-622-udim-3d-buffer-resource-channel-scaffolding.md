---
id: GRAPHICS-MATERIAL-622
title: Add UDIM and extended resource-channel scaffolding for material system
status: done
owner: codex
priority: p2
deps: [GRAPHICS-MATERIAL-616, GRAPHICS-MATERIAL-620]
updated_at: "2026-02-10"
evidence: "Extended shader-side MaterialSystem context with optional UDIM indirection toggle, buffer resource table, and 3D texture table scaffolding while keeping default behavior unchanged. Validation command: `cmake --build build/x64-debug --target April_graphics April_scene test-suite`; blocked by environment missing standard headers (`cmath`, `string`)."
---

## Goal
Prepare material resource interfaces for Falcor-style UDIM, buffer, and 3D texture channel support.

## Scope
- Add shader-side structures and helper signatures for UDIM indirection.
- Add material-system resource arrays and access helpers for buffers and 3D textures.
- Add host-side descriptor plumbing placeholders and feature defines.

## Acceptance Criteria
- [x] Shader MaterialSystem declares optional UDIM indirection pathway.
- [x] Shader MaterialSystem supports buffer and 3D texture access helpers.
- [x] Host defines include toggles for UDIM/extra resource channels.
- [x] Existing scenes remain functional with new features disabled.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene`
- run: `build/x64-debug/bin/editor.exe`
- verify: no regressions with default feature flags; extended arrays bind cleanly.

## Expected Files
- `engine/graphics/shader/material/material-system.slang`
- `engine/graphics/source/graphics/material/material-system.hpp`
- `engine/graphics/source/graphics/material/material-system.cpp`
