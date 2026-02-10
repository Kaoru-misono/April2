---
id: GRAPHICS-MATERIAL-611
title: Align StandardMaterial and ShadingData semantics with Falcor
status: done
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-610]
updated_at: "2026-02-10"
evidence: "Updated `ShadingData` with material context/IoR, switched Standard BSDF setup to front/back-face eta logic, and made metallic/roughness normalization consistent in `standard-bsdf.slang`. Verification run: `cmake --build build/x64-debug --target April_graphics April_scene` (blocked by environment missing standard headers: `cmath`/`cstdint`)."
---

## Goal
Match Falcor semantics for shading data preparation and StandardMaterial BSDF parameter derivation.

## Scope
- Extend shader-side `ShadingData` with Falcor-equivalent material and IoR context fields required for correct eta handling.
- Update Standard material setup flow so `eta`, front/back-face behavior, and material parameter decoding follow Falcor patterns.
- Make metallic/roughness handling internally consistent for all derived BSDF terms.

## Acceptance Criteria
- [x] `ShadingData` contains required context (`materialID`, material header data, outside medium IoR or equivalent) used in material setup.
- [x] Standard material setup computes `eta` with front/back-face aware logic equivalent to Falcor behavior.
- [x] Metallic/roughness values are used consistently across diffuse/specular/roughness fields (no split clamp path).
- [x] Render output on representative standard materials remains visually stable after parity update.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene`
- run: `build/x64-debug/bin/editor.exe`
- verify: standard metal/rough/transmission materials produce stable output with no obvious energy spikes from out-of-range inputs.

## Expected Files
- `engine/graphics/shader/material/shading-data.slang`
- `engine/graphics/shader/material/standard-material.slang`
- `engine/graphics/shader/material/standard-bsdf.slang`
- `engine/graphics/shader/material/material-data.slang`

## Notes
- Preserve ABI compatibility where possible; if ABI changes are required, follow generated header update flow in the same task.
