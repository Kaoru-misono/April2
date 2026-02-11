# GRAPHICS-MATERIAL-628 Deviation Report

## Executive Summary

This report documents deviations between **Falcor's material framework** (reference) and the **April engine's current implementation**. The goal is to restore Falcor's architecture/semantics/dataflow/responsibility boundaries while only allowing:
1. C++ code generation from Slang
2. Shader/C++ sharing the same Slang sources via macros

---

## 1. Material Data Layout Deviations

### 1.1 MaterialHeader Structure

| Aspect | Falcor | Current | Deviation Type |
|--------|--------|---------|----------------|
| Size | 16 bytes (uint4 packedData) | 32 bytes (8 uint fields) | **Binary Layout** |
| Packing | Bit-packed via PACK_BITS/EXTRACT_BITS macros | Simple uint fields | **Implementation** |
| Fields | materialType, nestedPriority, lobeType, doubleSided, thinSurface, emissive, isBasicMaterial, alphaThreshold, alphaMode, samplerID, lightProfile, deltaSpecular, IoR, alphaTextureHandle | abiVersion, materialType, flags, alphaMode, activeLobes, reserved0/1/2 | **Field Layout** |

**Falcor Evidence** (`falcor-material-reference/Falcor/Scene/Material/MaterialData.slang:43-198`):
```cpp
struct MaterialHeader {
    uint4 packedData = {};
    // Bit-packed fields using PACK_BITS/EXTRACT_BITS
    static constexpr uint kMaterialTypeBits = 16;
    // ... extensive bit layout definitions
};
```

**Current Implementation** (`engine/graphics/shader/material/material-data.slang:11-21`):
```cpp
struct MaterialHeader {
    uint abiVersion;
    uint materialType;
    uint flags;
    uint alphaMode;
    uint activeLobes;
    uint reserved0/1/2;
};
```

**Fix Strategy**: Revert MaterialHeader to Falcor's 16-byte bit-packed format. This requires:
- Updating Slang struct to match Falcor's uint4 layout
- Implementing PACK_BITS/EXTRACT_BITS accessor methods
- Updating C++ generated code to match

### 1.2 Material Data Blob Size

| Aspect | Falcor | Current | Deviation Type |
|--------|--------|---------|----------------|
| Total Size | 128 bytes (header 16B + payload 112B) | 144 bytes | **Binary Layout** |
| Structure | MaterialDataBlob = MaterialHeader + MaterialPayload | StandardMaterialData (single struct) | **Architecture** |

**Falcor Evidence** (`MaterialData.slang:200-218`):
```cpp
struct MaterialPayload { uint data[28]; }; // 112B
struct MaterialDataBlob {
    MaterialHeader header; // 16B
    MaterialPayload payload; // 112B
}; // Total: 128B, fits in one cacheline
```

**Fix Strategy**: Restore 128B blob format with separate header + payload.

---

## 2. Material Update System Deviations

### 2.1 UpdateFlags Enumeration

| Aspect | Falcor | Current | Deviation Type |
|--------|--------|---------|----------------|
| Flags | None, CodeChanged, DataChanged, ResourcesChanged, DisplacementChanged, EmissiveChanged | None, DataChanged, TexturesChanged, All | **API Contract** |
| Granularity | 5 distinct update categories | 3 categories | **Missing Feature** |

**Falcor Evidence** (`Material.h:63-71`):
```cpp
enum class UpdateFlags : uint32_t {
    None                = 0x0,
    CodeChanged         = 0x1,
    DataChanged         = 0x2,
    ResourcesChanged    = 0x4,
    DisplacementChanged = 0x8,
    EmissiveChanged     = 0x10,
};
```

**Fix Strategy**: Align MaterialUpdateFlags with Falcor's UpdateFlags enum.

### 2.2 Material.update() Method

| Aspect | Falcor | Current | Deviation Type |
|--------|--------|---------|----------------|
| Signature | `virtual UpdateFlags update(MaterialSystem* pOwner) = 0` | Not present | **Missing API** |
| Flow | Material pushes updates to owner | System pulls via writeData() | **Dataflow** |

**Falcor Evidence** (`Material.h:128-129`):
```cpp
virtual Material::UpdateFlags update(MaterialSystem* pOwner) = 0;
```

**Fix Strategy**: Add `update(MaterialSystem*)` method to IMaterial interface; materials update their own state and return what changed.

### 2.3 Update Callback Registration

| Aspect | Falcor | Current | Deviation Type |
|--------|--------|---------|----------------|
| Mechanism | `registerUpdateCallback()` on Material | Not present | **Missing API** |
| Purpose | Materials notify system of changes | System polls isDirty() | **Dataflow** |

**Falcor Evidence** (`Material.h:356`):
```cpp
void registerUpdateCallback(const UpdateCallback& updateCallback) { mUpdateCallback = updateCallback; }
```

**Fix Strategy**: Add callback registration; materials invoke callback when state changes.

---

## 3. Texture Management Deviations

### 3.1 TextureManager Integration

| Aspect | Falcor | Current | Deviation Type |
|--------|--------|---------|----------------|
| Ownership | MaterialSystem owns TextureManager | No centralized TextureManager | **Architecture** |
| Access | `mpTextureManager->bindShaderData()` | Direct descriptor registration | **API** |

**Falcor Evidence** (`MaterialSystem.h:253`, `MaterialSystem.cpp:71`):
```cpp
std::unique_ptr<TextureManager> mpTextureManager;
mpTextureManager = std::make_unique<TextureManager>(mpDevice, kMaxTextureCount);
```

**Fix Strategy**: Introduce TextureManager class or equivalent abstraction.

### 3.2 TextureHandle Abstraction

| Aspect | Falcor | Current | Deviation Type |
|--------|--------|---------|----------------|
| Type | `TextureHandle` struct with packedData | Raw uint32_t handles | **Type Safety** |
| Features | Mode flags (Texture, UDIM, Virtual) | Simple index | **Missing Feature** |

**Falcor Evidence** (`TextureHandle.slang`):
```cpp
struct TextureHandle {
    uint packedData;
    static constexpr uint kTextureIDBits = 30;
    // Mode: Texture, UDIM, Virtual
};
```

**Fix Strategy**: Introduce TextureHandle struct for type safety and mode support.

### 3.3 Texture Slot System

| Aspect | Falcor | Current | Deviation Type |
|--------|--------|---------|----------------|
| API | `setTexture(TextureSlot, Texture)` / `getTexture(TextureSlot)` | Direct member access | **API** |
| Slot Info | TextureSlotInfo, TextureSlotData arrays | No slot metadata | **Missing Feature** |

**Falcor Evidence** (`Material.h:89-108, 223-258`):
```cpp
struct TextureSlotInfo { std::string name; TextureChannelFlags mask; bool srgb; };
struct TextureSlotData { ref<Texture> pTexture; };
std::array<TextureSlotInfo, (size_t)TextureSlot::Count> mTextureSlotInfo;
std::array<TextureSlotData, (size_t)TextureSlot::Count> mTextureSlotData;
```

**Fix Strategy**: Add formal texture slot API with slot info metadata.

---

## 4. MaterialSystem Deviations

### 4.1 ParameterBlock Binding

| Aspect | Falcor | Current | Deviation Type |
|--------|--------|---------|----------------|
| Mechanism | `ref<ParameterBlock> mpMaterialsBlock` | Direct buffer binding | **Architecture** |
| Creation | `createParameterBlock()` via shader reflection | Manual buffer management | **Implementation** |

**Falcor Evidence** (`MaterialSystem.cpp:759-799`):
```cpp
void MaterialSystem::createParameterBlock() {
    auto pPass = ComputePass::create(mpDevice, kShaderFilename, "main", defines);
    auto pReflector = pPass->getProgram()->getReflector()->getParameterBlock("gMaterialsBlock");
    mpMaterialsBlock = ParameterBlock::create(mpDevice, pReflector);
    // Verify struct size matches
}
```

**Fix Strategy**: Restore ParameterBlock-based binding with shader reflection validation.

### 4.2 Shader Define Names

| Aspect | Falcor | Current | Deviation Type |
|--------|--------|---------|----------------|
| Texture count | `MATERIAL_SYSTEM_TEXTURE_DESC_COUNT` | `MATERIAL_TEXTURE_TABLE_SIZE` | **Naming** |
| Sampler count | `MATERIAL_SYSTEM_SAMPLER_DESC_COUNT` | `MATERIAL_SAMPLER_TABLE_SIZE` | **Naming** |
| Buffer count | `MATERIAL_SYSTEM_BUFFER_DESC_COUNT` | `MATERIAL_BUFFER_TABLE_SIZE` | **Naming** |

**Falcor Evidence** (`MaterialSystem.cpp:691-694`):
```cpp
defines.add("MATERIAL_SYSTEM_SAMPLER_DESC_COUNT", std::to_string(kMaxSamplerCount));
defines.add("MATERIAL_SYSTEM_TEXTURE_DESC_COUNT", std::to_string(mTextureDescCount));
defines.add("MATERIAL_SYSTEM_BUFFER_DESC_COUNT", std::to_string(mBufferDescCount));
```

**Fix Strategy**: Rename defines to match Falcor naming convention.

### 4.3 Material Addition Flow

| Aspect | Falcor | Current | Deviation Type |
|--------|--------|---------|----------------|
| Default sampler | Sets if null on add | Not set | **Missing Behavior** |
| Callback | Registers update callback on add | Not registered | **Missing Behavior** |
| Dedup | Checks if identical material exists | Checks pointer only | **Behavior** |

**Falcor Evidence** (`MaterialSystem.cpp:224-251`):
```cpp
MaterialID MaterialSystem::addMaterial(const ref<Material>& pMaterial) {
    // Reuse previously added materials
    if (auto it = std::find(mMaterials.begin(), mMaterials.end(), pMaterial); it != mMaterials.end())
        return MaterialID{...};
    if (pMaterial->getDefaultTextureSampler() == nullptr)
        pMaterial->setDefaultTextureSampler(mpDefaultTextureSampler);
    pMaterial->registerUpdateCallback([this](auto flags) { mMaterialUpdates |= flags; });
    // ...
}
```

**Fix Strategy**: Add default sampler assignment and callback registration in addMaterial().

---

## 5. Material Base Class Deviations

### 5.1 getDataBlob() vs writeData()

| Aspect | Falcor | Current | Deviation Type |
|--------|--------|---------|----------------|
| Method | `virtual MaterialDataBlob getDataBlob() const = 0` | `virtual void writeData(StandardMaterialData&) = 0` | **API Signature** |
| Return | Returns complete blob | Writes to provided struct | **Dataflow** |

**Falcor Evidence** (`Material.h:298`):
```cpp
virtual MaterialDataBlob getDataBlob() const = 0;
```

**Fix Strategy**: Change API to return MaterialDataBlob; implement prepareDataBlob<T>() helper.

### 5.2 Missing Material Properties

| Property | Falcor | Current | Deviation Type |
|----------|--------|---------|----------------|
| nestedPriority | Yes | No | **Missing Property** |
| thinSurface | In header flags | Via MaterialFlags enum | **Different Location** |
| deltaSpecular | Yes | No | **Missing Property** |
| alphaTextureHandle | In header | Separate field | **Different Location** |

**Fix Strategy**: Move properties to MaterialHeader bit-packed format.

---

## 6. Typed Payload Model Deviation

### 6.1 Single Struct vs Typed Payloads

| Aspect | Falcor | Current | Deviation Type |
|--------|--------|---------|----------------|
| Data model | MaterialDataBlob with typed payload | All types use StandardMaterialData | **Architecture** |
| Type safety | Each material type has own payload struct | UnlitMaterial writes to StandardMaterialData | **Design Defect** |

**Falcor Evidence** (Multiple files: BasicMaterialData.slang, StandardMaterial, etc.):
```cpp
// Each material type defines its own payload
struct BasicMaterialData { ... }; // Used by BasicMaterial
// getDataBlob() returns typed blob via prepareDataBlob<T>()
```

**Fix Strategy**:
- Define per-material-type payload structs
- Keep MaterialDataBlob as header + generic payload
- Each material type populates its payload section

---

## 7. Engine Adaptation Code to Remove

### 7.1 MaterialSystemConfig

**Location**: `material-system.hpp:26-31`
```cpp
struct MaterialSystemConfig {
    uint32_t textureTableSize = 128;
    uint32_t samplerTableSize = 8;
    uint32_t bufferTableSize = 16;
};
```

**Rationale**: Falcor derives these from constants and material metadata, not from config struct. Should compute from `kMaxSamplerCount`, `kMaxTextureCount`, and per-material `getMaxTextureCount()`.

### 7.2 MaterialSystemDiagnostics

**Location**: `material-system.hpp:36-54`

**Rationale**: Falcor uses `MaterialStats` struct returned by `getStats()`. Current diagnostics struct has different fields and purpose. Align with Falcor's `MaterialStats`.

### 7.3 Descriptor Handle Validation

**Location**: `material-system.cpp:92-120`
```cpp
auto MaterialSystem::validateAndClampDescriptorHandle(...) const -> void
```

**Rationale**: Falcor throws on overflow rather than clamping. Should fail-fast per Falcor behavior.

### 7.4 Code Cache Mechanism

**Location**: `material-system.cpp:415-445`
```cpp
auto MaterialSystem::rebuildCodeCache() const -> void
```

**Rationale**: Falcor rebuilds shader modules/conformances in `update()` when `CodeChanged` flag is set, not via separate cache mechanism.

---

## 8. Summary: Deviation Categories

| Category | Count | Priority |
|----------|-------|----------|
| Binary Layout | 2 | P0 - Must fix |
| Missing API | 4 | P0 - Must fix |
| Architecture | 3 | P1 - Should fix |
| Dataflow | 3 | P1 - Should fix |
| Missing Feature | 3 | P2 - Can defer |
| Naming | 3 | P3 - Low priority |

---

## Next Steps

1. **Phase 1**: Fix binary layout (MaterialHeader 16B, MaterialDataBlob 128B)
2. **Phase 2**: Add Material.update() and callback system
3. **Phase 3**: Restore ParameterBlock binding
4. **Phase 4**: Add TextureManager integration
5. **Phase 5**: Implement typed payload model

---

## 9. 2026-02-11 Deep-Dive Delta Audit (April vs Falcor master)

This addendum captures a fresh, line-level comparison against:

- April repository state on 2026-02-11.
- Falcor `master` sources fetched from GitHub:
  - `Source/Falcor/Scene/Material/Material.h`
  - `Source/Falcor/Scene/Material/MaterialSystem.h/.cpp/.slang`
  - `Source/Falcor/Scene/Material/MaterialData.slang`
  - `Source/Falcor/Scene/Material/TextureHandle.slang`
  - `Source/Falcor/Scene/Material/MaterialTypeRegistry.h/.cpp`
  - `Source/Falcor/Rendering/Materials/IMaterial.slang`
  - `Source/Falcor/Rendering/Materials/IMaterialInstance.slang`
  - `Source/Falcor/Rendering/Materials/MaterialInstanceHints.slang`
  - `Source/Falcor/Rendering/Materials/StandardMaterial.slang`
  - `Source/Falcor/Rendering/Materials/StandardMaterialInstance.slang`

### 9.1 Already Aligned (Core Contract)

| Area | Falcor | April | Status |
|------|--------|-------|--------|
| Material blob ABI | `MaterialHeader` 16B bit-packed + `MaterialPayload` 112B | Same layout and bit-pack accessors in Slang-generated ABI | Aligned |
| Dynamic shader dispatch | `createDynamicObject<IMaterial, MaterialDataBlob>` | Same in material factory extension | Aligned |
| Update flags + callback | `UpdateFlags` + `registerUpdateCallback()` + `update(MaterialSystem*)` | Same host-side callback/update flow | Aligned |
| Texture handle mode bitfield | `TextureHandle` with mode + UDIM flag bits | Same packed mode/ID/UDIM contract | Aligned |

### 9.2 April Has, Falcor Does Not (or Not Built-In)

| April-only capability | Evidence in April | Falcor comparison |
|-----------------------|------------------|-------------------|
| Built-in `Unlit` material type in host+shader stack | `material-types.slang`, `unlit-material.hpp/.cpp`, `unlit-material.slang` | Falcor built-in material enum set does not include `Unlit` |
| Asset-level explicit `materialType` metadata used at runtime | `asset/material-asset.hpp` + registry load path in `render-resource-registry.cpp` | Falcor generally constructs typed material objects directly in scene/material pipelines |
| JSON serialization on `IMaterial` interface (`serializeParameters`/`deserializeParameters`) | `i-material.hpp` | Falcor uses `MaterialParamLayout` + `SerializedMaterialParams` contract for supported types |
| Extra material hint `DisableOpacity` | `material-instance-hints.slang` | Falcor hint set is `DisableNormalMapping` + `AdjustShadingNormal` |
| `AlphaMode::Blend` enum value in shared material types | `material-types.slang` | Falcor material alpha mode in the core material header path is opaque/mask |

### 9.3 Falcor Has, April Still Missing

#### 9.3.1 System ownership and resource model

1. **TextureManager-owned texture lifecycle**
   - Falcor: material textures are centralized behind `TextureManager` with deferred loading and binding.
   - April: descriptor registration vectors/maps are embedded in `MaterialSystem` directly.

2. **ParameterBlock reflection binding path**
   - Falcor: `createParameterBlock()` reflects `gMaterialsBlock`, validates struct byte size, and binds one block object.
   - April: binds `StructuredBuffer` and descriptor arrays manually from scene shader variables.

3. **Complete managed resource channels (buffer + 3D texture replacement path)**
   - Falcor: `addBuffer()/replaceBuffer()/addTexture3D()` and associated update flags.
   - April: descriptor registration APIs exist for buffers/textures/samplers, but no equivalent material-system-level replacement/update management path matching Falcor behavior.

#### 9.3.2 Update/lifecycle behavior

4. **Dynamic-material-only update fast path**
   - Falcor: supports `isDynamic()` and updates only dynamic IDs when possible.
   - April: `MaterialSystem::update()` iterates all materials each call.

5. **Material identity/equality and duplicate removal workflow**
   - Falcor: material-level equality checks and `removeDuplicateMaterials()`.
   - April: pointer identity dedup only at insertion; no semantic duplicate collapse.

6. **Material optimization pipeline (constant-texture analysis and cleanup)**
   - Falcor: `optimizeMaterials()` with texture analyzer and material-side optimization hooks.
   - April: no equivalent optimization pass in material system.

#### 9.3.3 Shader/program contracts

7. **Per-material define merge with conflict validation**
   - Falcor: `Material::getDefines()` merged by `MaterialSystem::getDefines()` with mismatch checks.
   - April: material system provides global defines but does not merge/validate per-material define sets.

8. **Material instance byte-size aggregation**
   - Falcor: computes max `getMaterialInstanceByteSize()` across active materials and emits `FALCOR_MATERIAL_INSTANCE_SIZE`.
   - April: currently emits fixed `128` in `MaterialSystem::getShaderDefines()`.

9. **Volume phase-function conformances injected by material system**
   - Falcor: adds several phase-function conformances in `getTypeConformances()`.
   - April: no equivalent conformance injection path.

#### 9.3.4 Tooling/authoring

10. **Formal param layout contract for differentiable/serialized params**
    - Falcor: `MaterialParamLayout` and `SerializedMaterialParams` APIs.
    - April: JSON serialization exists but no equivalent param-layout schema contract.

11. **Thread-safe global material type registry with strict ID budget enforcement**
    - Falcor: mutex-protected registry, monotonic IDs, hard cap check against `kMaterialTypeBits`.
    - April: hash-based extension IDs with collision bumping; no global lock contract.

### 9.4 Same-Named Features with Behavioral Mismatch

| Feature | Falcor behavior | April behavior | Gap class |
|--------|------------------|----------------|-----------|
| Alpha mode encoding | Header bit budget matches supported alpha modes | `AlphaMode::Blend` exists while header allocates only 1 alpha bit | **P0 correctness risk** |
| Material system binding boundary | Material resources are bound through one reflected `ParameterBlock` | Material data buffer bound via `bindShaderData()`, texture/sampler arrays are manually bound in scene pass | Architectural divergence |
| Define strictness | Material system defines are authoritative and consistently consumed | Scene shader keeps fallback defaults (`128/8`) which can mask define propagation errors | Build/runtime contract fragility |
| Conformance/module metadata reuse | Rebuilt once and consumed from canonical metadata sets | Metadata cache is built, but some query APIs still re-iterate materials | Efficiency/consistency drift |
| Material hints | Includes normal-map disable + shading-normal adjust behavior | Includes normal-map disable + opacity disable, no shading-normal adjust equivalent | Semantic drift |

### 9.5 Priority Re-ranking after this Audit

#### P0 (must fix)

1. **AlphaMode packing mismatch** (`Blend` vs 1-bit storage in header).
2. **Single authoritative material binding contract** (avoid split ownership between material system and scene pass).

#### P1 (high value)

3. Add `ParameterBlock` reflection-backed binding path, including byte-size validation.
4. Introduce centralized texture/resource manager semantics (or equivalent abstraction) for parity-level lifecycle control.
5. Add dynamic-material selective update path.

#### P2 (feature parity)

6. Add per-material define merge + conflict checks.
7. Compute `FALCOR_MATERIAL_INSTANCE_SIZE` from active materials.
8. Introduce formal param layout/serialized parameter schema.
9. Strengthen material type registry guarantees (thread-safety + explicit bit-budget policy).

---

## 10. 2026-02-11 Post-Implementation Re-Compare (after GRAPHICS-MATERIAL-629 slices)

This section re-checks the same Falcor references after the latest implementation batch.

### 10.1 Deltas closed in this batch

1. **Alpha mode ABI mismatch**
   - Closed: shared/generated `AlphaMode::Blend` removed from engine material contract; host migration maps legacy `BLEND` inputs to `MASK`.

2. **Split host ownership for material binding**
   - Closed: scene path now binds material resources via `MaterialSystem::bindShaderData()` against `ParameterBlock<MaterialSystem>`.

3. **Material hint semantic drift**
   - Closed: hints now match Falcor semantics (`DisableNormalMapping`, `AdjustShadingNormal`).

4. **Define contract looseness**
   - Closed: scene/material shader contracts now hard-require host-provided material-system defines; `FALCOR_MATERIAL_INSTANCE_SIZE` is mandatory.

5. **Material instance interface shape drift**
   - Largely closed: `IMaterialInstance` switched to Falcor-style template eval/sample signatures using `ISampleGenerator`; `BSDFProperties` payload updated toward Falcor structure.

6. **Registry policy drift**
   - Closed: extension type IDs are mutex-protected monotonic allocations with explicit bit-budget guard.

7. **Resource API surface gap (partial)**
   - Improved: host now exposes `add*/replace*` resource mutation entry points for texture/sampler/buffer/texture3D with `ResourcesChanged` signaling.

8. **April-only built-in Unlit + metadata type dispatch**
   - Closed in latest destructive pass: `Unlit` host/shader implementation removed from active material architecture and scene registry now always creates Falcor-aligned standard PBR materials.

9. **April-only JSON serialization on `IMaterial`**
   - Closed in latest destructive pass: serialization API removed from the `IMaterial` contract.

### 10.2 Remaining differences (still not zero-delta)

1. **Falcor `TextureManager` parity is partially implemented**
   - April now uses `MaterialTextureManager` abstraction with descriptor ownership and deferred-loader queue, but still lacks Falcor-level texture analysis/invalidation policies.

2. **Falcor `MaterialParamLayout`/`SerializedMaterialParams` contract is partially implemented**
   - Shader-side contract files now exist and are integrated into module lists; host-side full parameter marshaling pipeline is still pending.

3. **Falcor material utility flows are only partially implemented**
   - Host now exposes `removeDuplicateMaterials()` / `optimizeMaterials()` entry points, but does not yet include Falcor-level texture-analysis optimization passes.

4. **Falcor phase-function conformance injection path is partially implemented**
   - `IPhaseFunction` + isotropic/HG implementations and host conformance injection are present; volumetric runtime integration depth is still limited.

5. **Full ParameterBlock reflection lifecycle parity remains partial**
   - Binding now upgrades through nested `ParameterBlock` root when available, but MaterialSystem still does not own a dedicated reflected materials block lifecycle object.

### 10.3 Updated parity status

- **Core runtime ABI/dispatch:** high parity.
- **Host/shader material-system contract strictness:** high parity.
- **Material resource lifecycle/tooling ecosystem:** high-minus parity.
- **Authoring/serialization and utility ecosystem parity:** medium-high parity (remaining gaps are deep optimization/marshaling integration).

### 10.4 Latest Architecture Delta Check (2026-02-11)

The following items remain the concrete non-zero deltas after the latest destructive convergence pass:

1. **TextureManager runtime integration depth is still partial**
   - `MaterialTextureManager` now owns descriptor registration/lookup, deferred loaders are consumed inside `MaterialSystem::update()`, and replacement invalidates stale texture views; remaining gap is broader Falcor-equivalent invalidation/streaming policy depth.
   - Evidence:
     - `engine/graphics/source/graphics/material/material-texture-manager.hpp`
     - `engine/graphics/source/graphics/material/material-texture-manager.cpp`
     - `engine/graphics/source/graphics/material/material-system.cpp`

2. **Material parameter layout contract is scaffolded, not end-to-end**
   - Shader contracts for `MaterialParamLayout` and `SerializedMaterialParams` are present and host now supports marshaled layout/serialized payload upload + binding hooks (`setMaterialParamLayout()`, `setSerializedMaterialParams()`); remaining gap is full editor/asset-driven production data population.
   - Evidence:
     - `engine/graphics/shader/material/material-param-layout.slang`
     - `engine/graphics/shader/material/serialized-material-params.slang`
     - `engine/graphics/source/graphics/material/standard-material.cpp`

3. **Phase-function path is contract-complete but runtime-thin**
   - `IPhaseFunction` and isotropic/Henyey-Greenstein implementations exist, host type-conformance injection is in place, and material-system runtime now exposes phase evaluation helper; remaining gap is wider volumetric pipeline consumption coverage.
   - Evidence:
     - `engine/graphics/shader/material/phase/i-phase-function.slang`
     - `engine/graphics/shader/material/phase/isotropic-phase-function.slang`
     - `engine/graphics/shader/material/phase/henyey-greenstein-phase-function.slang`
     - `engine/graphics/source/graphics/material/material-system.cpp`

4. **ParameterBlock lifecycle ownership remains partial**
   - Binding now upgrades nested parameter-block variables to root scope and `MaterialSystem` maintains a dedicated reflected materials binding block with byte-size validation/recreation; remaining gap is full decoupling so all binding writes flow through the owned block instance.
   - Evidence:
     - `engine/graphics/source/graphics/material/material-system.cpp`
     - `engine/graphics/source/graphics/rhi/parameter-block.cpp`

5. **Material optimization path lacks Falcor-level analyzers**
   - `removeDuplicateMaterials()` / `optimizeMaterials()` now include texture-impact optimization rules and constant-texture pruning through `MaterialTextureAnalyzer` (1x1 emissive/normal analysis), but still lack full Falcor-level deep rewrite/packing passes.
   - Evidence:
     - `engine/graphics/source/graphics/material/material-system.hpp`
     - `engine/graphics/source/graphics/material/material-system.cpp`
