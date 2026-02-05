# MaterialAsset System Implementation Plan

*Created: 2026-02-05*

## Overview

Implement a complete MaterialAsset system that integrates with the existing asset pipeline and GLTF import workflow. Materials store PBR parameters, reference TextureAssets by UUID, and are automatically imported from GLTF files during mesh import.

## Architecture

### Three-Layer System
1. **Asset Layer** (`engine/asset/`) - MaterialAsset class for CPU metadata and JSON serialization
2. **Graphics Layer** (`engine/graphics/`) - Material class for GPU resources and parameter binding
3. **Renderer Layer** (`engine/graphics/renderer/`) - Material cache and binding in SceneRenderer

### Material Storage Strategy
- **MaterialAsset**: JSON metadata with PBR parameters and texture UUIDs
- **Material**: Runtime GPU constant buffer + loaded textures
- **No shader variants**: Phase 1 uses unified PBR shader for all materials

---

## Phase 1: MaterialAsset Foundation

Create the core MaterialAsset class with JSON serialization.

- [ ] **Task: Create MaterialAsset header**
  - [ ] Create `engine/asset/source/asset/material-asset.hpp`
  - [ ] Define `MaterialParameters` struct:
    - `float4 baseColorFactor`
    - `float metallicFactor`
    - `float roughnessFactor`
    - `float3 emissiveFactor`
    - `float occlusionStrength`
    - `float normalScale`
    - `float alphaCutoff`
    - `std::string alphaMode` ("OPAQUE", "MASK", "BLEND")
    - `bool doubleSided`
  - [ ] Define `TextureReference` struct:
    - `std::string assetId` (UUID)
    - `int texCoord` (UV channel)
  - [ ] Define `MaterialTextures` struct:
    - `std::optional<TextureReference> baseColorTexture`
    - `std::optional<TextureReference> metallicRoughnessTexture`
    - `std::optional<TextureReference> normalTexture`
    - `std::optional<TextureReference> occlusionTexture`
    - `std::optional<TextureReference> emissiveTexture`
  - [ ] Add `NLOHMANN_DEFINE_TYPE_INTRUSIVE` macros for JSON serialization
  - [ ] Define `MaterialAsset` class inheriting from `Asset`

- [ ] **Task: Implement MaterialAsset**
  - [ ] Create `engine/asset/source/asset/material-asset.cpp`
  - [ ] Implement `serializeJson()` override
  - [ ] Implement `deserializeJson()` override
  - [ ] Implement `computeDDCKey()` (return empty for Phase 1)

- [ ] **Task: Add MaterialAsset to AssetManager**
  - [ ] Update `engine/asset/source/asset/asset-manager.hpp`:
    - Add `loadMaterialAsset()` method
    - Add `saveMaterialAsset()` method
  - [ ] Update `engine/asset/source/asset/asset-manager.cpp`:
    - Implement material loading in `loadAssetFromFile<MaterialAsset>()`
    - Implement material saving
    - Update asset registry to handle material type

- [ ] **Task: Create test MaterialAsset**
  - [ ] Create `content/materials/test_red.material.asset` manually
  - [ ] Set baseColorFactor to red [0.8, 0.2, 0.2, 1.0]
  - [ ] Set metallic=0.0, roughness=0.5
  - [ ] Verify loading with unit test

- [ ] **Task: Write unit tests**
  - [ ] Create `engine/test/test-material-asset.cpp`
  - [ ] Test MaterialAsset JSON serialization
  - [ ] Test MaterialAsset deserialization
  - [ ] Test texture reference handling
  - [ ] Test parameter validation

---

## Phase 2: Runtime Material System

Create GPU material representation in graphics module.

- [ ] **Task: Create Material header**
  - [ ] Create `engine/graphics/source/graphics/resources/material.hpp`
  - [ ] Define `MaterialData` struct (GPU constant buffer layout):
    - `float4 baseColorFactor`
    - `float3 emissiveFactor`
    - `float metallicFactor`
    - `float roughnessFactor`
    - `float occlusionStrength`
    - `float normalScale`
    - `float alphaCutoff`
    - `uint32_t flags` (texture presence bits)
  - [ ] Define `Material` class with:
    - `setParameters()` method
    - `setTexture()` method
    - `bind()` method for parameter block binding
    - `isTransparent()` and `isDoubleSided()` accessors

- [ ] **Task: Implement Material**
  - [ ] Create `engine/graphics/source/graphics/resources/material.cpp`
  - [ ] Implement constructor (create constant buffer)
  - [ ] Implement `setParameters()` (update constant buffer)
  - [ ] Implement `setTexture()` (store texture refs)
  - [ ] Implement `bind()` (bind buffer and textures to parameter block)
  - [ ] Create default linear sampler

- [ ] **Task: Add Material creation to Device**
  - [ ] Update `engine/graphics/source/graphics/rhi/render-device.hpp`:
    - Add `createMaterialFromAsset()` method
  - [ ] Update `engine/graphics/source/graphics/rhi/render-device.cpp`:
    - Implement material creation from MaterialAsset
    - Load referenced texture assets
    - Create Material instance with all textures loaded

- [ ] **Task: Create default material**
  - [ ] Add default material creation in Device or SceneRenderer
  - [ ] Use white baseColor, metallic=0, roughness=0.5

- [ ] **Task: Write GPU tests**
  - [ ] Create `engine/test/test-material-gpu.cpp`
  - [ ] Test Material constant buffer creation
  - [ ] Test texture binding
  - [ ] Test parameter updates

---

## Phase 3: SceneRenderer Integration

Integrate materials into the rendering pipeline.

- [ ] **Task: Add material cache to SceneRenderer**
  - [ ] Update `engine/graphics/source/graphics/renderer/scene-renderer.hpp`:
    - Add `std::unordered_map<core::UUID, core::ref<Material>> m_materialCache`
    - Add `std::vector<core::ref<Material>> m_materials` (materials for current mesh)
    - Add `core::ref<Material> m_defaultMaterial`
    - Add `getMaterial(core::UUID const& id)` helper method

- [ ] **Task: Update SceneRenderer constructor**
  - [ ] Create default material in constructor
  - [ ] Initialize material cache

- [ ] **Task: Load materials from mesh**
  - [ ] In `renderMeshEntities()`, after getting mesh:
    - Extract material list from mesh metadata (or MeshRendererComponent)
    - Load each material by UUID using AssetManager
    - Cache in `m_materials` vector
    - Use `m_defaultMaterial` as fallback

- [ ] **Task: Update shader to use material parameters**
  - [ ] Modify inline shader strings (or create `.slang` files):
    - Add `MaterialData` constant buffer to pixel shader
    - Add texture samplers (baseColorTex, etc.)
    - Sample baseColorTexture if present (check flags)
    - Apply material parameters to lighting calculation
  - [ ] Update vertex shader to output UV coordinates

- [ ] **Task: Bind materials per submesh**
  - [ ] In `renderMeshEntities()` loop over submeshes:
    - Get material from `m_materials[submesh.materialIndex]`
    - Call `material->bind(rootVar["material"])`
    - Bind pipeline and draw

- [ ] **Task: Update MeshRendererComponent**
  - [ ] Update `engine/scene/source/scene/components.hpp`:
    - Change `materialId` type from `uint32_t` to `std::string` (UUID)
    - Or add `std::vector<std::string> materialIds` for multi-material support

- [ ] **Task: Test rendering with materials**
  - [ ] Create test scene with colored materials
  - [ ] Verify each submesh renders with correct material
  - [ ] Verify parameter changes reflect in rendering

---

## Phase 4: GLTF Material Import

Automatically extract and import materials during GLTF mesh import.

- [ ] **Task: Extract materials in DDC mesh compilation**
  - [ ] Update `engine/asset/source/asset/ddc/ddc-manager.cpp`:
    - In `compileInternalMesh()` after line 213
    - Loop through `model.materials` array
    - For each GLTF material:
      - Create `MaterialAsset` instance
      - Extract PBR parameters from `pbrMetallicRoughness`
      - Extract emissive, alphaMode, doubleSided
      - Store in material list

- [ ] **Task: Import textures referenced by materials**
  - [ ] For each texture in GLTF material:
    - Get texture from `model.textures[texIndex]`
    - Get image from `model.images[texture.source]`
    - Resolve path: `gltfDir / image.uri`
    - Call `AssetManager::importAsset()` for texture
    - Store texture UUID in MaterialAsset

- [ ] **Task: Save MaterialAsset files**
  - [ ] For each extracted material:
    - Generate filename: `{materialName}.material.asset`
    - Save in same directory as GLTF file
    - Call `AssetManager::saveMaterialAsset()`

- [ ] **Task: Link materials to mesh**
  - [ ] Store material UUIDs in mesh metadata
  - [ ] Or update MeshRendererComponent to reference materials
  - [ ] Ensure `submesh.materialIndex` correctly indexes into material array

- [ ] **Task: Handle texture path resolution**
  - [ ] Handle external texture files (image.uri)
  - [ ] Handle embedded textures (image.bufferView)
  - [ ] Handle data URIs (base64 encoded)

- [ ] **Task: Test GLTF import**
  - [ ] Import simple GLTF with materials (e.g., BoxTextured.gltf)
  - [ ] Verify .material.asset files created
  - [ ] Verify texture references resolved
  - [ ] Verify material parameters match GLTF values

---

## Phase 5: Advanced Features & Testing

Add full texture support and comprehensive testing.

- [ ] **Task: Implement all texture slots**
  - [ ] baseColorTexture (albedo)
  - [ ] metallicRoughnessTexture (combined)
  - [ ] normalTexture (tangent space)
  - [ ] occlusionTexture (ambient occlusion)
  - [ ] emissiveTexture (emissive glow)

- [ ] **Task: Implement alpha blending**
  - [ ] Add blend state for transparent materials (alphaMode="BLEND")
  - [ ] Sort transparent objects back-to-front
  - [ ] Render opaque first, then transparent

- [ ] **Task: Implement alpha testing**
  - [ ] Add discard in pixel shader for alphaMode="MASK"
  - [ ] Use alphaCutoff threshold

- [ ] **Task: Implement double-sided rendering**
  - [ ] Disable backface culling for doubleSided materials
  - [ ] Handle normal flipping in shader

- [ ] **Task: Create comprehensive tests**
  - [ ] Test material import integration
  - [ ] Test rendering with all texture types
  - [ ] Test alpha blending
  - [ ] Test double-sided materials

- [ ] **Task: Manual testing with sample models**
  - [ ] Download glTF-Sample-Models (DamagedHelmet, MetalRoughSpheres)
  - [ ] Import and render in engine
  - [ ] Verify visual correctness
  - [ ] Profile performance

---

## Verification Steps

After each phase:

1. **Phase 1**: Load MaterialAsset JSON, verify all fields present
2. **Phase 2**: Create Material from asset, verify GPU buffer created
3. **Phase 3**: Render mesh with material, verify colors match parameters
4. **Phase 4**: Import GLTF, verify .material.asset files generated
5. **Phase 5**: Import complex GLTF (DamagedHelmet), verify all materials render correctly

---

## File Locations

### New Files
- `engine/asset/source/asset/material-asset.hpp`
- `engine/asset/source/asset/material-asset.cpp`
- `engine/graphics/source/graphics/resources/material.hpp`
- `engine/graphics/source/graphics/resources/material.cpp`
- `engine/test/test-material-asset.cpp`
- `engine/test/test-material-gpu.cpp`

### Modified Files
- `engine/asset/source/asset/asset-manager.hpp` (~line 376)
- `engine/asset/source/asset/asset-manager.cpp`
- `engine/asset/source/asset/ddc/ddc-manager.cpp` (~line 213)
- `engine/graphics/source/graphics/rhi/render-device.hpp`
- `engine/graphics/source/graphics/rhi/render-device.cpp`
- `engine/graphics/source/graphics/renderer/scene-renderer.hpp`
- `engine/graphics/source/graphics/renderer/scene-renderer.cpp` (~line 295)
- `engine/scene/source/scene/components.hpp` (~line 71)

---

## JSON File Format Example

```json
{
    "guid": "a1b2c3d4-5e6f-7890-abcd-ef1234567890",
    "source_path": "",
    "type": "Material",
    "parameters": {
        "baseColorFactor": [0.8, 0.2, 0.2, 1.0],
        "metallicFactor": 0.0,
        "roughnessFactor": 0.5,
        "emissiveFactor": [0.0, 0.0, 0.0],
        "occlusionStrength": 1.0,
        "normalScale": 1.0,
        "alphaCutoff": 0.5,
        "alphaMode": "OPAQUE",
        "doubleSided": false
    },
    "textures": {
        "baseColorTexture": {
            "assetId": "b2c3d4e5-6f78-90ab-cdef-123456789abc",
            "texCoord": 0
        }
    }
}
```

---

## Notes

- Materials use unified PBR shader (no per-material shader variants in Phase 1)
- Texture references stored as UUID strings for stability
- Material indices in mesh submeshes already extracted by GLTF import
- No DDC compilation needed for Phase 1 (materials are JSON metadata)
- Future: Add shader variants, material instances, material editor UI
