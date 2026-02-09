# Falcor-Informed, April-Native Material Architecture Review

Date: 2026-02-09  
Task: GRAPHICS-MATERIAL-405

## Intent

This document is **not** a Falcor migration plan.  
It is a design review that uses Falcor as a reference model to:

1. extract architecture principles that hold up in production,
2. identify weaknesses in April's current material path,
3. propose a **modern-engine** path that fits April's codebase and module boundaries.

## Source Anchors

### Falcor references
- `external-repo/Scene/Material/MaterialSystem.slang`
- `external-repo/Scene/Material/MaterialFactory.slang`
- `external-repo/Scene/Material/MaterialSystem.cpp`
- `external-repo/Scene/Material/MaterialTypeRegistry.cpp`
- `external-repo/Scene/Material/MaterialData.slang`
- `external-repo/Scene/Material/StandardMaterial.cpp`
- `external-repo/Scene/Material/StandardMaterialParamLayout.slang`
- `external-repo/Scene/Material/SerializedMaterialParams.h`

### April references
- `engine/graphics/source/graphics/material/material-system.hpp`
- `engine/graphics/source/graphics/material/material-system.cpp`
- `engine/graphics/source/graphics/material/material-type-registry.cpp`
- `engine/graphics/source/graphics/material/i-material.hpp`
- `engine/graphics/source/graphics/material/unlit-material.cpp`
- `engine/graphics/shader/material/material-data.slang`
- `engine/graphics/shader/material/material-system.slang`
- `engine/graphics/shader/material/material-factory.slang`
- `engine/graphics/shader/scene/scene-mesh.slang`
- `engine/scene/source/scene/renderer/scene-renderer.cpp`
- `engine/scene/source/scene/renderer/render-resource-registry.cpp`

## What We Should Learn From Falcor (Principles)

1. **Separation of concerns across host/shader/runtime**
   - Host tracks metadata and lifecycle; shader does evaluation/factory; runtime binds resources.
2. **Type-driven extensibility**
   - Material type registry + conformance mapping is explicit and auditable.
3. **Lifecycle and update semantics matter**
   - Fine-grained update flags avoid broad invalidation and hidden perf debt.
4. **Resource model must scale**
   - Descriptor tables and helper APIs should be first-class, not ad hoc.
5. **Tooling is part of architecture**
   - Param layout, serialization, and diagnostics are core to long-term maintainability.

## What We Should *Not* Copy Directly

1. Falcor's full legacy compatibility surface and broad historical feature baggage.
2. Falcor-specific workflow assumptions (global patterns and renderer-specific knobs).
3. Falcor's exact data layout choices where April can use cleaner modern constraints.
4. Any complexity that doesn't pay off for April's current module ownership (`graphics -> scene -> editor`).

## April Current State (Short Assessment)

### Strengths
- ABI versioning exists in header: `engine/graphics/shader/material/material-data.slang`.
- Shader-side interface path is active (`IMaterialInstance` + factory).
- Descriptor handles are integrated host->shader for current forward path.
- Material type registry exists and Unlit path is wired end-to-end.

### Structural Weaknesses
- Single payload struct (`StandardMaterialData`) is used for multiple material types.
- Descriptor table capacity is hardcoded in shader (`128/8`) and manually mirrored in scene code.
- Material lifecycle uses coarse dirty-state; no per-material update flags.
- Asset-to-material type selection currently uses string heuristic (`"unlit"` in path).
- Factory path still has type branching instead of registry/capability dispatch.
- No standardized material parameter layout/serialization contract.

## Critical Path Defects to Refactor Early

These are the issues that should be addressed early rather than deferred.

### A) Type/payload coupling defect (high priority)
- Symptom: `UnlitMaterial` writes into `StandardMaterialData` (`engine/graphics/source/graphics/material/unlit-material.cpp`).
- Risk: type ambiguity, wasted bandwidth, fragile extension for non-PBR materials.
- Refactor direction: introduce typed payload model (`MaterialDataBlob`-style or tagged payload blocks).

### B) Descriptor scalability defect (high priority)
- Symptom: fixed shader array sizes in `engine/graphics/shader/material/material-system.slang` and `engine/graphics/shader/scene/scene-mesh.slang`.
- Risk: hard cap, hidden clamping behavior, brittle when content grows.
- Refactor direction: host-provided descriptor-count defines + validation/fail-fast policy.

### C) Type resolution correctness defect (high priority)
- Symptom: path-name heuristic in `engine/scene/source/scene/renderer/render-resource-registry.cpp`.
- Risk: content-driven bugs, non-deterministic authoring behavior.
- Refactor direction: explicit `materialType` in asset contract, no string heuristics in runtime.

## Modern-Engine Target (April-Native)

### 1) Material contract
- Keep compact common header (`type/version/flags`) + typed payload block.
- Explicit type budget in ABI (max ids) and registry bounds enforcement.

### 2) Resource contract
- Descriptor tables are runtime-sized (or compile-time-defined by host), not magic constants in shader files.
- Full support path includes 2D textures, 3D textures, buffers, and samplers.

### 3) Lifecycle contract
- Per-material update flags (`data changed`, `resources changed`, `conformance changed`).
- Selective upload path in material system, not always full rebuild.

### 4) Extensibility contract
- Material type declared in asset metadata.
- Host registry + shader factory use the same source-of-truth type id.
- Conformance completeness check runs before pipeline link.

### 5) Tooling contract
- Material param layout metadata for editor/debug/automation.
- Serialized param format and round-trip tests.
- Material diagnostics surface (counts, descriptor usage, type map, invalid references).

## Gap List (Prioritized)

1. Typed payload model missing.  
2. Descriptor limits hardcoded in shader.  
3. Asset material type not explicit (path heuristic).  
4. Coarse dirty-bit update model.  
5. Factory dispatch partly hardcoded.  
6. Missing conformance preflight validation.  
7. Missing parameter layout/serialization system.  
8. Incomplete resource class support in shader context (buffer/3D expansion path).  
9. Limited material diagnostics and stats APIs.  
10. Registry lacks explicit ABI bit-budget enforcement contract.

## Refactor-Now Plan (Do Early)

### Phase 0 - correctness hardening
1. Add explicit `materialType` field in material asset and consume it in registry.
2. Remove path heuristic branch from runtime.
3. Add conformance preflight checks before program link.

### Phase 1 - architecture safety
1. Replace fixed descriptor constants with host-defined capacities.
2. Add overflow policy: fail-fast in debug, safe fallback with telemetry in runtime.
3. Add material stats API for editor/debug visibility.

### Phase 2 - extensibility baseline
1. Split payload model from `StandardMaterialData` to typed payloads.
2. Introduce per-material update flags and selective upload.
3. Add parameter layout + serialization contract.

## Validation Strategy

- **Correctness**: material-type mapping tests, conformance-failure diagnostics tests, shader decode tests per type.
- **Performance**: descriptor pressure scene benchmark + selective upload benchmark.
- **Stability**: golden-scene image diff across Standard/Unlit and mixed-material scenes.
- **Tooling**: editor displays consistent type/id/index and descriptor usage counters.

## Final Position

Use Falcor as a **reference architecture**, not an implementation template.  
For April, the best modern path is: **explicit contracts, typed payloads, scalable descriptors, selective updates, and strong diagnostics**, with early refactor of structural defects before adding more material features.
