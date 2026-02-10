---
id: GRAPHICS-MATERIAL-625
title: Align material factory to Falcor extension-style dispatch
status: done
owner: codex
priority: p0
deps: [GRAPHICS-MATERIAL-624]
updated_at: "2026-02-10"
evidence: "Replaced standalone factory helpers with `extension MaterialSystemContext` methods and moved dynamic object dispatch there. Verified with Slang typecheck on `material-factory.slang` and `scene-mesh.slang`."
---

## Goal
Match Falcor's material factory pattern by exposing material creation and pattern generation through MaterialSystem extensions.

## Scope
- Convert material factory implementation to extension methods on `MaterialSystemContext`.
- Keep dynamic dispatch based on material type id using `createDynamicObject<IMaterial, StandardMaterialData>()`.
- Update scene usage to call extension API directly.

## Acceptance Criteria
- [x] `material-factory.slang` is extension-based and no longer uses the old helper struct shape.
- [x] Material instance lookup uses `sd.materialID` and routes to `setupMaterialInstance(...)`.
- [x] Scene shader calls the extension method path for material instance creation.

## Test Plan
- `build/x64-debug/_deps/slang-src/bin/slangc.exe -no-codegen -I engine/graphics/shader engine/graphics/shader/material/material-factory.slang`
- `build/x64-debug/_deps/slang-src/bin/slangc.exe -no-codegen -I engine/graphics/shader engine/graphics/shader/scene/scene-mesh.slang`

## Expected Files
- `engine/graphics/shader/material/material-factory.slang`
- `engine/graphics/shader/scene/scene-mesh.slang`
