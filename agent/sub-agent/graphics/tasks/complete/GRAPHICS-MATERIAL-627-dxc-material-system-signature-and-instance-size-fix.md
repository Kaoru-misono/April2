---
id: GRAPHICS-MATERIAL-627
title: Fix DXC material-system signature issue and material instance size defines
status: done
owner: codex
priority: p0
deps: [GRAPHICS-MATERIAL-625, GRAPHICS-MATERIAL-626]
updated_at: "2026-02-10"
evidence: "Resolved runtime pipeline failures by (1) adding MaterialSystem shader modules + defines to SceneRenderer program creation, (2) setting `FALCOR_MATERIAL_INSTANCE_SIZE=256` in host defines, (3) reducing IMaterial payload to materialID (128-byte interface budget), and (4) avoiding DXC incomplete-type expansion by gating extra resource arrays behind `MATERIAL_SYSTEM_ENABLE_EXTRA_RESOURCES=0` and using struct member helper methods. Verified with Slang/DXIL compile commands for `scene-mesh.slang` and material modules."
---

## Goal
Stabilize material program linking and DXC codegen for the scene mesh pipeline using Falcor-aligned module/define plumbing.

## Scope
- Ensure SceneRenderer injects material shader modules and shader defines into Program creation.
- Align IMaterial/IMaterialInstance runtime size constraints with actual implementation payloads.
- Eliminate DXC incomplete-type failure caused by extra descriptor-array resources in MaterialSystem context.

## Acceptance Criteria
- [x] SceneRenderer adds `materialSystem->getShaderModules()` and `materialSystem->getShaderDefines()` to program creation.
- [x] `FALCOR_MATERIAL_INSTANCE_SIZE` is explicitly defined as `256` in runtime program defines.
- [x] `IMaterial` implementations use compact payload (`uint materialID`) and satisfy `[anyValueSize(128)]`.
- [x] DXC no longer emits `ByteAddressBuffer [16] incomplete type` for scene mesh pipeline.

## Test Plan
- `slangc -no-codegen -I engine/graphics/shader engine/graphics/shader/material/material-system.slang`
- `slangc -no-codegen -I engine/graphics/shader engine/graphics/shader/material/standard-material.slang`
- `slangc -no-codegen -I engine/graphics/shader engine/graphics/shader/scene/scene-mesh.slang`
- `slangc -target dxil -entry psMain -profile ps_6_5 -I engine/graphics/shader -DFALCOR_MATERIAL_INSTANCE_SIZE=256 -DMATERIAL_TEXTURE_TABLE_SIZE=128 -DMATERIAL_SAMPLER_TABLE_SIZE=8 -DMATERIAL_BUFFER_TABLE_SIZE=16 -DMATERIAL_TEXTURE3D_TABLE_SIZE=16 -conformance "StandardMaterial:IMaterial=0" engine/graphics/shader/scene/scene-mesh.slang engine/graphics/shader/material/standard-material.slang`

## Expected Files
- `engine/scene/source/scene/renderer/scene-renderer.cpp`
- `engine/graphics/source/graphics/material/material-system.cpp`
- `engine/graphics/shader/material/i-material.slang`
- `engine/graphics/shader/material/i-material-instance.slang`
- `engine/graphics/shader/material/material-system.slang`
- `engine/graphics/shader/material/material-factory.slang`
- `engine/graphics/shader/material/standard-material.slang`
- `engine/graphics/shader/material/unlit-material.slang`
- `engine/graphics/shader/scene/scene-mesh.slang`
