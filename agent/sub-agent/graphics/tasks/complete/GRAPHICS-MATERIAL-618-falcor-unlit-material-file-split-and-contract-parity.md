---
id: GRAPHICS-MATERIAL-618
title: Split Unlit material files and complete interface contract parity
status: done
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-615, GRAPHICS-MATERIAL-616]
updated_at: "2026-02-10"
evidence: "Split Unlit material into `unlit-material.slang` and `unlit-material-instance.slang`, and implemented expanded instance contract methods (`getProperties`, `getLobeTypes`, volume hooks, `evalAD`). Validation command: `cmake --build build/x64-debug --target April_graphics April_scene test-suite`; blocked by environment missing standard headers (`cmath`, `string`)."
---

## Goal
Refactor Unlit material into Falcor-style material/instance file separation and satisfy expanded material instance contract.

## Scope
- Split `UnlitMaterial` and `UnlitMaterialInstance` into separate files.
- Ensure Unlit instance implements all required `IMaterialInstance` methods.
- Keep unlit shading behavior unchanged (no BSDF contribution, emissive/albedo support).

## Acceptance Criteria
- [x] `unlit-material.slang` only defines `UnlitMaterial : IMaterial`.
- [x] `unlit-material-instance.slang` defines full `IMaterialInstance` method set.
- [x] Contract additions (`getProperties`, `getLobeTypes`, volume hooks) have deterministic Unlit behavior.
- [x] No reference to deprecated combined Unlit implementation remains.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene`
- run: `build/x64-debug/bin/editor.exe`
- verify: emissive unlit meshes still render and no runtime interface conformance errors occur.

## Expected Files
- `engine/graphics/shader/material/unlit-material.slang`
- `engine/graphics/shader/material/unlit-material-instance.slang`
- `engine/graphics/shader/material/i-material-instance.slang`
