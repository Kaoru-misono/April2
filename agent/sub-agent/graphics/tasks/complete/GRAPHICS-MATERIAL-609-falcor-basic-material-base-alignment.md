---
id: GRAPHICS-MATERIAL-609
title: Correct BasicMaterial architecture to Falcor-style base class (no new MaterialType)
status: done
owner: codex
priority: p0
deps: [GRAPHICS-MATERIAL-604]
updated_at: "2026-02-10"
evidence: "Verified against Falcor `BasicMaterial` architecture (`BasicMaterial.h/.cpp`, `BasicMaterialData.slang`) and aligned April host design: no `MaterialType::Basic`, introduced host-side `BasicMaterial` abstraction, made `StandardMaterial` inherit it, and removed Basic runtime type dispatch assumptions. Verification: `cmake --build build/vs2022 --config Debug --target April_graphics April_scene` (passed), `./build/vs2022/bin/Debug/test-suite.exe -tc=\"*conformance*\"` (passed), `./build/vs2022/bin/Debug/test-suite.exe -tc=\"*compilation*\"` (passed)."
---

## Goal
Align April material architecture with Falcor by implementing BasicMaterial as a host-side base abstraction rather than introducing a new `MaterialType` enum value.

## Scope
- Remove any temporary `MaterialType::Basic` assumptions from shader/host contracts.
- Add a Falcor-style host `BasicMaterial` base class between `IMaterial` and `StandardMaterial`.
- Keep `Standard` and `Unlit` as concrete material type ids while reusing common behavior through the base class.

## Acceptance Criteria
- [x] `MaterialType` enum does not include `Basic` as a concrete runtime type id.
- [x] `StandardMaterial` inherits shared behavior from `BasicMaterial` base class.
- [x] Material factory/type conformance flow remains valid with no Basic enum dispatch branch.

## Test Plan
- build: `cmake --build build/vs2022 --config Debug --target April_graphics April_scene`
- test: `./build/vs2022/bin/Debug/test-suite.exe -tc="*conformance*"`
- test: `./build/vs2022/bin/Debug/test-suite.exe -tc="*compilation*"`

## Expected Files
- `engine/graphics/source/graphics/material/basic-material.{hpp,cpp}`
- `engine/graphics/source/graphics/material/standard-material.{hpp,cpp}`
- `engine/graphics/shader/material/material-types.slang`
- `engine/graphics/shader/material/material-factory.slang`
