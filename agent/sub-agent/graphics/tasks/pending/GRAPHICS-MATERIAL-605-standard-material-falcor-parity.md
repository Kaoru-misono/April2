---
id: GRAPHICS-MATERIAL-605
title: Phase 5 - StandardMaterial Falcor-parity implementation
status: todo
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-603, GRAPHICS-MATERIAL-604]
updated_at: "2026-02-10"
evidence: ""
---

## Goal
Upgrade StandardMaterial to the rebuilt Falcor-aligned framework with complete parameter and BSDF behavior.

## Scope
- Align host StandardMaterial payload and texture semantics with shared ABI contracts.
- Align shader StandardMaterial and BSDF evaluation path with the new interface stack.
- Ensure migration preserves expected PBR behavior while removing legacy compatibility shims.

## Acceptance Criteria
- [ ] StandardMaterial fully uses new IMaterial/IMaterialInstance/IBSDF and MaterialSystem contracts.
- [ ] PBR texture slots and scalar parameters map consistently host-to-shader.
- [ ] Mixed BasicMaterial and StandardMaterial rendering is stable.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene`
- run: `build/x64-debug/bin/editor.exe`
- verify: standard material assets render correctly with expected shading behavior.

## Expected Files
- `engine/graphics/source/graphics/material/standard-material.{hpp,cpp}`
- `engine/graphics/shader/material/standard-material.slang`
- `engine/graphics/shader/material/standard-bsdf.slang`
- `engine/graphics/shader/material/material-data.slang`
