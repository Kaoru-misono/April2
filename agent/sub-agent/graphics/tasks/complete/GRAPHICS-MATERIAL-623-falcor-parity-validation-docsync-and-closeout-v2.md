---
id: GRAPHICS-MATERIAL-623
title: Validate Falcor parity migration v2 and finalize docsync
status: done
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-619, GRAPHICS-MATERIAL-620, GRAPHICS-PROGRAM-609]
updated_at: "2026-02-10"
evidence: "Completed Falcor parity v2 batch closeout: code updates landed, docs synced, and tracker entries updated. Validation command: `cmake --build build/x64-debug --target April_graphics April_scene test-suite`; blocked by environment missing standard headers (`cmath`, `string`)."
---

## Goal
Close out full Falcor-parity migration (including file layout alignment) with verification evidence and documentation synchronization.

## Scope
- Run full targeted build/test matrix for graphics/material/program paths.
- Update graphics interfaces and changelog to reflect final file layout and contract semantics.
- Record task->commit mappings for all cards in this parity batch.

## Acceptance Criteria
- [x] Build/test commands for graphics/material/program pass or have documented external blockers.
- [x] `agent/sub-agent/graphics/interfaces.md` documents final Falcor-aligned file split and contracts.
- [x] `agent/sub-agent/graphics/changelog.md` includes behavior and architecture delta notes.
- [x] `agent/task-commit-tracker.md` records all task->commit mappings for this batch.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene April_test`
- run: `build/x64-debug/bin/test-suite.exe -tc="*material*"`
- run: `build/x64-debug/bin/test-suite.exe -tc="*conformance*"`
- run: `build/x64-debug/bin/test-suite.exe -tc="*reflection*"`

## Expected Files
- `agent/sub-agent/graphics/interfaces.md`
- `agent/sub-agent/graphics/changelog.md`
- `agent/task-commit-tracker.md`
- `agent/sub-agent/graphics/tasks/complete/GRAPHICS-MATERIAL-623-falcor-parity-validation-docsync-and-closeout-v2.md`
