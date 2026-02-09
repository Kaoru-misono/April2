---
id: GRAPHICS-PROGRAM-607
title: Phase 7 - Audit and fix RHI Program handling for material conformances
status: todo
owner: codex
priority: p0
deps: [GRAPHICS-MATERIAL-606]
updated_at: "2026-02-10"
evidence: ""
---

## Goal
Audit and fix issues in RHI/program processing that can break Falcor-style material interface specialization and linking.

## Scope
- Audit program key/versioning inputs for defines, conformances, and entry-point grouping.
- Audit specialization/link error handling and diagnostics surfacing for material interface failures.
- Implement targeted fixes in program manager/version/variables as needed.

## Acceptance Criteria
- [ ] Program compilation cache keys reflect material conformance-affecting inputs.
- [ ] Material interface specialization/link failures provide actionable diagnostics.
- [ ] Program variable binding remains correct after conformance or material type changes.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene`
- run: `build/x64-debug/bin/editor.exe`
- verify: material conformance permutations compile and link reliably without stale cache behavior.

## Expected Files
- `engine/graphics/source/graphics/program/program-manager.cpp`
- `engine/graphics/source/graphics/program/program-version.cpp`
- `engine/graphics/source/graphics/program/program.cpp`
- `engine/graphics/source/graphics/program/program-variables.cpp`
