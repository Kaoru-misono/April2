# GRAPHICS-MATERIAL-628 Patches

## Overview

This document contains git-style unified diffs for restoring Falcor's material framework.
Changes are organized by phase priority.

---

## Patch 1: MaterialHeader Binary Layout (P0)

### 1.1 material-data.slang

**File**: `engine/graphics/shader/material/material-data.slang`

```diff
--- a/engine/graphics/shader/material/material-data.slang
+++ b/engine/graphics/shader/material/material-data.slang
@@ -1,21 +1,114 @@
 // Material data structures for GPU upload.
-// These structs are the source of truth and generate matching C++ headers.
+// Aligned with Falcor MaterialData.slang (single source of truth).

 import material.material_types;

 // @export-cpp
-// Material ABI version. Increment when binary layout changes.
-static const uint kMaterialAbiVersion = 1;
+// Bit packing helpers (Falcor-style)
+uint PACK_BITS(uint numBits, uint offset, uint value, uint field)
+{
+    uint mask = ((1u << numBits) - 1u) << offset;
+    return (value & ~mask) | ((field << offset) & mask);
+}
+
+uint EXTRACT_BITS(uint numBits, uint offset, uint value)
+{
+    return (value >> offset) & ((1u << numBits) - 1u);
+}

 // @export-cpp
+// MaterialHeader: 16 bytes (uint4), bit-packed per Falcor spec.
 struct MaterialHeader
 {
-    uint abiVersion;        // Material ABI version (kMaterialAbiVersion)
-    uint materialType;      // MaterialType enum
-    uint flags;             // MaterialFlags bitmask
-    uint alphaMode;         // AlphaMode enum
-    uint activeLobes;       // LobeType bitmask
-    uint reserved0;         // Reserved for forward-compatible header growth
-    uint reserved1;         // Reserved for forward-compatible header growth
-    uint reserved2;         // Reserved for forward-compatible header growth
+    uint4 packedData;
+
+    // Bit layout constants (match Falcor MaterialData.slang)
+    static const uint kMaterialTypeBits = 16;
+    static const uint kNestedPriorityBits = 4;
+    static const uint kLobeTypeBits = 8;
+    static const uint kSamplerIDBits = 8;
+    static const uint kAlphaModeBits = 1;
+    static const uint kAlphaThresholdBits = 16;
+    static const uint kIoRBits = 16;
+
+    // packedData.x offsets
+    static const uint kMaterialTypeOffset = 0;
+    static const uint kNestedPriorityOffset = 16;
+    static const uint kLobeTypeOffset = 20;
+    static const uint kDoubleSidedFlagOffset = 28;
+    static const uint kThinSurfaceFlagOffset = 29;
+    static const uint kEmissiveFlagOffset = 30;
+    static const uint kIsBasicMaterialFlagOffset = 31;
+
+    // packedData.y offsets
+    static const uint kAlphaThresholdOffset = 0;
+    static const uint kAlphaModeOffset = 16;
+    static const uint kSamplerIDOffset = 17;
+
+    // packedData.z offsets
+    static const uint kIoROffset = 0;
+
+    // Accessors
+    [mutating] void setMaterialType(uint type) {
+        packedData.x = PACK_BITS(kMaterialTypeBits, kMaterialTypeOffset, packedData.x, type);
+    }
+    uint getMaterialType() { return EXTRACT_BITS(kMaterialTypeBits, kMaterialTypeOffset, packedData.x); }
+
+    [mutating] void setAlphaMode(uint mode) {
+        packedData.y = PACK_BITS(kAlphaModeBits, kAlphaModeOffset, packedData.y, mode);
+    }
+    uint getAlphaMode() { return EXTRACT_BITS(kAlphaModeBits, kAlphaModeOffset, packedData.y); }
+
+    [mutating] void setActiveLobes(uint lobes) {
+        packedData.x = PACK_BITS(kLobeTypeBits, kLobeTypeOffset, packedData.x, lobes);
+    }
+    uint getActiveLobes() { return EXTRACT_BITS(kLobeTypeBits, kLobeTypeOffset, packedData.x); }
+
+    [mutating] void setDoubleSided(bool v) {
+        packedData.x = PACK_BITS(1, kDoubleSidedFlagOffset, packedData.x, v ? 1 : 0);
+    }
+    bool isDoubleSided() { return (packedData.x & (1u << kDoubleSidedFlagOffset)) != 0; }
+
+    [mutating] void setEmissive(bool v) {
+        packedData.x = PACK_BITS(1, kEmissiveFlagOffset, packedData.x, v ? 1 : 0);
+    }
+    bool isEmissive() { return (packedData.x & (1u << kEmissiveFlagOffset)) != 0; }
+
+    [mutating] void setDefaultTextureSamplerID(uint id) {
+        packedData.y = PACK_BITS(kSamplerIDBits, kSamplerIDOffset, packedData.y, id);
+    }
+    uint getDefaultTextureSamplerID() { return EXTRACT_BITS(kSamplerIDBits, kSamplerIDOffset, packedData.y); }
 };
+
+// @export-cpp
+// MaterialPayload: 112 bytes, type-dependent data.
+struct MaterialPayload
+{
+    uint data[28];
+};
+
+// @export-cpp
+// MaterialDataBlob: 128 bytes total (one cache line), header + payload.
+struct MaterialDataBlob
+{
+    MaterialHeader header;  // 16 bytes
+    MaterialPayload payload; // 112 bytes
+};

 // @export-cpp
+// StandardMaterialData: Backward-compatible struct for StandardMaterial.
+// Fits within MaterialDataBlob.payload (112 bytes max after header).
 struct StandardMaterialData
 {
-    MaterialHeader header;
-
     float4 baseColor;
     float metallic;
     float roughness;
@@ -35,6 +128,7 @@ struct StandardMaterialData
     uint normalTextureHandle;
     uint occlusionTextureHandle;
     uint emissiveTextureHandle;
+    uint transmissionTextureHandle;
     uint samplerHandle;
-    uint bufferHandle;
-    uint reservedDescriptorHandle;
+    uint2 _reserved;
 };
```

**Rationale**: This change aligns MaterialHeader with Falcor's 16-byte bit-packed format and introduces the MaterialDataBlob abstraction.

---

## Patch 2: Update Flags Alignment (P0)

### 2.1 i-material.hpp

**File**: `engine/graphics/source/graphics/material/i-material.hpp`

```diff
--- a/engine/graphics/source/graphics/material/i-material.hpp
+++ b/engine/graphics/source/graphics/material/i-material.hpp
@@ -16,11 +16,16 @@ namespace april::graphics
 class ShaderVariable;
 class Texture;
+class MaterialSystem;

 /**
- * Material update flags for selective GPU upload.
+ * Material update flags (Falcor Material::UpdateFlags).
  */
 enum class MaterialUpdateFlags : uint32_t
 {
-    None = 0,
-    DataChanged = 1 << 0,       // Material parameters changed
-    TexturesChanged = 1 << 1,   // Texture bindings changed
-    All = DataChanged | TexturesChanged
+    None                = 0x0,
+    CodeChanged         = 0x1,  // Material shader code changed
+    DataChanged         = 0x2,  // Material data (parameters) changed
+    ResourcesChanged    = 0x4,  // Material resources (textures, buffers, samplers) changed
+    DisplacementChanged = 0x8,  // Displacement mapping parameters changed
+    EmissiveChanged     = 0x10, // Material emissive properties changed
 };

 inline auto operator|(MaterialUpdateFlags a, MaterialUpdateFlags b) -> MaterialUpdateFlags
@@ -47,20 +52,33 @@ class IMaterial : public core::Object
 public:
     virtual ~IMaterial() = default;

+    // ---- Falcor Material interface ----
+
     /**
-     * Get the material type.
+     * Update material. Prepares material for rendering.
+     * @param pOwner The material system this material is used with.
+     * @return Updates since last call to update().
      */
-    virtual auto getType() const -> generated::MaterialType = 0;
+    virtual auto update(MaterialSystem* pOwner) -> MaterialUpdateFlags = 0;

     /**
-     * Get the material type name as a string.
+     * Get the material data blob for uploading to the GPU.
      */
-    virtual auto getTypeName() const -> std::string = 0;
+    virtual auto getDataBlob() const -> generated::MaterialDataBlob = 0;

     /**
-     * Write material data to a GPU-compatible struct.
-     * @param data Output struct to populate.
+     * Get the material type.
      */
-    virtual auto writeData(generated::StandardMaterialData& data) const -> void = 0;
+    virtual auto getType() const -> generated::MaterialType = 0;
+
+    /**
+     * Get the material type name as a string.
+     */
+    virtual auto getTypeName() const -> std::string = 0;

     /**
      * Get type conformances for shader compilation.
@@ -93,6 +111,19 @@ public:
      */
     virtual auto isDoubleSided() const -> bool = 0;

+    /**
+     * Get maximum texture count for this material.
+     */
+    virtual auto getMaxTextureCount() const -> size_t { return 7; }
+
+    /**
+     * Get maximum buffer count for this material.
+     */
+    virtual auto getMaxBufferCount() const -> size_t { return 0; }
+
+    // ---- Update callback (Falcor pattern) ----
+    using UpdateCallback = std::function<void(MaterialUpdateFlags)>;
+    auto registerUpdateCallback(UpdateCallback const& callback) -> void { m_updateCallback = callback; }
+
     // ---- Lifecycle and update tracking ----

     /**
@@ -105,10 +136,7 @@ public:
      */
     auto getUpdateFlags() const -> MaterialUpdateFlags { return m_updateFlags; }

-    /**
-     * Mark the material as needing an update.
-     */
-    auto markDirty(MaterialUpdateFlags flags = MaterialUpdateFlags::All) -> void { m_updateFlags |= flags; }
+protected:
+    auto markUpdates(MaterialUpdateFlags flags) -> void;

-    /**
-     * Clear update flags after GPU upload.
-     */
-    auto clearDirty() -> void { m_updateFlags = MaterialUpdateFlags::None; }
-
-    // ---- Serialization for tooling ----
-
-    /**
-     * Serialize material parameters to JSON.
-     */
-    virtual auto serializeParameters(nlohmann::json& outJson) const -> void = 0;
-
-    /**
-     * Deserialize material parameters from JSON.
-     * @return true if successful.
-     */
-    virtual auto deserializeParameters(nlohmann::json const& inJson) -> bool = 0;
-
-protected:
     MaterialUpdateFlags m_updateFlags{MaterialUpdateFlags::All};
+    UpdateCallback m_updateCallback;
 };

 } // namespace april::graphics
```

**Rationale**: Aligns the update flags with Falcor's enumeration and adds the `update()` method and callback pattern.

---

## Patch 3: MaterialSystem Shader Defines (P1)

### 3.1 material-system.cpp

**File**: `engine/graphics/source/graphics/material/material-system.cpp`

```diff
--- a/engine/graphics/source/graphics/material/material-system.cpp
+++ b/engine/graphics/source/graphics/material/material-system.cpp
@@ -40,12 +40,24 @@ MaterialSystem::MaterialSystem(core::ref<Device> device, MaterialSystemConfig co

 auto MaterialSystem::getShaderDefines() const -> DefineList
 {
     auto defines = DefineList{};
-    defines["MATERIAL_TEXTURE_TABLE_SIZE"] = std::to_string(m_config.textureTableSize);
-    defines["MATERIAL_SAMPLER_TABLE_SIZE"] = std::to_string(m_config.samplerTableSize);
-    defines["MATERIAL_BUFFER_TABLE_SIZE"] = std::to_string(m_config.bufferTableSize);
-    // Match IMaterialInstance anyValueSize constraint for dynamic object storage.
-    defines["FALCOR_MATERIAL_INSTANCE_SIZE"] = "256";
+    // Falcor-style define names
+    defines["MATERIAL_SYSTEM_TEXTURE_DESC_COUNT"] = std::to_string(m_textureDescCount);
+    defines["MATERIAL_SYSTEM_SAMPLER_DESC_COUNT"] = std::to_string(kMaxSamplerCount);
+    defines["MATERIAL_SYSTEM_BUFFER_DESC_COUNT"] = std::to_string(m_bufferDescCount);
+
+    // Compute max material instance size from all materials
+    size_t materialInstanceByteSize = 128;
+    for (auto const& material : m_materials) {
+        if (material) {
+            materialInstanceByteSize = std::max(materialInstanceByteSize, material->getMaterialInstanceByteSize());
+        }
+    }
+    defines["FALCOR_MATERIAL_INSTANCE_SIZE"] = std::to_string(materialInstanceByteSize);
+
     return defines;
 }
```

**Rationale**: Renames shader defines to match Falcor naming convention.

---

## Patch 4: MaterialSystem::update() Method (P0)

### 4.1 material-system.hpp

**File**: `engine/graphics/source/graphics/material/material-system.hpp`

```diff
--- a/engine/graphics/source/graphics/material/material-system.hpp
+++ b/engine/graphics/source/graphics/material/material-system.hpp
@@ -112,10 +112,15 @@ public:
     auto getMaterialCount() const -> uint32_t;

     /**
-     * Update GPU buffers with current material data.
-     * Call this after modifying material properties.
+     * Update material system. Prepares all resources for rendering.
+     * @param forceUpdate Force full update of all materials.
+     * @return Combined update flags across all materials.
      */
-    auto updateGpuBuffers() -> void;
+    auto update(bool forceUpdate = false) -> MaterialUpdateFlags;
+
+    /**
+     * Bind material system to a shader var (Falcor pattern).
+     */
+    auto bindShaderData(ShaderVariable const& var) const -> void;

     /**
-     * Bind material data buffer to a shader variable.
-     * @param var Shader variable for the material data buffer.
+     * Get statistics about material system.
      */
-    auto bindToShader(ShaderVariable& var) const -> void;
+    auto getStats() const -> MaterialStats;

 private:
+    // Falcor-style metadata update
+    auto updateMetadata() -> void;
+    auto uploadMaterial(uint32_t materialId) -> void;
+
+    std::vector<MaterialUpdateFlags> m_materialsUpdateFlags;
+    MaterialUpdateFlags m_materialUpdates{MaterialUpdateFlags::None};
+    bool m_materialsChanged{true};
```

### 4.2 material-system.cpp (update method)

**File**: `engine/graphics/source/graphics/material/material-system.cpp`

```diff
--- a/engine/graphics/source/graphics/material/material-system.cpp
+++ b/engine/graphics/source/graphics/material/material-system.cpp
@@ -121,6 +121,15 @@ auto MaterialSystem::addMaterial(core::ref<IMaterial> material) -> uint32_t
         return it->second;
     }

+    // Set default sampler if null (Falcor pattern)
+    if (material->getDefaultTextureSampler() == nullptr) {
+        material->setDefaultTextureSampler(m_defaultTextureSampler);
+    }
+
+    // Register update callback (Falcor pattern)
+    material->registerUpdateCallback([this](auto flags) {
+        m_materialUpdates |= flags;
+    });
+
     auto const index = static_cast<uint32_t>(m_materials.size());
     m_materials.push_back(material);
     m_materialIndices[material.get()] = index;
@@ -130,49 +139,68 @@ auto MaterialSystem::addMaterial(core::ref<IMaterial> material) -> uint32_t

     markDirty();
     return index;
 }

-auto MaterialSystem::updateGpuBuffers() -> void
+auto MaterialSystem::update(bool forceUpdate) -> MaterialUpdateFlags
 {
-    // Check if any materials have pending updates.
-    auto anyDirty = m_dirty;
-    if (!anyDirty)
-    {
-        for (auto const& material : m_materials)
-        {
-            if (material && material->isDirty())
-            {
-                anyDirty = true;
-                break;
-            }
-        }
+    // If materials were added/removed, update metadata and recreate parameter block
+    if (forceUpdate || m_materialsChanged) {
+        updateMetadata();
+        m_materialsChanged = false;
+        forceUpdate = true;
     }

-    if (!anyDirty)
-        return;
-
-    if (m_dirty)
-    {
-        rebuildMaterialData();
-
-        if (m_cpuMaterialData.empty())
-        {
-            m_dirty = false;
-            return;
-        }
-
-        ensureBufferCapacity(static_cast<uint32_t>(m_cpuMaterialData.size()));
+    // Update all materials and track per-material flags
+    MaterialUpdateFlags updateFlags = MaterialUpdateFlags::None;
+    m_materialsUpdateFlags.resize(m_materials.size());
+    std::fill(m_materialsUpdateFlags.begin(), m_materialsUpdateFlags.end(), MaterialUpdateFlags::None);

-        if (m_materialDataBuffer)
-        {
-            auto const dataSize = m_cpuMaterialData.size() * sizeof(generated::StandardMaterialData);
-            m_materialDataBuffer->setBlob(m_cpuMaterialData.data(), 0, dataSize);
-        }
-
-        for (auto const& material : m_materials)
-        {
-            if (material)
-            {
-                material->clearDirty();
-            }
+    for (size_t i = 0; i < m_materials.size(); ++i) {
+        if (auto& material = m_materials[i]) {
+            auto flags = material->update(this);
+            m_materialsUpdateFlags[i] = flags;
+            updateFlags |= flags;
         }
-
-        m_dirty = false;
-        return;
     }

-    for (uint32_t i = 0; i < m_materials.size(); ++i)
-    {
-        auto const& material = m_materials[i];
-        if (!material || !material->isDirty())
-        {
-            continue;
-        }
+    // Include updates recorded since last update
+    updateFlags |= m_materialUpdates;
+    m_materialUpdates = MaterialUpdateFlags::None;

-        writeMaterialData(i);
+    // Ensure buffer capacity
+    if (!m_materials.empty()) {
+        ensureBufferCapacity(static_cast<uint32_t>(m_materials.size()));
+    }

-        if (m_materialDataBuffer)
-        {
-            auto const offset = static_cast<size_t>(i) * sizeof(generated::StandardMaterialData);
-            m_materialDataBuffer->setBlob(&m_cpuMaterialData[i], offset, sizeof(generated::StandardMaterialData));
+    // Upload modified materials
+    if (forceUpdate || hasFlag(updateFlags, MaterialUpdateFlags::DataChanged)) {
+        for (uint32_t i = 0; i < m_materials.size(); ++i) {
+            if (forceUpdate || hasFlag(m_materialsUpdateFlags[i], MaterialUpdateFlags::DataChanged)) {
+                uploadMaterial(i);
+            }
         }
-
-        material->clearDirty();
     }
+
+    return updateFlags;
 }
```

**Rationale**: Replaces `updateGpuBuffers()` with Falcor-style `update()` method that calls `material->update()` for each material.

---

## Patch 5: Remove Engine Adaptation Code

### 5.1 Remove validateAndClampDescriptorHandle

**File**: `engine/graphics/source/graphics/material/material-system.cpp`

```diff
--- a/engine/graphics/source/graphics/material/material-system.cpp
+++ b/engine/graphics/source/graphics/material/material-system.cpp
@@ -92,29 +92,6 @@ auto MaterialSystem::getDiagnostics() const -> MaterialSystemDiagnostics
     return diag;
 }

-auto MaterialSystem::validateAndClampDescriptorHandle(
-    uint32_t& handle,
-    uint32_t maxCount,
-    uint32_t& overflowCounter,
-    uint32_t materialIndex,
-    char const* slotName
-) const -> void
-{
-    if (handle < maxCount)
-    {
-        return;
-    }
-
-    // Debug fail-fast: assert in debug builds to catch issues early.
-    AP_ASSERT(false, "Descriptor overflow: handle {} >= capacity {} for slot '{}'", handle, maxCount, slotName);
-
-    // Runtime fallback: clamp to fallback slot 0 and log warning.
-    AP_WARN(
-        "MaterialSystem: descriptor overflow for material {} slot '{}' (handle={}, capacity={}), using fallback 0",
-        materialIndex,
-        slotName,
-        handle,
-        maxCount
-    );
-
-    handle = kInvalidDescriptorHandle;
-    ++overflowCounter;
-    ++m_invalidHandleCount;
-}
```

**Rationale**: Falcor throws on overflow rather than clamping. Remove the clamping logic.

---

## Summary

| Patch | Description | Files Changed | Priority |
|-------|-------------|---------------|----------|
| 1 | MaterialHeader 16-byte layout | material-data.slang | P0 |
| 2 | Update flags alignment | i-material.hpp | P0 |
| 3 | Shader define names | material-system.cpp | P1 |
| 4 | MaterialSystem::update() | material-system.hpp/cpp | P0 |
| 5 | Remove clamping adaptation | material-system.cpp | P1 |

---

## Build Verification

After applying all patches:
```bash
cmake --build build/vs2022 --config Debug
```

Expected results:
1. Clean compilation (no errors)
2. No validation errors at runtime
3. Material rendering works correctly
