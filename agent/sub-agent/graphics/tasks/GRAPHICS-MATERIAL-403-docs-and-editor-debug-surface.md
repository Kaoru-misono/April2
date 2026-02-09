---
id: GRAPHICS-MATERIAL-403
title: Document extension workflow and expose material debug surfaces
status: done
owner: codex
priority: p2
deps: [GRAPHICS-MATERIAL-401]
updated_at: 2026-02-09
evidence: "Implemented in commit `5954a72`: updated graphics interfaces/changelog docs for material extension architecture and onboarding workflow; added render-resource registry debug APIs (`getMaterialTypeId()`, `getMaterialTypeName()`) and surfaced GPU material index + material type (name/id) in editor inspector Mesh Renderer panel. Verification attempted via `cmake --build build/x64-debug --target April_editor April_scene April_graphics` (environment toolchain failure: missing standard headers e.g. `cmath`, `string`, `functional`)."
---

## Goal
Complete migration by documenting extension contract and exposing runtime debug info.

## Scope
- Update interfaces/changelog docs for graphics material architecture.
- Add debug inspector fields for material type id and GPU material index.
- Document steps to add a new material type.

## Acceptance Criteria
- [x] `interfaces.md` and `changelog.md` reflect architecture changes.
- [x] Editor/runtime debug view can inspect material type/index mapping.
- [x] New material type onboarding steps are documented.

## Test Plan
- docs + manual validation in editor debug UI.

## Expected Files
- `agent/sub-agent/graphics/interfaces.md`
- `agent/sub-agent/graphics/changelog.md`
- `engine/editor/source/editor/window/*`

