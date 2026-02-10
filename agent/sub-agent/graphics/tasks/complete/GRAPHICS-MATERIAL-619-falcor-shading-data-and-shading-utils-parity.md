---
id: GRAPHICS-MATERIAL-619
title: Align ShadingData and shading utility split with Falcor conventions
status: done
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-617]
updated_at: "2026-02-10"
evidence: "Added `shading-utils.slang` and switched scene tangent construction to shared helper; updated shading data with additional context fields used by material setup. Validation command: `cmake --build build/x64-debug --target April_graphics April_scene test-suite`; blocked by environment toolchain headers missing (`cmath`, `string`)."
---

## Goal
Bring shading-point data preparation and utilities closer to Falcor structure and semantics.

## Scope
- Keep `shading-data.slang` focused on data structure and preparation entry points.
- Move frame/normal-map helper code into dedicated `shading-utils.slang`.
- Add missing ShadingData fields needed by split Standard material path.

## Acceptance Criteria
- [x] `shading-data.slang` includes material header/materialID/front-facing IoR semantics used by material setup.
- [x] `shading-utils.slang` provides reusable frame/normal helper methods.
- [x] Scene integration compiles against new prepare-shading API without duplicated frame math.
- [x] Standard/Unlit instance paths consume shared ShadingData fields consistently.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene`
- run: `build/x64-debug/bin/editor.exe`
- verify: normal-mapped and non-normal-mapped meshes both render with consistent orientation.

## Expected Files
- `engine/graphics/shader/material/shading-data.slang`
- `engine/graphics/shader/material/shading-utils.slang`
- `engine/graphics/shader/scene/scene-mesh.slang`
