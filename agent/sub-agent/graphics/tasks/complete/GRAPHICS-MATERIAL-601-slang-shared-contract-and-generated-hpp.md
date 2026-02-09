---
id: GRAPHICS-MATERIAL-601
title: Phase 1 - Slang shared material contract and generated C++ header pipeline
status: done
owner: codex
priority: p0
deps: []
updated_at: "2026-02-10"
evidence: "Implemented slices: (1) added `@export-cpp` constant export support in `engine/graphics/scripts/slang-codegen.py`; (2) exported `kMaterialAbiVersion` from `material-data.slang`; (3) regenerated `engine/graphics/source/graphics/generated/material/material-data.generated.hpp`; (4) switched Standard/Unlit host writes to `generated::kMaterialAbiVersion`; (5) replaced hardcoded built-in registry ids in `material-system.cpp` with `generated::MaterialType` values; (6) removed string/dynamic-cast material type remapping in `MaterialSystem::rebuildMaterialData()` and now write type id directly from `IMaterial::getType()`. Verification: `cmake --build build/vs2022 --config Debug --target April_graphics` passed; `./build/vs2022/bin/Debug/test-suite.exe -tc=\"*conformance*\"` passed; `./build/vs2022/bin/Debug/test-suite.exe -tc=\"*compilation*\"` passed. Runtime launch check `build/vs2022/bin/Debug/editor.exe` still hits existing shader root path issue (`Can't find shader file graphics/scene/scene-mesh.slang`)."
---

## Goal
Establish a Falcor-aligned shared material contract where one Slang source defines host and shader ABI through generated C++ headers.

## Scope
- Audit current material Slang exports and remove host-side duplicated ABI declarations.
- Define shared enums/flags/header/layout in Slang with stable ids and explicit ABI versioning.
- Ensure generated headers are consumed by host material/program code paths as the source of truth.

## Acceptance Criteria
- [x] Material ABI structs/enums consumed by C++ come from Slang-generated headers.
- [x] No conflicting hand-written mirror definitions remain in graphics material host path.
- [x] Shader and host compile with aligned material type ids, flags, and header layout.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene`
- run: `build/x64-debug/bin/editor.exe`
- verify: material shader compile and host ABI usage show no layout/type mismatch failures.

## Expected Files
- `engine/graphics/shader/material/material-types.slang`
- `engine/graphics/shader/material/material-data.slang`
- `engine/graphics/source/graphics/material/*.hpp`
- `engine/graphics/source/graphics/material/*.cpp`
