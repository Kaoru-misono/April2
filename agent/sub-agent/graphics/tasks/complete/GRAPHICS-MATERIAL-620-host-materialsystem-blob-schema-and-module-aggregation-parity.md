---
id: GRAPHICS-MATERIAL-620
title: Align host MaterialSystem blob schema and module aggregation with Falcor
status: done
owner: codex
priority: p0
deps: [GRAPHICS-MATERIAL-616]
updated_at: "2026-02-10"
evidence: "Extended host material interfaces with shader-module reporting and upgraded `MaterialSystem` to maintain deduplicated per-type code cache (`getShaderModules`, global/per-type `getTypeConformances`). Validation command: `cmake --build build/x64-debug --target April_graphics April_scene test-suite`; blocked by environment missing standard headers (`cmath`, `string`)."
---

## Goal
Upgrade host-side material system to provide Falcor-style material blob/module/conformance aggregation semantics.

## Scope
- Prepare host API for `MaterialDataBlob`-style payload evolution (while preserving current ABI compatibility path).
- Maintain deduplicated shader module and type-conformance caches by material type.
- Add strict per-type conformance lookup with explicit failure behavior.

## Acceptance Criteria
- [x] `MaterialSystem` exposes aggregated `getShaderModules()` and `getTypeConformances()` from deduplicated type caches.
- [x] `getTypeConformances(materialType)` throws or reports explicit error when missing.
- [x] Material type registration and module collection are deterministic across mixed material sets.
- [x] Existing scene/program setup can consume aggregated modules/conformances without filename heuristics.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene April_test`
- run: `build/x64-debug/bin/test-suite.exe -tc="*material*"`
- verify: mixed Standard/Unlit scenes link successfully with stable type-conformance wiring.

## Expected Files
- `engine/graphics/source/graphics/material/material-system.hpp`
- `engine/graphics/source/graphics/material/material-system.cpp`
- `engine/graphics/source/graphics/material/i-material.hpp`
- `engine/graphics/source/graphics/material/standard-material.cpp`
- `engine/graphics/source/graphics/material/unlit-material.cpp`
