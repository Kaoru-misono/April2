---
id: RT-002
title: Add headless runtime smoke test
status: todo
owner: codex
priority: p1
deps: []
updated_at: 2026-02-09
evidence: ""
---

## Goal
Add a lightweight headless runtime smoke test to validate startup and one-frame execution.

## Acceptance Criteria
- [ ] Headless runtime path initializes and runs one frame without assertions.
- [ ] Smoke test can run in CI-friendly mode.
- [ ] Failure output is actionable.

## Test Plan
- build: `cmake --build build/x64-debug --target test-suite`
- run: `./build/bin/test-suite --test-case="*runtime*smoke*"`

## Expected Files
- `engine/test/test-runtime.cpp`
- `engine/runtime/source/**`
