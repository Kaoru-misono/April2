---
id: GRAPHICS-MATERIAL-501
title: Fix material factory shader import missing material-types
status: done
owner: opencode
priority: p0
deps: []
updated_at: 2026-02-09
evidence: "Fixed: Added `import material.material_types;` to material-factory.slang imports. Shader compilation errors for MaterialFlags and MaterialType are resolved. Editor now progresses past material system initialization but fails on missing scene-mesh.slang file path (separate issue). Verification: `timeout 10s ./build/x64-debug/bin/editor` shows MaterialSystem initialized successfully without prior compilation errors."
---

## Goal
Fix shader compilation error in material-factory.slang due to missing import of material-types.slang

## Scope
- Add missing import to material-factory.slang
- Verify shader compilation succeeds
- Test editor launch without shader errors

## Acceptance Criteria
- [ ] material-factory.slang compiles without undefined identifier errors
- [ ] Editor launches successfully without shader compilation failures
- [ ] Material system initializes correctly

## Test Plan
- shader: Test editor launch `./build/x64-debug/bin/editor`
- build: Verify no shader compilation errors in logs

## Expected Files
- `engine/graphics/shader/material/material-factory.slang`