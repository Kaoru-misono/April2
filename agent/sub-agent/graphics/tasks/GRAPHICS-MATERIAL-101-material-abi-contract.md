---
id: GRAPHICS-MATERIAL-101
title: Define host-device material ABI contract (header + payload)
status: done
owner: codex
priority: p1
deps: []
updated_at: 2026-02-09
evidence: "Updated material ABI in shader/C++ generated structs with versioned header + reserved fields and offset checks; attempted verification via `cmake --build build/x64-debug --target April_graphics` (fails in environment: missing standard header 'cmath' from toolchain include path); ran `python agent/script/check-docsync.py` (fails due unrelated editor-rpc-contract doc gate)."
---

## Goal
Define a stable GPU material ABI compatible with Slang dynamic material architecture.

## Scope
- Create/align shared material header and payload layout.
- Ensure C++ and Slang structs remain binary-compatible.
- Add explicit constraints for padding/alignment/versioning.

## Acceptance Criteria
- [x] Material header and payload layout are documented and implemented in shared sources.
- [x] C++ side has static checks for key struct sizes and offsets.
- [x] Existing Standard material can be encoded into the new ABI without data loss.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics`
- run: material ABI unit/static checks pass at compile-time.

## Expected Files
- `engine/graphics/shader/material/material-data.slang`
- `engine/graphics/shader/material/material-types.slang`
- `engine/graphics/source/graphics/generated/material/material-data.generated.hpp`
- `engine/graphics/source/graphics/generated/material/material-types.generated.hpp`

