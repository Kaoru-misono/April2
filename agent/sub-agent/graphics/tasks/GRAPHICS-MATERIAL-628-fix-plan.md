# GRAPHICS-MATERIAL-628 Fix Plan

## Scope

This plan restores Falcor's material framework architecture. Changes are organized by file, with each change mapped to a deviation from the report.

---

## Phase 1: MaterialHeader Binary Layout (P0)

### File: `engine/graphics/shader/material/material-data.slang`

**Current**:
```slang
struct MaterialHeader {
    uint abiVersion;
    uint materialType;
    uint flags;
    uint alphaMode;
    uint activeLobes;
    uint reserved0;
    uint reserved1;
    uint reserved2;
}; // 32 bytes
```

**Target** (Falcor parity):
```slang
struct MaterialHeader {
    uint4 packedData;

    // Bit layout accessors (match Falcor MaterialData.slang)
    void setMaterialType(uint type);
    uint getMaterialType();
    void setAlphaMode(uint mode);
    uint getAlphaMode();
    void setAlphaThreshold(float16_t threshold);
    float16_t getAlphaThreshold();
    void setNestedPriority(uint priority);
    uint getNestedPriority();
    void setActiveLobes(uint lobes);
    uint getActiveLobes();
    void setDefaultTextureSamplerID(uint id);
    uint getDefaultTextureSamplerID();
    void setDoubleSided(bool v);
    bool isDoubleSided();
    void setThinSurface(bool v);
    bool isThinSurface();
    void setEmissive(bool v);
    bool isEmissive();
    void setIsBasicMaterial(bool v);
    bool isBasicMaterial();
    void setIoR(float16_t ior);
    float16_t getIoR();
    void setAlphaTextureHandle(TextureHandle handle);
    TextureHandle getAlphaTextureHandle();
}; // 16 bytes
```

**Changes**:
1. Replace 8 uint fields with uint4 packedData
2. Add bit-packing accessor methods
3. Add PACK_BITS/EXTRACT_BITS macros (or inline helpers)

### File: `engine/graphics/shader/material/material-data.slang` (continued)

**Add MaterialPayload and MaterialDataBlob**:
```slang
struct MaterialPayload {
    uint data[28]; // 112 bytes
};

struct MaterialDataBlob {
    MaterialHeader header;  // 16 bytes
    MaterialPayload payload; // 112 bytes
}; // Total: 128 bytes
```

### File: `engine/graphics/source/graphics/material/generated/material-data.generated.hpp`

**Changes**:
1. Update generated MaterialHeader to match Slang uint4 layout
2. Add bit-packing accessor methods for C++
3. Add MaterialPayload and MaterialDataBlob structs
4. Update static_asserts:
   - `sizeof(MaterialHeader) == 16`
   - `sizeof(MaterialDataBlob) == 128`

---

## Phase 2: Material Update System (P0)

### File: `engine/graphics/source/graphics/material/i-material.hpp`

**Add/Modify**:
```cpp
enum class MaterialUpdateFlags : uint32_t {
    None                = 0x0,
    CodeChanged         = 0x1,
    DataChanged         = 0x2,
    ResourcesChanged    = 0x4,
    DisplacementChanged = 0x8,
    EmissiveChanged     = 0x10,
};

class IMaterial : public core::Object {
public:
    // Add: Falcor-style update method
    virtual auto update(MaterialSystem* pOwner) -> MaterialUpdateFlags = 0;

    // Add: Data blob accessor (replaces writeData)
    virtual auto getDataBlob() const -> MaterialDataBlob = 0;

    // Add: Update callback registration
    using UpdateCallback = std::function<void(MaterialUpdateFlags)>;
    auto registerUpdateCallback(UpdateCallback const& callback) -> void;

protected:
    auto markUpdates(MaterialUpdateFlags flags) -> void;
    UpdateCallback m_updateCallback;
};
```

**Remove**:
- `writeData(StandardMaterialData&)` - replaced by `getDataBlob()`

### File: `engine/graphics/source/graphics/material/basic-material.hpp`

**Add**:
```cpp
class BasicMaterial : public IMaterial {
public:
    // Falcor-style update
    auto update(MaterialSystem* pOwner) -> MaterialUpdateFlags override;

    // Data blob accessor
    auto getDataBlob() const -> MaterialDataBlob override;

protected:
    // Helper to prepare typed blob
    template<typename T>
    auto prepareDataBlob(T const& data) const -> MaterialDataBlob;

    // Payload data (replaces direct members for GPU upload)
    BasicMaterialData m_data;
};
```

### File: `engine/graphics/source/graphics/material/standard-material.hpp`

**Add**:
```cpp
class StandardMaterial : public BasicMaterial {
public:
    auto update(MaterialSystem* pOwner) -> MaterialUpdateFlags override;
};
```

---

## Phase 3: MaterialSystem Alignment (P1)

### File: `engine/graphics/source/graphics/material/material-system.hpp`

**Rename defines** (match Falcor):
```cpp
// From:
defines["MATERIAL_TEXTURE_TABLE_SIZE"]
// To:
defines["MATERIAL_SYSTEM_TEXTURE_DESC_COUNT"]
defines["MATERIAL_SYSTEM_SAMPLER_DESC_COUNT"]
defines["MATERIAL_SYSTEM_BUFFER_DESC_COUNT"]
```

**Replace MaterialSystemConfig with Falcor-style constants**:
```cpp
// Remove:
struct MaterialSystemConfig { ... };

// Add:
static constexpr size_t kMaxSamplerCount = 1ull << 8; // 256
static constexpr size_t kMaxTextureCount = 1ull << 30;
```

**Replace MaterialSystemDiagnostics with MaterialStats**:
```cpp
// From:
struct MaterialSystemDiagnostics { ... };

// To:
struct MaterialStats {
    uint64_t materialTypeCount = 0;
    uint64_t materialCount = 0;
    uint64_t materialOpaqueCount = 0;
    uint64_t materialMemoryInBytes = 0;
    uint64_t textureCount = 0;
    uint64_t textureCompressedCount = 0;
    uint64_t textureTexelCount = 0;
    uint64_t textureTexelChannelCount = 0;
    uint64_t textureMemoryInBytes = 0;
};
```

**Add ParameterBlock binding**:
```cpp
class MaterialSystem {
public:
    // Add: Falcor-style update
    auto update(bool forceUpdate) -> MaterialUpdateFlags;

    // Add: Parameter block binding
    auto bindShaderData(ShaderVar const& var) const -> void;

private:
    core::ref<ParameterBlock> m_materialsBlock;
    auto createParameterBlock() -> void;
    auto uploadMaterial(uint32_t materialId) -> void;
};
```

### File: `engine/graphics/source/graphics/material/material-system.cpp`

**Modify addMaterial()**:
```cpp
auto MaterialSystem::addMaterial(core::ref<IMaterial> material) -> uint32_t {
    // Add: Check for existing identical material (not just pointer)
    for (size_t i = 0; i < m_materials.size(); ++i) {
        if (m_materials[i] == material) return static_cast<uint32_t>(i);
    }

    // Add: Set default sampler if null
    if (material->getDefaultTextureSampler() == nullptr) {
        material->setDefaultTextureSampler(m_defaultTextureSampler);
    }

    // Add: Register update callback
    material->registerUpdateCallback([this](auto flags) {
        m_materialUpdates |= flags;
    });

    // ... rest of implementation
}
```

**Add update() method** (replaces updateGpuBuffers):
```cpp
auto MaterialSystem::update(bool forceUpdate) -> MaterialUpdateFlags {
    MaterialUpdateFlags updateFlags = MaterialUpdateFlags::None;

    if (forceUpdate || m_materialsChanged) {
        updateMetadata();
        m_materialsBlock = nullptr;
        m_materialsChanged = false;
        forceUpdate = true;
    }

    // Update all materials
    for (size_t i = 0; i < m_materials.size(); ++i) {
        auto& material = m_materials[i];
        MaterialUpdateFlags flags = material->update(this);
        m_materialsUpdateFlags[i] = flags;
        updateFlags |= flags;
    }

    // Create parameter block if needed
    if (!m_materialsBlock) {
        createParameterBlock();
        updateFlags |= MaterialUpdateFlags::DataChanged | MaterialUpdateFlags::ResourcesChanged;
        forceUpdate = true;
    }

    // Upload modified materials
    if (forceUpdate || hasFlag(updateFlags, MaterialUpdateFlags::DataChanged)) {
        for (uint32_t i = 0; i < m_materials.size(); ++i) {
            if (forceUpdate || hasFlag(m_materialsUpdateFlags[i], MaterialUpdateFlags::DataChanged)) {
                uploadMaterial(i);
            }
        }
    }

    return updateFlags;
}
```

**Remove**: `validateAndClampDescriptorHandle()` - Falcor throws on overflow.

**Remove**: `rebuildCodeCache()` - Integrate into update() flow.

---

## Phase 4: TextureHandle Abstraction (P2)

### File: `engine/graphics/shader/material/texture-handle.slang` (NEW)

```slang
struct TextureHandle {
    uint packedData;

    static const uint kTextureIDBits = 30;
    static const uint kModeBits = 2;

    enum class Mode : uint {
        Texture = 0,
        UDIM = 1,
        Virtual = 2,
    };

    uint getTextureID() { return packedData & ((1u << kTextureIDBits) - 1); }
    Mode getMode() { return Mode(packedData >> kTextureIDBits); }
    bool isValid() { return packedData != 0; }
};
```

---

## Phase 5: Texture Slot System (P2)

### File: `engine/graphics/source/graphics/material/i-material.hpp`

**Add texture slot API**:
```cpp
class IMaterial {
public:
    enum class TextureSlot {
        BaseColor,
        Specular,
        Emissive,
        Normal,
        Transmission,
        Displacement,
        Count
    };

    struct TextureSlotInfo {
        std::string name;
        TextureChannelFlags mask = TextureChannelFlags::None;
        bool srgb = false;

        auto isEnabled() const -> bool { return mask != TextureChannelFlags::None; }
    };

    virtual auto getTextureSlotInfo(TextureSlot slot) const -> TextureSlotInfo const&;
    virtual auto hasTextureSlot(TextureSlot slot) const -> bool;
    virtual auto setTexture(TextureSlot slot, core::ref<Texture> texture) -> bool;
    virtual auto getTexture(TextureSlot slot) const -> core::ref<Texture>;
};
```

---

## File Change Summary

| File | Action | Lines Changed (est.) |
|------|--------|---------------------|
| material-data.slang | Modify | ~80 |
| material-data.generated.hpp | Regenerate | ~100 |
| i-material.hpp | Modify | ~60 |
| basic-material.hpp | Modify | ~40 |
| basic-material.cpp | Modify | ~80 |
| standard-material.hpp | Modify | ~20 |
| standard-material.cpp | Modify | ~60 |
| material-system.hpp | Modify | ~50 |
| material-system.cpp | Modify | ~150 |
| texture-handle.slang | New | ~30 |

**Total estimated changes**: ~670 lines

---

## Build Verification

After each phase:
```bash
cmake --build build/vs2022 --config Debug
```

Expected:
- No compilation errors
- No validation errors at startup
- Material rendering smoke test passes
