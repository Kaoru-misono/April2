---
id: GRAPHICS-MATERIAL-402
title: Add one non-standard material type through full pipeline
status: todo
owner: codex
priority: p2
deps: [GRAPHICS-MATERIAL-401, GRAPHICS-MATERIAL-303]
updated_at: 2026-02-09
evidence: ""
---

## Goal
Validate extensibility by adding one additional material type end-to-end.

## Scope
- Add minimal non-standard material (for example `Unlit`).
- Provide host representation + Slang implementation + factory registration.
- Ensure import/runtime can instantiate and render it.

## Acceptance Criteria
- [ ] New material type can be created and rendered in scene.
- [ ] Scene shader path requires no core branching changes for the new type.
- [ ] Type conformance and material registry paths both participate.

## Test Plan
- build: `cmake --build build/x64-debug --target April_graphics April_scene`
- manual: create one test material asset of new type and render mesh with it.

## Expected Files
- `engine/graphics/source/graphics/material/unlit-material.hpp`
- `engine/graphics/source/graphics/material/unlit-material.cpp`
- `engine/graphics/shader/material/unlit-material.slang`
- `engine/graphics/shader/material/material-factory.slang`

