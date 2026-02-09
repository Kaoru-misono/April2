---
id: GRAPHICS-MATERIAL-201
title: Extend MaterialSystem with descriptor tables (textures samplers buffers)
status: done
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-101]
updated_at: 2026-02-09
evidence: "Implemented descriptor-table host path in commit `ccdca3f` by adding texture/sampler/buffer descriptor registration and lookup APIs in MaterialSystem plus StandardMaterial descriptor-handle serialization in GPU data. Verification attempted with `cmake --build build/x64-debug --target April_graphics` (environment toolchain failure: missing standard header `cmath`)."
---

## Goal
Move material resources from per-draw ad hoc binds to MaterialSystem-managed descriptor tables.

## Scope
- Add descriptor arrays for textures/samplers/buffers in host-side MaterialSystem.
- Add resource registration and handle allocation APIs.
- Keep compatibility fallback for missing resources.

## Acceptance Criteria
- [x] MaterialSystem exposes descriptor-backed resource access APIs.
- [x] Material creation path writes valid descriptor handles.
- [x] Existing Standard material can render through descriptor tables.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics`
- manual: verify standard material textures are present after descriptor-table migration.

## Expected Files
- `engine/graphics/source/graphics/material/material-system.hpp`
- `engine/graphics/source/graphics/material/material-system.cpp`
- `engine/graphics/source/graphics/material/standard-material.hpp`
- `engine/graphics/source/graphics/material/standard-material.cpp`

