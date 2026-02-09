# Falcor vs April Material Architecture Gap Analysis

Date: 2026-02-09
Task: GRAPHICS-MATERIAL-405

## 1) Executive Summary

Falcor's material architecture is a full-stack system (host metadata, shader factory, dynamic interface dispatch, update/change tracking, parameter layout/serialization, and resource management with explicit limits and validation) built for many material types and long-lived renderer features.

April has made strong progress (versioned material ABI, descriptor handles, shader-side IMaterialInstance path, type registry, second material type), but it is still a transition architecture. The current path is functional for Standard + Unlit forward shading, yet it lacks robust type-safe payload partitioning, lifecycle/state tracking, scalable descriptor management, and tooling/serialization surfaces needed for large material ecosystems.

Main conclusion: April should move from "single-blob + ad hoc extension" to a typed multi-material platform with explicit material metadata, capability contracts, and staged migration gates.

## 2) Source Baseline

### Falcor references
- `external-repo/Scene/Material/Material.h`
- `external-repo/Scene/Material/MaterialData.slang`
- `external-repo/Scene/Material/MaterialSystem.h`
- `external-repo/Scene/Material/MaterialSystem.cpp`
- `external-repo/Scene/Material/MaterialSystem.slang`
- `external-repo/Scene/Material/MaterialFactory.slang`
- `external-repo/Scene/Material/MaterialTypeRegistry.h`
- `external-repo/Scene/Material/MaterialTypeRegistry.cpp`
- `external-repo/Scene/Material/StandardMaterial.cpp`
- `external-repo/Scene/Material/StandardMaterialParamLayout.slang`
- `external-repo/Scene/Material/SerializedMaterialParams.h`

### April references
- `engine/graphics/source/graphics/material/i-material.hpp`
- `engine/graphics/source/graphics/material/material-system.hpp`
- `engine/graphics/source/graphics/material/material-system.cpp`
- `engine/graphics/source/graphics/material/material-type-registry.hpp`
- `engine/graphics/source/graphics/material/material-type-registry.cpp`
- `engine/graphics/source/graphics/material/standard-material.hpp`
- `engine/graphics/source/graphics/material/unlit-material.hpp`
- `engine/graphics/source/graphics/material/unlit-material.cpp`
- `engine/graphics/shader/material/material-data.slang`
- `engine/graphics/shader/material/material-system.slang`
- `engine/graphics/shader/material/material-factory.slang`
- `engine/graphics/shader/material/i-material.slang`
- `engine/graphics/shader/scene/scene-mesh.slang`
- `engine/scene/source/scene/renderer/scene-renderer.cpp`
- `engine/scene/source/scene/renderer/render-resource-registry.cpp`
- `engine/editor/source/editor/window/inspector-window.cpp`

## 3) Architecture Comparison

### Falcor (observed)
- Material base is feature-rich: update flags, texture slots, alpha policy, dynamic material support, data blob export, shader modules, type conformances, defines, and parameter layout hooks.
- Material header is tightly packed with explicit bit budgets (`kMaterialTypeBits`, sampler bits, alpha bits, IoR bits), giving long-term ABI governance.
- Material system tracks metadata and updates reactively (`mMaterialsChanged`, per-material update flags, dynamic material list), and only rebuilds what is needed.
- Shader-side material system exposes central resource arrays, utility sampling APIs, alpha test helpers, displacement helpers, UDIM indirection, and material factory extension.
- Type registry supports built-in + dynamic registration, thread-safe lookup, and limits based on ABI bit budget.
- Parameter layout + serialization path exists for differentiable/training or automation workflows.

### April (observed)
- Host material interface is minimal and clear (`writeData`, `getTypeConformances`, flags, texture binding).
- Material ABI is versioned and includes descriptor handle fields in `StandardMaterialData`.
- Material system now owns descriptor registration and type-conformance aggregation.
- Shader path has `IMaterialInstance`, material factory, and Standard/Unlit instance creation.
- Scene forward pass now routes through material factory and one-pass descriptor table binding.
- Type registry exists and maps names/ids, but currently without ABI bit-limit contract.
- Editor has debug surface for material type and GPU index.

## 4) Capability Mapping (Falcor -> April)

1. Host material abstraction: Present in both, Falcor significantly richer.
2. Header ABI governance: Present in both, Falcor more compact/contractual.
3. Multi-material factory via dynamic object: Present in both.
4. Descriptor/resource management: Present in both, Falcor has broader coverage (3D textures, reserved counts, texture manager integration).
5. Update lifecycle granularity: Strong in Falcor, weak in April.
6. Parameter layout/serialization: Present in Falcor, missing in April.
7. Feature utility APIs (alpha/displacement/UDIM): Present in Falcor, mostly missing in April.
8. Material metadata and stats: Present in Falcor, minimal in April.
9. Dynamic material support: explicit in Falcor, absent in April.
10. Material tooling and optimization pipeline: extensive in Falcor, early in April.

## 5) April Architecture Gaps and Improvement Points

### Gap 1: Single payload struct for all types
- Current: `IMaterial::writeData(generated::StandardMaterialData&)` and Unlit also writes `StandardMaterialData`.
- Risk: Wasted bandwidth, weak type safety, harder extension.
- Improve: Introduce `MaterialDataBlob` style union/payload model or per-type payload structs with common header.
- Complexity: Medium-High.
- Verify: add compile-time size/offset checks + shader decode tests per type.

### Gap 2: No material update flags/lifecycle model
- Current: global dirty bit in `MaterialSystem` only.
- Risk: over-updates, no reactive invalidation by code/data/resources.
- Improve: add `UpdateFlags` at IMaterial level + per-material update tracking in MaterialSystem.
- Complexity: Medium.
- Verify: tests for selective upload and unchanged-material skip behavior.

### Gap 3: Descriptor tables are fixed constants in shader
- Current: `kMaterialTextureTableSize = 128`, `kMaterialSamplerTableSize = 8` hardcoded in shader and scene binding.
- Risk: scalability ceiling, silent clamping behavior.
- Improve: host-driven defines and reflection-backed limits, plus overflow diagnostics.
- Complexity: Medium.
- Verify: stress test with >128 textures and explicit failure path.

### Gap 4: Type registry has no ABI bit-budget contract
- Current: `MaterialTypeRegistry` uses hash/linear probing without explicit max bits from header.
- Risk: future incompatibility if type space grows/changes.
- Improve: define `kMaterialTypeBits` in header contract and enforce registry bounds.
- Complexity: Low-Medium.
- Verify: unit tests for max type id and overflow error behavior.

### Gap 5: Material selection in scene uses asset-path heuristic for Unlit
- Current: `render-resource-registry.cpp` checks if path contains `"unlit"`.
- Risk: brittle content workflow and false matches.
- Improve: store explicit material type in `MaterialAsset` metadata and load by declared type.
- Complexity: Medium.
- Verify: import/export tests with type field; no path-based branching.

### Gap 6: Missing material shader module discovery pipeline
- Current: scene shader imports are manually edited; not driven by host material set.
- Risk: divergence between host types and shader factory/imports.
- Improve: add host API to aggregate required shader modules per active material type.
- Complexity: Medium.
- Verify: runtime program build test with mixed material sets.

### Gap 7: No parameter layout/serialization surface
- Current: no equivalent of Falcor material param layout and serialized params.
- Risk: weak tooling, no robust editor preset exchange, hard to support ML/differentiable workflows later.
- Improve: define typed parameter layout metadata and serialization contract per material type.
- Complexity: Medium-High.
- Verify: round-trip serialization tests (asset <-> runtime material).

### Gap 8: No material optimization pass (texture analysis/constants)
- Current: direct usage of bound textures; no optimization.
- Risk: avoidable texture fetch overhead and memory use.
- Improve: optional optimization stage (constant texture collapse, alpha simplification).
- Complexity: Medium.
- Verify: golden-scene perf comparison + visual diff tests.

### Gap 9: Utility shading helpers are minimal
- Current: no shared alpha/displacement/UDIM-like material helpers at system level.
- Risk: duplicated logic across passes and inconsistent behavior.
- Improve: move common material operations into `material-system.slang` extension helpers.
- Complexity: Medium.
- Verify: shared helper usage in forward + future RT pass.

### Gap 10: Material resource categories are incomplete
- Current: textures/samplers/buffers in host API, shader context currently exposes textures/samplers only.
- Risk: extension friction for buffer-driven materials.
- Improve: complete buffer/3D texture path end-to-end in shader and host bind stage.
- Complexity: Medium.
- Verify: add one buffer-backed material type sample.

### Gap 11: Limited runtime diagnostics/telemetry
- Current: some warnings and inspector fields, no system-level stats snapshot.
- Risk: hard to debug content scale issues.
- Improve: add material stats API (counts by type, descriptor usage, memory estimates).
- Complexity: Low-Medium.
- Verify: editor debug panel + automated smoke counters.

### Gap 12: Missing conformance completeness validation
- Current: link error logs were improved, but preflight validation is minimal.
- Risk: late failure at program link.
- Improve: add pre-link checks comparing active material types against registered conformances.
- Complexity: Low.
- Verify: unit/integration test that injects missing conformance and checks explicit diagnostic.

## 6) Prioritized Roadmap

### Phase A (stabilize current architecture, 1-2 sprints)
1. Replace path heuristic with explicit material type in asset schema.
2. Add conformance preflight validation.
3. Promote descriptor limits to host-driven defines (remove hardcoded shader constants).
4. Add material stats/debug API.

### Phase B (scale and correctness, 2-4 sprints)
1. Introduce per-material update flags and selective GPU upload.
2. Add ABI type-id bit-budget contract and registry bounds checks.
3. Complete buffer/3D texture support in shader material system context.
4. Add module aggregation API for material shader dependencies.

### Phase C (long-term extensibility, 4+ sprints)
1. Refactor payload model away from `StandardMaterialData`-for-all-types.
2. Add parameter layout + serialization contract per material type.
3. Add material optimization/analysis pass.
4. Introduce shared material utility helpers for forward + RT consistency.

## 7) Migration Constraints and Risk Controls

- Keep current Standard path as compatibility baseline while introducing typed payloads.
- Gate each phase with golden scene render diff + performance smoke checks.
- Maintain one-way compatibility adapters where asset/runtime schema changes are introduced.
- Add explicit task-level test gates before enabling new material classes by default.

## 8) Suggested Follow-up Tasks

1. GRAPHICS-MATERIAL-406: MaterialAsset explicit type field and loader migration.
2. GRAPHICS-MATERIAL-407: MaterialSystem descriptor limits via host defines/reflection.
3. GRAPHICS-MATERIAL-408: Per-material update flags and selective upload.
4. GRAPHICS-MATERIAL-409: Material payload/type-safe blob redesign.
5. GRAPHICS-MATERIAL-410: Material stats/diagnostics API and editor panel.
