---
id: GRAPHICS-MATERIAL-604
title: Phase 4 - Implement BasicMaterial end-to-end
status: done
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-603]
updated_at: "2026-02-10"
evidence: "Falcor alignment pass completed: validated against `falcor-material-reference/Scene/Material/BasicMaterial.{h,cpp}` and `BasicMaterialData.slang`, then reworked April material host side so `BasicMaterial` is a shared base abstraction (not a new `MaterialType`). Added `engine/graphics/source/graphics/material/basic-material.{hpp,cpp}` with shared texture binding/flags/descriptor/common payload serialization, switched `StandardMaterial` to inherit `BasicMaterial`, and updated `MaterialSystem` descriptor-handle update path to target `BasicMaterial` abstraction. Verification: `cmake --build build/vs2022 --config Debug --target April_graphics April_scene` (passed), `cmake --build build/vs2022 --config Debug --target test-suite` (passed), `./build/vs2022/bin/Debug/test-suite.exe -tc=\"*conformance*\"` (passed), `./build/vs2022/bin/Debug/test-suite.exe -tc=\"*compilation*\"` (passed)."
---

## Goal
Implement Falcor-style BasicMaterial on top of the rebuilt material interfaces and system.

## Scope
- Add host BasicMaterial class with parameter payload, texture bindings, and serialization.
- Add shader BasicMaterial instance and BSDF implementation path.
- Register type ids and factory dispatch so BasicMaterial is selectable through normal material flow.

## Acceptance Criteria
- [x] BasicMaterial compiles and binds through MaterialSystem without ad-hoc pass logic.
- [x] BasicMaterial parameters round-trip via serialization.
- [x] Scene render path can display meshes using BasicMaterial.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene`
- run: `build/x64-debug/bin/editor.exe`
- verify: Standard/Unlit path renders with BasicMaterial host base wiring and no Basic enum runtime type.

## Expected Files
- `engine/graphics/source/graphics/material/basic-material.{hpp,cpp}`
- `engine/graphics/source/graphics/material/basic-material.{hpp,cpp}`
- `engine/graphics/shader/material/material-factory.slang`
- `engine/graphics/source/graphics/material/material-type-registry.*`
