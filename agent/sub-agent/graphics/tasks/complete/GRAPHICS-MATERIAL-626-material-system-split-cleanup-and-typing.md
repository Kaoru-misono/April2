---
id: GRAPHICS-MATERIAL-626
title: Clean up material-system split and restore strict typing
status: done
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-625]
updated_at: "2026-02-10"
evidence: "Removed dispatch functions from `material-system.slang` to avoid cyclic dependencies and keep resource helpers focused. Verified module typecheck with Slang."
---

## Goal
Finalize the split between MaterialSystem resource helpers and factory dispatch logic so Slang typing remains stable.

## Scope
- Keep `material-system.slang` focused on resource tables and texture sampling helpers.
- Remove IMaterial/IMaterialInstance dispatch functions from `material-system.slang` and centralize in factory extension module.
- Preserve optional scaffold channels (buffer/3D texture/UDIM define) while maintaining typecheck health.

## Acceptance Criteria
- [x] `material-system.slang` compiles without importing `i-material.slang`.
- [x] No duplicate/ambiguous context type declarations remain.
- [x] Material resource helper functions remain available to material implementations.

## Test Plan
- `build/x64-debug/_deps/slang-src/bin/slangc.exe -no-codegen -I engine/graphics/shader engine/graphics/shader/material/material-system.slang`
- `build/x64-debug/_deps/slang-src/bin/slangc.exe -no-codegen -I engine/graphics/shader engine/graphics/shader/material/standard-material.slang`

## Expected Files
- `engine/graphics/shader/material/material-system.slang`
- `engine/graphics/shader/material/standard-material.slang`
- `engine/graphics/shader/material/unlit-material.slang`
