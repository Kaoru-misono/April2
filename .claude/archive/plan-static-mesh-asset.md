# StaticMeshAsset Implementation Plan

## Overview

Implement `StaticMeshAsset` following the same pattern as `TextureAsset`, with:
- Asset metadata in JSON format (.asset files)
- Compiled binary format via DDC (Derived Data Cache)
- Runtime GPU mesh creation in graphics module

## Source File Support

- `.gltf` / `.glb` - glTF 2.0 (via tinygltf, already in core)
- Future: `.obj`, `.fbx`

## Standard Vertex Format

All meshes compiled to a consistent interleaved vertex format with required and optional attributes:

```cpp
// Required attributes (always present)
struct StaticVertexBase
{
    float position[3];   // 12 bytes - offset 0
    float normal[3];     // 12 bytes - offset 12
    float tangent[4];    // 16 bytes - offset 24 (w = bitangent sign)
    float texCoord0[2];  // 8 bytes  - offset 40
};
// Base: 48 bytes per vertex

// Optional attributes (included based on source mesh data)
// - texCoord1[2]: +8 bytes  (lightmap UV, etc.)
// - color[4]:     +16 bytes (vertex color RGBA)

// Flags in MeshHeader indicate which optional attributes are present
enum VertexFlags : uint8_t
{
    HasTexCoord1 = 1 << 0,
    HasColor     = 1 << 1,
};
```

## Files to Create/Modify

### Asset Module (`engine/asset/source/asset/`)

| File | Action | Description |
|------|--------|-------------|
| `static-mesh-asset.hpp` | Create | StaticMeshAsset class + MeshImportSettings |
| `static-mesh-asset.cpp` | Create | computeDDCKey implementation |
| `blob-header.hpp` | Modify | Add MeshHeader, Submesh, MeshPayload structs |
| `ddc/ddc-manager.hpp` | Modify | Add getOrCompileMesh() declaration |
| `ddc/ddc-manager.cpp` | Modify | Implement mesh compilation with tinygltf |
| `asset-manager.hpp` | Modify | Add getMeshData() method |

### Graphics Module (`engine/graphics/source/graphics/`)

| File | Action | Description |
|------|--------|-------------|
| `rhi/render-device.hpp` | Modify | Add createMeshFromAsset() |
| `rhi/render-device.cpp` | Modify | Implement mesh buffer creation |

## Detailed Design

### 1. MeshImportSettings

```cpp
struct MeshImportSettings
{
    bool optimize = true;           // Use meshoptimizer
    bool generateTangents = true;   // Compute tangent space
    bool flipWindingOrder = false;  // Flip triangle winding
    float scale = 1.0f;             // Uniform scale factor

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(...)
};
```

### 2. StaticMeshAsset

```cpp
class StaticMeshAsset : public Asset
{
public:
    StaticMeshAsset() : Asset(AssetType::Mesh) {}

    MeshImportSettings m_settings;

    void serializeJson(nlohmann::json& outJson) override;
    bool deserializeJson(const nlohmann::json& inJson) override;
    [[nodiscard]] auto computeDDCKey() const -> std::string;
};
```

### 3. Binary Format (MeshHeader)

```cpp
struct Submesh
{
    uint32_t indexOffset;    // Offset in index buffer
    uint32_t indexCount;     // Number of indices
    uint32_t materialIndex;  // Material slot
};

struct MeshHeader
{
    static constexpr uint32_t kMagic = 0x41504D58; // "APMX"

    uint32_t magic;
    uint32_t version;
    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t vertexStride;      // 48+ based on flags
    uint32_t indexFormat;       // 0=uint16, 1=uint32
    uint32_t submeshCount;
    uint32_t flags;             // VertexFlags
    float boundsMin[3];         // AABB min
    float boundsMax[3];         // AABB max
    uint64_t vertexDataSize;
    uint64_t indexDataSize;
    // Followed by: Submesh[submeshCount], vertex data, index data
};

struct MeshPayload
{
    MeshHeader header;
    std::span<Submesh const> submeshes;
    std::span<std::byte const> vertexData;
    std::span<std::byte const> indexData;
};
```

### 4. DDCManager::compileInternalMesh()

1. Load glTF using tinygltf
2. For each primitive in mesh:
   - Extract position, normal, texcoord attributes
   - Generate tangents if needed (mikktspace or simple)
   - Convert to StaticVertex format
3. Optionally optimize with meshoptimizer
4. Build MeshHeader with bounds
5. Serialize to binary blob

### 5. Device::createMeshFromAsset()

1. Call assetManager.getMeshData() to get MeshPayload
2. Create vertex buffer with vertexData
3. Create index buffer with indexData
4. Return VertexArrayObject or new StaticMesh wrapper

## Implementation Order

1. **blob-header.hpp** - Add MeshHeader, Submesh, MeshPayload
2. **static-mesh-asset.hpp/cpp** - Create asset class
3. **ddc-manager** - Add mesh compilation (tinygltf integration)
4. **asset-manager.hpp** - Add getMeshData()
5. **render-device** - Add createMeshFromAsset()
6. **Tests** - Add test-asset.cpp mesh tests

## Verification

1. Build asset module: `cmake --build build/vs2022 --target April_asset`
2. Build graphics module: `cmake --build build/vs2022 --target April_graphics`
3. Run tests: `./build/vs2022/bin/Debug/test-asset.exe`
4. Test with sample glTF file (cube/box primitive)
