---
id: GRAPHICS-MATERIAL-103
title: Add regression checks for submesh material slot integrity
status: todo
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-102]
updated_at: 2026-02-09
evidence: ""
---

## Goal
Add regression coverage that catches slot/index drift from import to GPU draw path.

## Scope
- Extend tests for `submesh.materialIndex -> mesh slot -> GPU material index`.
- Add runtime debug dump path for per-submesh material binding (optional debug flag).

## Acceptance Criteria
- [ ] Test fails when slot/index chain is broken.
- [ ] Test passes on Sponza import and draw preparation path.
- [ ] Debug dump can be enabled to audit per-submesh mapping.

## Test Plan
- build: `cmake --build build/x64-debug --target test-suite`
- run: `./build/bin/test-suite --test-case="*sponza*"`

## Expected Files
- `engine/test/test-asset.cpp`
- `engine/test/source/test-reflection.cpp`
- `engine/scene/source/scene/renderer/scene-renderer.cpp`

