---
id: SCENE-IMPORT-001
title: Fix Sponza material slot misalignment after import
status: in_progress
owner: codex
priority: p1
deps: []
updated_at: 2026-02-08
evidence: ""
---

## Goal
Fix runtime material mismatch for imported Sponza so each submesh uses the correct imported material slot.

## Acceptance Criteria
- [ ] Imported Sponza renders with correct per-submesh material assignment.
- [ ] Slot index mapping is stable from glTF primitive material index to runtime material id lookup.
- [ ] Regression validation exists (test/log/assert) for slot index alignment.

## Test Plan
- build: `cmake --build build/x64-debug --target test-suite April_editor`
- run: `./build/bin/test-suite --test-case="*sponza*"`
- manual: import and view `content/model/Sponza/glTF/Sponza.gltf`, verify material regions are not shifted.

## Notes
- Expected touch points are likely `engine/scene` and possibly `engine/asset`.
