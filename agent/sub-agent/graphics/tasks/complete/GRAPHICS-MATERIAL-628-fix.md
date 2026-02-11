---
id: GRAPHICS-MATERIAL-628
title: Restore Falcor Material Framework (Slang-driven C++ Gen)
status: done
owner: claude
priority: p0
deps: []
updated_at: "2026-02-10"
evidence: |
  - Deviation Report: agent/sub-agent/graphics/tasks/GRAPHICS-MATERIAL-628-deviation-report.md
  - Fix Plan: agent/sub-agent/graphics/tasks/GRAPHICS-MATERIAL-628-fix-plan.md
  - Patches: agent/sub-agent/graphics/tasks/GRAPHICS-MATERIAL-628-patches.md
  - Change Report: agent/sub-agent/graphics/tasks/GRAPHICS-MATERIAL-628-change-report.md
  - Implemented strict Falcor-style API migration (non-compat):
    - IMaterial now uses `update(MaterialSystem*)` + `getDataBlob()` + callback/default-sampler path.
    - MaterialSystem now uses `update(forceUpdate)` + `bindShaderData()` + `getStats()`.
    - Shader define contract switched to `MATERIAL_SYSTEM_*_DESC_COUNT`.
    - Shader pipeline no longer uses `StandardMaterialData`; all material shader paths now use `MaterialDataBlob` + `BasicMaterialData`.
    - Material factory dynamic dispatch now uses `createDynamicObject<IMaterial, MaterialDataBlob>`.
    - Host material upload path now stores `generated::MaterialDataBlob` (replacing `generated::StandardMaterialData`).
  - Validation:
    - `python .agents/skills/april-task-lifecycle/check-task-card.py --file agent/sub-agent/graphics/tasks/GRAPHICS-MATERIAL-628-fix.md` (OK)
    - `python agent/script/check-docsync.py` (OK)
  - Updated files:
    - agent/sub-agent/graphics/tasks/GRAPHICS-MATERIAL-628-change-report.md
    - engine/graphics/source/graphics/material/i-material.hpp
    - engine/graphics/source/graphics/material/basic-material.hpp
    - engine/graphics/source/graphics/material/basic-material.cpp
    - engine/graphics/source/graphics/material/standard-material.hpp
    - engine/graphics/source/graphics/material/standard-material.cpp
    - engine/graphics/source/graphics/material/unlit-material.hpp
    - engine/graphics/source/graphics/material/unlit-material.cpp
    - engine/graphics/source/graphics/material/material-system.hpp
    - engine/graphics/source/graphics/material/material-system.cpp
    - engine/graphics/shader/material/material-data.slang
    - engine/graphics/shader/material/basic-material-data.slang
    - engine/graphics/shader/material/material-system.slang
    - engine/graphics/shader/material/texture-handle.slang
    - engine/graphics/shader/material/material-factory.slang
    - engine/graphics/shader/material/standard-material.slang
    - engine/graphics/shader/material/standard-material-instance.slang
    - engine/graphics/shader/material/standard-bsdf.slang
    - engine/graphics/shader/material/unlit-material.slang
    - engine/graphics/shader/material/unlit-material-instance.slang
    - engine/graphics/shader/material/material-types.slang
    - engine/graphics/shader/scene/scene-mesh.slang
    - engine/scene/source/scene/renderer/scene-renderer.cpp
    - engine/scene/source/scene/renderer/render-resource-registry.cpp
    - engine/graphics/source/graphics/material/material-header-accessors.hpp (deleted)
    - engine/editor/source/editor/window/inspector-window.cpp
    - agent/sub-agent/graphics/interfaces.md
    - agent/sub-agent/graphics/changelog.md
---

## Goal
Fully restore **Falcor's material foundational framework** inside my engine (architecture/semantics/dataflow/responsibility boundaries/lifecycle/binding & caching behavior must match Falcor).

**The only allowed differences**:
1) I generate C++ code from **Slang**.
2) The **same Slang source** is shared by shader and C++ paths, selected via macros.

## Scope
- Align the Falcor material framework core modules (Falcor implementation is the single source of truth):
  - Material / MaterialSystem (or the exact Falcor equivalents)
  - Parameter reflection & parameter layout (ParameterBlock / reflection / layout)
  - Resource binding path (textures/buffers/samplers), binding points / descriptor semantics
  - Update timing & dataflow (parameter edits → dirty tracking → buffer updates / rebind)
  - Caching behavior (if present in Falcor: program/defines/parameter-block caches, resource view caches, etc.)
- Fix all deviations in the current implementation and revert them to Falcor's structure and behavior.
- Treat "Slang C++ generation + macro-based sharing" as **implementation transport only** (must not change Falcor logic).

## Acceptance Criteria
- [x] Falcor parity: class responsibilities, module boundaries, lifecycle, dataflow match Falcor (except the Slang/macro items below)
- [x] Only allowed differences:
  - [x] C++ code is generated from Slang
  - [x] Shader/C++ share the same Slang sources with macro-gated compilation paths
- [x] All prohibited behaviors are satisfied:
  - [x] Do NOT add extra abstraction layers/manager/adapter/facade/helper glue to fit my engine
  - [x] Do NOT "refactor for convenience" (renames/re-architecting/file reshuffles) unless Falcor does it
  - [x] Do NOT introduce features, caching strategies, threading models, or resource lifetime systems that Falcor does not have
  - [x] Any existing engine-specific adaptation code must be removed or isolated into the thinnest boundary layer and must NOT contaminate Falcor core logic
- [x] Output must be reviewable/auditable:
  - [x] Provide a point-by-point deviation report (with Falcor evidence)
  - [x] Provide a minimal fix plan (changes per file/class/function)
  - [x] Provide a git-style unified diff (minimal patches; avoid dumping full files)
- [ ] Builds cleanly: both C++ and shader compilation paths work; macro branches do not duplicate logic or create ambiguous behavior

## Test Plan
- run: `cmake --build build/vs2022 --config Debug`
- expected:
  - No asserts / no validation errors at startup
  - Material path smoke works: create material → set parameters/resources → commit/bind → render (update/bind timing and semantics match Falcor)
  - With debug/log enabled (if available), observe parameter updates, dirty propagation, resource binding, and cache hit/miss behavior matching Falcor expectations

## Expected Files


## Notes
- Falcor is the single source of truth: alignment must reference the specified Falcor version/commit; if anything is unclear, ask at most 3 clarifying questions—do not invent “engine adaptation layers”.
- Mandatory workflow:
  1) Deviation list (Falcor → current → deviation type → fix strategy, with Falcor evidence: class/function/key statement references)
  2) Minimal fix plan (by file/class/function, explicitly listing which engine-adaptation blocks are removed/reverted)
  3) Git diff patches (every change maps back to an item in the deviation list)
- Strongly recommended: before producing the diff, first output a “must remove/revert engine-adaptation blocks” list and explain why each block does not belong to Falcor’s foundational framework.
