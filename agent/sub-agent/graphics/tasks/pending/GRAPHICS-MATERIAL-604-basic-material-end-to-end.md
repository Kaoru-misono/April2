---
id: GRAPHICS-MATERIAL-604
title: Phase 4 - Implement BasicMaterial end-to-end
status: todo
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-603]
updated_at: "2026-02-10"
evidence: ""
---

## Goal
Implement Falcor-style BasicMaterial on top of the rebuilt material interfaces and system.

## Scope
- Add host BasicMaterial class with parameter payload, texture bindings, and serialization.
- Add shader BasicMaterial instance and BSDF implementation path.
- Register type ids and factory dispatch so BasicMaterial is selectable through normal material flow.

## Acceptance Criteria
- [ ] BasicMaterial compiles and binds through MaterialSystem without ad-hoc pass logic.
- [ ] BasicMaterial parameters round-trip via serialization.
- [ ] Scene render path can display meshes using BasicMaterial.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene`
- run: `build/x64-debug/bin/editor.exe`
- verify: a scene with BasicMaterial renders and updates correctly.

## Expected Files
- `engine/graphics/source/graphics/material/basic-material.{hpp,cpp}`
- `engine/graphics/shader/material/basic-material.slang`
- `engine/graphics/shader/material/material-factory.slang`
- `engine/graphics/source/graphics/material/material-type-registry.*`
