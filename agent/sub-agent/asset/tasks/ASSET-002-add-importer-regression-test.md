---
id: ASSET-002
title: Add importer regression test coverage
status: todo
owner: codex
priority: p1
deps: []
updated_at: 2026-02-09
evidence: ""
---

## Goal
Add a focused regression test that catches importer behavior drift.

## Acceptance Criteria
- [ ] New doctest case fails before fix and passes after fix.
- [ ] Test asset and expected behavior are clearly documented.
- [ ] Test runs in `test-suite` without flaky behavior.

## Test Plan
- build: `cmake --build build/x64-debug --target test-suite`
- run: `./build/bin/test-suite --test-case="*asset*import*"`

## Expected Files
- `engine/test/test-asset.cpp`
- `engine/asset/source/asset/importer/*`
