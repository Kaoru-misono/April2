# Asset Tasks

> Every task must be independently mergeable. Avoid big-bang PRs.

## Task Template
- ID: ASSET-000
- Goal: Define the asset handle/load API contract.
- Dependencies: (none)
- Scope: Document asset public headers in `sub-agent/asset/interfaces.md`.
- Out of scope: No asset pipeline changes.
- Acceptance Criteria:
- `sub-agent/asset/interfaces.md` lists all public headers
- `changelog.md` updated if APIs change
- Test Plan: N/A (docs only)
- Expected files touched:
- `sub-agent/asset/interfaces.md`
- `sub-agent/asset/changelog.md`
- Risk: Low
- Owner: TBD

---

## Backlog
- ASSET-001 Document import configuration options
- ASSET-002 Add importer regression test

## In Progress
- ID: ASSET-003
- Goal: Extend glTF import to support embedded textures (GLB bufferView / data URI).
- Dependencies: `engine/asset/source/asset/importer/gltf-importer.cpp`.
- Scope: Resolve embedded texture sources into texture assets; improve diagnostics.
- Out of scope: Editor UI and renderer changes.
- Acceptance Criteria:
- Importing embedded-texture glTF produces texture assets referenced by materials.
- Logs distinguish missing/unsupported/failed decode cases.
- Test Plan: Import a GLB with embedded textures and verify material textures are bound.
- Expected files touched:
- `engine/asset/source/asset/importer/gltf-importer.cpp`
- Risk: Medium (image decoding and import path).
- Owner: Agent
