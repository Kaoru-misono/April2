---
id: GRAPHICS-MATERIAL-608
title: Phase 8 - Validation, doc sync, and migration closeout
status: todo
owner: codex
priority: p1
deps: [GRAPHICS-PROGRAM-607]
updated_at: "2026-02-10"
evidence: ""
---

## Goal
Close the migration with full validation, documentation synchronization, and task-to-commit tracking.

## Scope
- Run agreed build/test/manual validation matrix for graphics/scene/editor paths.
- Update public interface and changelog docs for all impacted modules.
- Update task commit tracker with final task id to commit id mapping for this batch.

## Acceptance Criteria
- [ ] Validation commands pass or have documented known blockers with evidence.
- [ ] Public API/behavior changes are reflected in interfaces/changelog/integration docs.
- [ ] Task commit tracker includes all task ids completed in this migration batch.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene April_runtime April_editor test-suite`
- test: `build/x64-debug/bin/test-suite`
- run: `build/x64-debug/bin/editor.exe`

## Expected Files
- `agent/sub-agent/graphics/interfaces.md`
- `agent/sub-agent/graphics/changelog.md`
- `agent/sub-agent/scene/interfaces.md`
- `agent/sub-agent/scene/changelog.md`
- `agent/task-commit-tracker.md`
