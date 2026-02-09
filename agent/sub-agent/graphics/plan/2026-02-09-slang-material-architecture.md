# Graphics Plan: Slang Material Architecture Migration (2026-02-09)

## Current status
- done: Engine has basic `MaterialSystem` + `StandardMaterialData` upload and per-draw texture binding.
- done: Shader-side has `IMaterialInstance`/`StandardMaterialInstance` scaffolding in `engine/graphics/shader/material/`.
- gap: Scene forward shader still directly reads `StructuredBuffer<StandardMaterialData>` and fixed textures (`scene-mesh.slang`), not the Slang material factory/interface path.
- gap: GPU resource layout is per-draw ad hoc bindings, not a unified material parameter block like Falcor's `MaterialSystem`.

## Reference baseline (Falcor)
- `falcor-material-reference/Scene/Material/MaterialSystem.slang`
- `falcor-material-reference/Scene/Material/MaterialFactory.slang`
- `falcor-material-reference/Rendering/Materials/IMaterial.slang`
- `falcor-material-reference/Rendering/Materials/IMaterialInstance.slang`
- `falcor-material-reference/Scene/Material/MaterialData.slang`

## Target architecture
- Host side:
- Keep `RenderID` for engine object identity.
- Introduce explicit GPU material table index and stable material type id.
- `MaterialSystem` owns all material resources (data buffer + descriptor arrays for textures/samplers/buffers).
- Shader side:
- Introduce a shader-visible `MaterialSystem` struct + `MaterialFactory` extension.
- Access material by `materialID` and create dynamic object via Slang interface (`IMaterial`/`IMaterialInstance`).
- Scene shading entrypoints consume unified material instance API instead of hardcoding `StandardMaterialData`.

## Milestones
### M1: GPU Data Contract Stabilization
- goal: Make host->GPU material indexing and payload layout explicit and verifiable.
- deliverables:
- Add GPU material index mapping API in graphics/scene boundary.
- Add debug validation for out-of-range material IDs and missing descriptors.
- Define material payload ABI (header + payload) in a shared Slang/C++ source.
- acceptance:
- Any draw call can be traced from mesh submesh slot -> render material -> GPU material index deterministically.

### M2: MaterialSystem Parameter Block
- goal: Replace per-draw texture binds with material-system-managed descriptor arrays.
- deliverables:
- Add material texture/sampler descriptor tables to engine-side `MaterialSystem`.
- Bind one material-system parameter block in scene passes.
- Replace `baseColorTexture`/`normalTexture` style standalone shader globals in `scene-mesh.slang`.
- acceptance:
- Scene pass binds material resources once per pass, not once per draw.

### M3: Slang Dynamic Material Dispatch
- goal: Route shading through `IMaterial` and `IMaterialInstance`.
- deliverables:
- Add `material/material-system.slang` and `material/material-factory.slang` (adapted from Falcor structure).
- Implement `getMaterial(materialID)` + `getMaterialInstance(...)` path.
- Wire `StandardMaterial` as first concrete material type through dynamic object creation.
- acceptance:
- Forward path compiles and shades via interface dispatch while matching current standard material look.

### M4: Multi-Material Extensibility
- goal: Support adding new material classes without changing scene shaders.
- deliverables:
- Add host-side material type registry and type-conformance aggregation at program build time.
- Add at least one non-standard material stub path (can be compile-time disabled).
- Document extension process in `sub-agent/graphics/interfaces.md`.
- acceptance:
- Adding a material implementation requires no edits in core scene shader entrypoint logic.

## Verification plan
- automated:
- Extend tests to validate:
- material slot -> GPU material index mapping
- descriptor-table index validity
- runtime:
- Add one-frame debug dump mode:
- per-submesh material slot index
- resolved GPU material index
- bound texture descriptor ids
- visual:
- Reimport and render `content/model/Sponza/glTF/Sponza.gltf`, compare before/after screenshots for slot consistency.

## Risks
- Slang dynamic object and type conformance integration may affect pipeline compilation cache behavior.
- Descriptor-array based material resources require careful backend compatibility checks (D3D12/Vulkan).
- Transitional period may need dual-path shading (legacy + new path) to avoid editor regression.

## Requests to other modules
- Scene:
- Provide a pass-level binding point for graphics `MaterialSystem` parameter block.
- Keep `materialId` transport in `FrameSnapshot` stable while graphics migrates internal representation.
- Asset:
- Keep imported material metadata stable for texture slot semantics and UV set indices.
- Editor:
- Expose material type and GPU material index in debug inspector (read-only) for validation.
