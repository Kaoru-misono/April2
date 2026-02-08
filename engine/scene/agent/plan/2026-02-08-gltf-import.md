# Scene Plan: Per-Submesh Materials for glTF (2026-02-08)

## Current status
- done: Mesh import stores `Submesh.materialIndex` and renderer draws all submeshes.
- in progress:
- risks: Renderer ignores per-submesh material slots and always uses one material for the whole mesh.

## Milestones
### M1: Submesh Material Binding
- goal: Use mesh material slots when instances do not override a material.
- deliverables:
- Update `SceneRenderer::renderMeshInstances` to select a material per submesh:
  - If `MeshRendererComponent::materialId` is valid, use it as an override.
  - Otherwise use `RenderResourceRegistry::getMeshMaterialId(meshId, submesh.materialIndex)`.
- Bind textures and set `materialIndex` for each submesh before the draw call.
- acceptance:
- A mesh with multiple glTF materials renders each submesh with its corresponding textures when no override is set.

### M2: Diagnostics
- goal: Easier debugging when material slots are missing.
- deliverables:
- Add a warning when a submesh references a missing material slot (once per mesh).
- acceptance:
- Missing materials are surfaced without spamming logs.

## Requests to other modules
- Editor: Leave `materialId` invalid when spawning mesh assets so slot-based materials can apply.
- Asset: Ensure glTF import writes material slot indices consistently with `Submesh.materialIndex`.
