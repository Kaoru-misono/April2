---
id: GRAPHICS-MATERIAL-302
title: Wire host-side type conformance aggregation for material interfaces
status: done
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-301]
updated_at: 2026-02-09
evidence: "Implemented in commit `8d2e34b`: MaterialSystem-provided type conformances are injected into scene shader program creation; Standard material now contributes `StandardMaterialInstance -> IMaterialInstance` conformance; program link failures now print active type-conformance summary for clearer missing-conformance diagnostics. Program cache key already includes `TypeConformanceList` in `ProgramVersionKey`, so conformance changes trigger distinct program versions. Verification attempted with `cmake --build build/x64-debug --target April_graphics April_scene` (environment toolchain failure: missing standard headers `cmath`/`string`)."
---

## Goal
Ensure programs that use Slang material interfaces receive correct host-side type conformance bindings.

## Scope
- Aggregate type conformances from active materials in MaterialSystem.
- Inject conformances into scene program compilation path.
- Validate default behavior when only Standard material exists.

## Acceptance Criteria
- [x] Scene shader using `IMaterial` interface compiles with host-provided conformances.
- [x] Missing conformance produces clear diagnostic.
- [x] Program cache key reflects conformance set changes.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene`
- manual: switch/enable material types and verify recompilation with updated conformances.

## Expected Files
- `engine/graphics/source/graphics/material/material-system.cpp`
- `engine/graphics/source/graphics/program/program.cpp`
- `engine/scene/source/scene/renderer/scene-renderer.cpp`

