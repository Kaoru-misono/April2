---
id: GRAPHICS-MATERIAL-603
title: Phase 3 - MaterialSystem Falcor-aligned core rebuild
status: done
owner: codex
priority: p0
deps: [GRAPHICS-MATERIAL-602]
updated_at: "2026-02-10"
evidence: "Implemented MaterialSystem core slice: added per-material write path (`writeMaterialData()`), switched `updateGpuBuffers()` to selective uploads for dirty materials while keeping full rebuild path for structural dirtiness, and reset overflow diagnostics counters on full rebuild for accurate telemetry snapshots. Verification: `cmake --build build/vs2022 --config Debug --target April_graphics` (passed), `./build/vs2022/bin/Debug/test-suite.exe -tc=\"*conformance*\"` (passed), `./build/vs2022/bin/Debug/test-suite.exe -tc=\"*compilation*\"` (passed)."
---

## Goal
Rebuild MaterialSystem as the central Falcor-style material data and resource manager with scalable descriptor handling.

## Scope
- Implement typed payload upload path and stable material header contract.
- Implement descriptor table capacity flow from host defines into shader compile.
- Implement selective updates, overflow diagnostics, and material system telemetry.

## Acceptance Criteria
- [x] MaterialSystem supports typed payloads and incremental uploads.
- [x] Descriptor capacities are host-configurable and shader-visible without hardcoded constants.
- [x] Diagnostics expose usage/capacity/overflow counters for editor debugging.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene`
- run: `build/x64-debug/bin/editor.exe`
- verify: mixed scene upload/update paths are stable under descriptor pressure.

## Expected Files
- `engine/graphics/source/graphics/material/material-system.hpp`
- `engine/graphics/source/graphics/material/material-system.cpp`
- `engine/graphics/shader/material/material-system.slang`
- `engine/graphics/shader/material/material-data.slang`
