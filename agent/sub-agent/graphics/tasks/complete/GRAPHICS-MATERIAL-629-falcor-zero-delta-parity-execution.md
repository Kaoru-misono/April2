---
id: GRAPHICS-MATERIAL-629
title: Execute zero-delta Falcor material parity
status: done
owner: codex
priority: p0
deps: [GRAPHICS-MATERIAL-628]
updated_at: "2026-02-11"
evidence: "Implemented parity slices: (1) introduced shared Slang codegen schemas under `engine/graphics/shader/shared/material/*.shared.slang` and regenerated `source/graphics/generated/material/*` + `shader/material/generated/*`; (2) removed shared `AlphaMode::Blend` contract (host migration maps BLEND -> MASK), matching Falcor-style 1-bit alpha-mode header packing; (3) aligned material hint contract to Falcor semantics (`DisableNormalMapping`, `AdjustShadingNormal`) and made `FALCOR_MATERIAL_INSTANCE_SIZE` define mandatory in shader contracts; (4) switched scene shader binding contract to `ParameterBlock<MaterialSystem> materials` and moved resource binding authority into `MaterialSystem::bindShaderData()`; (5) extended `MaterialSystem` define emission/metadata behavior toward Falcor semantics (dynamic instance size define, material define conflict checks, dynamic-material update set, cached modules/conformances); (6) added host texture3D descriptor channel registration/accessors and strict material-system optional define requirement in shader material system; (7) added Falcor-style material-system resource mutation APIs (`add*/replace*` for texture/sampler/buffer/texture3D) with `ResourcesChanged` signaling; (8) hardened material-type registry extension ids with lock + monotonic allocation + bit-budget guard; (9) realigned shader `IMaterialInstance` contract to Falcor-style template signatures and updated `BSDFProperties` payload shape plus scene shader usage; (10) applied destructive convergence by removing active Unlit material runtime path, dropping asset `materialType` dispatch metadata, and removing JSON serialization APIs from `IMaterial`; (11) added utility parity hooks (`removeDuplicateMaterials()`, `optimizeMaterials()`) and shader-side param-layout scaffolding files (`material-param-layout.slang`, `serialized-material-params.slang`); (12) introduced host `MaterialTextureManager` abstraction and wired deferred loader resolution into `MaterialSystem::update()`; (13) added phase-function shader contracts, host conformance injection, and shader-side material phase evaluation helper; (14) improved parameter-block lifecycle by maintaining owned reflected binding-block state with size validation/recreation; (15) added host marshaling/upload hooks for material param layout and serialized parameter payload buffers. Verification: `cmake --build build/vs2022 --config Debug --target April_graphics --clean-first` (pass), `cmake --build build/vs2022 --config Debug --target April_graphics` (pass), `cmake --build build/vs2022 --config Debug --target April_scene April_asset` (pass), `cmake --build build/vs2022 --config Debug --target test-suite` (pass), `build/vs2022/bin/Debug/test-suite.exe -tc=\"*material*\"` (pass/no failures emitted), `build/vs2022/bin/Debug/test-suite.exe -tc=\"*conformance*\"` (pass/no failures emitted)."
---

## Goal
Implement the remaining April material-system differences so runtime architecture, contracts, and behavior match Falcor material framework with no intentional deltas.

## Scope
- Resolve correctness-critical ABI/semantic mismatches first.
- Align host/shader material-system responsibilities to Falcor ownership boundaries.
- Add missing Falcor lifecycle/resource/tooling behavior from the latest delta audit.

## Acceptance Criteria
- [x] P0 correctness mismatch resolved: alpha-mode encoding contract is Falcor-consistent.
- [x] Material-system binding path is single-authority and Falcor-aligned.
- [x] Missing Falcor lifecycle/resource APIs in current gap list are implemented or explicitly closed via equivalent behavior.
- [x] Define/conformance/material-instance-size behavior matches Falcor semantics.
- [x] Build/tests covering modified paths pass.

## Test Plan
- `cmake --build build/vs2022 --config Debug --target April_graphics April_scene`
- `cmake --build build/vs2022 --config Debug --target test-suite`
- `build/vs2022/bin/Debug/test-suite.exe -tc="*material*"`

## Expected Files
- `engine/graphics/source/graphics/material/i-material.hpp`
- `engine/graphics/source/graphics/material/material-system.hpp`
- `engine/graphics/source/graphics/material/material-system.cpp`
- `engine/graphics/source/graphics/material/material-type-registry.hpp`
- `engine/graphics/source/graphics/material/material-type-registry.cpp`
- `engine/graphics/shader/material/material-data.slang`
- `engine/graphics/shader/material/material-types.slang`
- `engine/graphics/shader/material/material-system.slang`
- `engine/graphics/shader/material/material-instance-hints.slang`
- `engine/graphics/shader/scene/scene-mesh.slang`
- `engine/scene/source/scene/renderer/scene-renderer.cpp`
- `agent/sub-agent/graphics/interfaces.md`
- `agent/sub-agent/graphics/changelog.md`
