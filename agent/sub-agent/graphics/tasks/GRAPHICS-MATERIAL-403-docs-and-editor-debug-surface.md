---
id: GRAPHICS-MATERIAL-403
title: Document extension workflow and expose material debug surfaces
status: todo
owner: codex
priority: p2
deps: [GRAPHICS-MATERIAL-401]
updated_at: 2026-02-09
evidence: ""
---

## Goal
Complete migration by documenting extension contract and exposing runtime debug info.

## Scope
- Update interfaces/changelog docs for graphics material architecture.
- Add debug inspector fields for material type id and GPU material index.
- Document steps to add a new material type.

## Acceptance Criteria
- [ ] `interfaces.md` and `changelog.md` reflect architecture changes.
- [ ] Editor/runtime debug view can inspect material type/index mapping.
- [ ] New material type onboarding steps are documented.

## Test Plan
- docs + manual validation in editor debug UI.

## Expected Files
- `agent/sub-agent/graphics/interfaces.md`
- `agent/sub-agent/graphics/changelog.md`
- `engine/editor/source/editor/window/*`

