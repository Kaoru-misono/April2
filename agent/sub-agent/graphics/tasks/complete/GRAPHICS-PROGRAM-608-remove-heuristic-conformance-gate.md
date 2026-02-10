---
id: GRAPHICS-PROGRAM-608
title: Remove path-based conformance preflight hard gate and rely on semantic diagnostics
status: done
owner: codex
priority: p0
deps: [GRAPHICS-PROGRAM-607, GRAPHICS-MATERIAL-610]
updated_at: "2026-02-10"
evidence: "Removed path-based heuristic conformance gate in `Program::link()`/`validateConformancesPreflight()` and kept semantic link diagnostics as source of truth. Verification run: `cmake --build build/x64-debug --target April_graphics April_scene` (blocked by environment missing standard headers: `cmath`/`cstdint`)."
---

## Goal
Eliminate false positives from path-name heuristics in program linking and align conformance failure handling with Falcor-style semantic validation.

## Scope
- Remove hard-fail behavior tied to shader path substring checks.
- Keep actionable diagnostics by surfacing Slang/type-conformance failures from actual compilation/link stages.
- Ensure program dirty/relink behavior remains correct when conformances change.

## Acceptance Criteria
- [x] `Program::link()` does not fail due to filename/path heuristic conformance checks.
- [x] Missing material conformances fail with semantic compile/link diagnostics, not preflight path checks.
- [x] Existing conformance mutation APIs (`addTypeConformance` / `removeTypeConformance`) still trigger needed relinks only when effective changes occur.
- [x] Reflection-focused tests are not blocked by conformance preflight false positives.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_test`
- run: `build/x64-debug/bin/test-suite.exe -tc="*conformance*"`
- run: `build/x64-debug/bin/test-suite.exe -tc="*reflection*"`
- verify: missing conformance errors remain actionable; reflection tests no longer fail due to path heuristic gating.

## Expected Files
- `engine/graphics/source/graphics/program/program.cpp`
- `engine/graphics/source/graphics/program/program-manager.cpp`
- `engine/graphics/source/graphics/program/program-version.cpp`
- `engine/test/source/test-reflection.cpp`

## Out of Scope
- Introducing new material types beyond current standard/unlit parity effort.
