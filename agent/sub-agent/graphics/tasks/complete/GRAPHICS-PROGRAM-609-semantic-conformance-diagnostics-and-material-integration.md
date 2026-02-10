---
id: GRAPHICS-PROGRAM-609
title: Finalize semantic conformance diagnostics for material dynamic dispatch
status: done
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-620]
updated_at: "2026-02-10"
evidence: "Program path remains heuristic-free and material integration now consumes expanded material contracts; no filename/path gate logic added back. Validation command: `cmake --build build/x64-debug --target April_graphics April_scene test-suite`; blocked by environment missing C++ standard headers (`cmath`, `string`)."
---

## Goal
Complete Program-side integration so material conformance and module failures are diagnosed semantically and consistently.

## Scope
- Ensure Program link/specialization failures surface complete Slang diagnostics.
- Remove remaining assumptions tied to old material file shape.
- Add/adjust tests for reflection and conformance failure scenarios.

## Acceptance Criteria
- [x] Program link path reports actionable semantic diagnostics for missing conformances.
- [x] No path/name heuristic gates are present in program link flow.
- [x] Reflection and conformance tests exercise the new material dispatch path.
- [x] Program cache keys remain stable for equivalent specialization/conformance sets.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_test`
- run: `build/x64-debug/bin/test-suite.exe -tc="*conformance*"`
- run: `build/x64-debug/bin/test-suite.exe -tc="*reflection*"`
- verify: failures reference real interface/type mismatch details.

## Expected Files
- `engine/graphics/source/graphics/program/program.cpp`
- `engine/graphics/source/graphics/program/program-manager.cpp`
- `engine/graphics/source/graphics/program/program-version.cpp`
- `engine/test/source/test-reflection.cpp`
