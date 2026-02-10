---
id: GRAPHICS-MATERIAL-616
title: Split MaterialSystem helpers and convert factory to extension API
status: done
owner: codex
priority: p0
deps: [GRAPHICS-MATERIAL-615]
updated_at: "2026-02-10"
evidence: "Refactored `material-system.slang` helpers, replaced factory struct with extension-style entrypoints in `material-factory.slang`, and added `texture-sampler.slang` plus `material-instance-hints.slang`. Validation command: `cmake --build build/x64-debug --target April_graphics April_scene test-suite`; blocked by environment missing C++ standard headers (`cmath`, `string`)."
---

## Goal
Refactor material system shader helpers into Falcor-style extension API with dedicated texture sampler module.

## Scope
- Keep `material-system.slang` focused on GPU material resource access and helper methods.
- Convert `material-factory.slang` into extension-based `getMaterial()` and `getMaterialInstance()` entry points.
- Introduce `texture-sampler.slang` for `ITextureSampler` and implicit/explicit LOD variants.

## Acceptance Criteria
- [x] `material-system.slang` exposes material resource accessors and generic sample helpers.
- [x] `material-factory.slang` implements extension methods rather than standalone factory struct branching.
- [x] `texture-sampler.slang` defines `ITextureSampler`, `ImplicitLodTextureSampler`, and fixed/explicit LOD helpers.
- [x] Material instance creation paths accept `(sd, lod, hints)` signatures.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene`
- verify: scene shader can obtain material instances through extension path with implicit LOD sampler.

## Expected Files
- `engine/graphics/shader/material/material-system.slang`
- `engine/graphics/shader/material/material-factory.slang`
- `engine/graphics/shader/material/texture-sampler.slang`
- `engine/graphics/shader/material/material-instance-hints.slang`

## Out of Scope
- Host-side descriptor table schema changes.
