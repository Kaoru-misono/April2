---
id: ASSET-EDITOR-001
title: Validate Sponza material import and fix Content Browser asset listing
status: done
owner: codex
priority: p1
deps: []
updated_at: 2026-02-08
evidence: "local changes in engine/test/test-asset.cpp + engine/editor/source/editor/window/content-browser-window.cpp; user verified test-suite run with Sponza case passing on 2026-02-08"
---

## Goal
1) Add a regression test that imports `content/model/sponza` and verifies imported mesh material slots and referenced material textures are wired correctly.
2) Make Content Browser only show `.asset` files and hide file extensions in the visible label.

## Acceptance Criteria
- [x] Test fails on slot/texture mismatch and passes on correct mapping.
- [x] Content Browser file list excludes non-`.asset` files.
- [x] Content Browser item label does not show `.asset` extension.

## Test Plan
- build: `cmake --build build/x64-debug --target test-suite`
- run: `./build/bin/test-suite --test-case="*sponza*"`
- manual: open editor Content Browser and confirm only `.asset` items are listed, with extension hidden.

## Notes
- Scope includes `engine/asset` and `engine/editor`.
