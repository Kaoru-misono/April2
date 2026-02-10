---
id: GRAPHICS-MATERIAL-613
title: Align host MaterialSystem conformance and module aggregation with Falcor
status: done
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-610, GRAPHICS-PROGRAM-608]
updated_at: "2026-02-10"
evidence: "Host MaterialSystem now deduplicates conformance aggregation by material type and host materials now publish both `<Type, IMaterial>` and `<TypeInstance, IMaterialInstance>` mappings (Standard/Unlit). Verification run: `cmake --build build/x64-debug --target April_graphics April_scene` (blocked by environment missing standard headers: `cmath`/`cstdint`)."
---

## Goal
Make host-side MaterialSystem collect and expose shader modules and type conformances in a Falcor-style, deduplicated, material-type-driven way.

## Scope
- Add/normalize per-material-type dedup for shader modules and type conformances.
- Ensure material code changes refresh aggregated module/conformance state consistently.
- Provide deterministic host API behavior for `getTypeConformances()` and related diagnostics.

## Acceptance Criteria
- [x] MaterialSystem aggregates shader modules and conformances by material type, not per-instance duplication.
- [x] `getTypeConformances()` returns complete set needed by material dynamic dispatch.
- [x] Missing material-type conformance data is surfaced explicitly (diagnostic/exception) rather than silently ignored.
- [x] Program creation path consumes aggregated conformances without requiring shader path heuristics.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene April_test`
- run: `build/x64-debug/bin/test-suite.exe -tc="*material*"`
- run: `build/x64-debug/bin/test-suite.exe -tc="*conformance*"`
- verify: type conformance wiring is stable across mixed material sets and no duplicate/omitted mapping regressions appear.

## Expected Files
- `engine/graphics/source/graphics/material/material-system.cpp`
- `engine/graphics/source/graphics/material/material-system.hpp`
- `engine/graphics/source/graphics/material/i-material.hpp`
- `engine/graphics/source/graphics/program/program-manager.cpp`

## Notes
- Keep selective upload optimization intact unless it conflicts with correctness.
