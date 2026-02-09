---
id: ASSET-003
title: Extend glTF import to support embedded textures
status: in_progress
owner: codex
priority: p1
deps: []
updated_at: 2026-02-09
evidence: ""
---

## Goal
Support embedded texture sources (GLB bufferView and data URI) in glTF import.

## Scope
- Resolve embedded texture sources into texture assets.
- Improve diagnostics for missing, unsupported, and decode-failure cases.

## Out of Scope
- Editor UI and renderer-side changes.

## Acceptance Criteria
- [ ] Importing embedded-texture glTF produces texture assets referenced by materials.
- [ ] Logs clearly distinguish missing/unsupported/failed decode cases.

## Test Plan
- manual: import a GLB with embedded textures and verify bound material textures.

## Expected Files
- `engine/asset/source/asset/importer/gltf-importer.cpp`

## Notes
- Risk: medium (image decode and import path branching).
