# ADR 2026-02-08: Editor glTF Import + Preview via Asset Pipeline

## Status
Proposed

## Context
- The asset system already supports glTF import (`GltfImporter`) and produces `.gltf.asset` plus `.material.asset`/texture assets.
- The editor can trigger imports, but the Content Browser only lists files and does not open assets.
- The viewport currently renders hard-coded assets and ignores per-submesh material indices.

We need a clear, end-to-end path so a glTF imported from the editor appears in the Content Browser and can be previewed with correct materials when opened.

## Decision
- Use the existing asset pipeline as the single source of truth for glTF import and preview.
- Teach the Content Browser to recognize `.asset` files by reading asset metadata and treat mesh assets as “Mesh” items.
- On double-click of a mesh `.asset`, spawn a scene entity with a `MeshRendererComponent`, register the mesh via `RenderResourceRegistry`, and leave `materialId` invalid to use mesh material slots.
- Update `SceneRenderer` to bind per-submesh materials when the instance has no material override, using `RenderResourceRegistry::getMeshMaterialId(meshId, submesh.materialIndex)`.

## Alternatives Considered
- Directly load glTF in the editor without asset files. Rejected: bypasses cache/registry and duplicates import logic.
- Add a dedicated glTF previewer window. Rejected: higher UI complexity for minimal gain.
- Convert glTF into a proprietary scene format on import. Rejected: larger scope and loss of round-trip transparency.

## Tradeoffs
- Adds file I/O for `.asset` metadata reads in the Content Browser.
- Slightly higher draw overhead when switching materials per submesh.
- Keeps one canonical import path with caching and registry benefits.

## Impact Scope
- `engine/editor`: Content Browser asset detection and double-click behavior.
- `engine/scene`: renderer binding logic for per-submesh materials.
- `engine/asset`: glTF/material/texture import correctness (especially for embedded textures).
- `engine/graphics`: no API change; relies on existing material system/shaders.

## Compatibility / Migration
- Existing mesh rendering with explicit `materialId` continues to work (acts as override).
- Assets and registry formats remain unchanged.

## Follow-up
- Add file dialog for imports.
- Add support for embedded textures (GLB/data URI) if not already handled.
- Add thumbnails and drag-and-drop instantiation from Content Browser.
- Add an integration test: import glTF, open `.gltf.asset`, verify material textures are bound.
