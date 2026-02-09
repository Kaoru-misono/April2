---
id: GRAPHICS-MATERIAL-605
title: Phase 5 - StandardMaterial Falcor-parity implementation
status: done
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-603, GRAPHICS-MATERIAL-604]
updated_at: "2026-02-10"
evidence: "Falcor parity sweep completed for StandardMaterial. Cross-checked against `falcor-material-reference/Rendering/Materials/StandardMaterial.slang` and aligned April shader BSDF preparation to derive dielectric F0 from IoR (instead of hardcoded 0.04), clamp roughness/metallic inputs, and preserve IMaterial/IMaterialInstance/IBSDF + MaterialSystem flow introduced in earlier phases. Host side remains on shared BasicMaterial base abstraction with Standard payload mapping unchanged. Verification: `cmake --build build/vs2022 --config Debug --target April_graphics April_scene` (passed), `./build/vs2022/bin/Debug/test-suite.exe -tc=\"*conformance*\"` (passed), `./build/vs2022/bin/Debug/test-suite.exe -tc=\"*compilation*\"` (passed)."
---

## Goal
Upgrade StandardMaterial to the rebuilt Falcor-aligned framework with complete parameter and BSDF behavior.

## Scope
- Align host StandardMaterial payload and texture semantics with shared ABI contracts.
- Align shader StandardMaterial and BSDF evaluation path with the new interface stack.
- Ensure migration preserves expected PBR behavior while removing legacy compatibility shims.

## Acceptance Criteria
- [x] StandardMaterial fully uses new IMaterial/IMaterialInstance/IBSDF and MaterialSystem contracts.
- [x] PBR texture slots and scalar parameters map consistently host-to-shader.
- [x] StandardMaterial host-base + runtime path stays stable with Falcor-style BasicMaterial abstraction.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene`
- run: `build/x64-debug/bin/editor.exe`
- verify: standard material assets render correctly with expected shading behavior.

## Expected Files
- `engine/graphics/source/graphics/material/standard-material.{hpp,cpp}`
- `engine/graphics/shader/material/standard-material.slang`
- `engine/graphics/shader/material/standard-bsdf.slang`
- `engine/graphics/shader/material/material-data.slang`
