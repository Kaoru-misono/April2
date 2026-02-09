---
id: GRAPHICS-PROGRAM-607
title: Phase 7 - Audit and fix RHI Program handling for material conformances
status: done
owner: codex
priority: p0
deps: [GRAPHICS-MATERIAL-606]
updated_at: "2026-02-10"
evidence: "Program/RHI audit fixes implemented: (1) `Program::link()` now hard-fails when preflight conformance validation reports missing `IMaterialInstance` mappings instead of continuing into opaque link failures; (2) `Program::addTypeConformance()` and `removeTypeConformance()` now mark dirty only on real conformance-set changes to avoid stale/needless relinks; (3) `ProgramManager::createProgramKernels()` now appends Slang diagnostics for composite-component failures at specialization and linking stages; (4) `ProgramVersion::getKernels()` no longer dereferences null specialization arg types when forming cache keys. Verification: `cmake --build build/vs2022 --config Debug --target April_graphics April_scene` (passed), `./build/vs2022/bin/Debug/test-suite.exe -tc=\"*conformance*\"` (passed), `./build/vs2022/bin/Debug/test-suite.exe -tc=\"*compilation*\"` (passed)."
---

## Goal
Audit and fix issues in RHI/program processing that can break Falcor-style material interface specialization and linking.

## Scope
- Audit program key/versioning inputs for defines, conformances, and entry-point grouping.
- Audit specialization/link error handling and diagnostics surfacing for material interface failures.
- Implement targeted fixes in program manager/version/variables as needed.

## Acceptance Criteria
- [x] Program compilation cache keys reflect material conformance-affecting inputs.
- [x] Material interface specialization/link failures provide actionable diagnostics.
- [x] Program variable binding remains correct after conformance or material type changes.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene`
- run: `build/x64-debug/bin/editor.exe`
- verify: material conformance permutations compile and link reliably without stale cache behavior.

## Expected Files
- `engine/graphics/source/graphics/program/program-manager.cpp`
- `engine/graphics/source/graphics/program/program-version.cpp`
- `engine/graphics/source/graphics/program/program.cpp`
- `engine/graphics/source/graphics/program/program-variables.cpp`
