// MaterialSystem - Falcor-aligned material system core.

#pragma once

#include "i-material.hpp"
#include "material-texture-manager.hpp"
#include "material-type-registry.hpp"
#include "rhi/buffer.hpp"
#include "rhi/sampler.hpp"
#include "rhi/texture.hpp"
#include "generated/material/material-data.generated.hpp"
#include "program/define-list.hpp"

#include <vector>
#include <unordered_map>
#include <map>
#include <cstdint>

namespace april::graphics
{
    class Device;
    class ParameterBlock;
    class ShaderVariable;

    struct MaterialParamLayoutEntry
    {
        uint32_t nameHash{0};
        uint32_t offset{0};
        uint32_t size{0};
        uint32_t type{0};
    };

    struct SerializedMaterialParam
    {
        uint32_t nameHash{0};
        uint32_t type{0};
        uint32_t dataOffset{0};
        uint32_t dataSize{0};
    };

    struct MaterialStats
    {
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

    class MaterialSystem : public core::Object
    {
        APRIL_OBJECT(MaterialSystem)
    public:
        static constexpr size_t kMaxSamplerCount = 1ull << 8;
        static constexpr size_t kMaxTextureCount = 1ull << 30;

        explicit MaterialSystem(core::ref<Device> device);
        ~MaterialSystem() override = default;

        auto getShaderDefines() const -> DefineList;

        auto addMaterial(core::ref<IMaterial> material) -> uint32_t;
        auto removeMaterial(uint32_t index) -> void;
        auto getMaterial(uint32_t index) const -> core::ref<IMaterial>;
        auto getMaterialCount() const -> uint32_t;

        auto update(bool forceUpdate = false) -> MaterialUpdateFlags;
        auto bindShaderData(ShaderVariable const& var) const -> void;

        auto getStats() const -> MaterialStats;
        auto getMaterialDataBuffer() const -> core::ref<Buffer>;
        auto getTextureDescriptorCount() const -> uint32_t { return m_textureDescCount; }
        auto getSamplerDescriptorCount() const -> uint32_t { return static_cast<uint32_t>(kMaxSamplerCount); }
        auto getBufferDescriptorCount() const -> uint32_t { return m_bufferDescCount; }
        auto getTexture3DDescriptorCount() const -> uint32_t { return m_texture3DDescCount; }

        auto getTypeConformances() const -> TypeConformanceList;
        auto getTypeConformances(generated::MaterialType type) const -> TypeConformanceList;
        auto getShaderModules() const -> ProgramDesc::ShaderModuleList;
        auto getShaderModules(ProgramDesc::ShaderModuleList& modules) const -> void;

        using DescriptorHandle = uint32_t;
        static constexpr DescriptorHandle kInvalidDescriptorHandle = 0;

        auto registerTextureDescriptor(core::ref<Texture> texture) -> DescriptorHandle;
        auto registerSamplerDescriptor(core::ref<Sampler> sampler) -> DescriptorHandle;
        auto registerBufferDescriptor(core::ref<Buffer> buffer) -> DescriptorHandle;
        auto registerTexture3DDescriptor(core::ref<Texture> texture) -> DescriptorHandle;

        auto addTexture(core::ref<Texture> texture) -> DescriptorHandle;
        auto replaceTexture(DescriptorHandle handle, core::ref<Texture> texture) -> bool;
        auto enqueueDeferredTextureLoad(MaterialTextureManager::DeferredTextureLoader loader) -> void;
        auto enqueueDeferredTexture3DLoad(MaterialTextureManager::DeferredTextureLoader loader) -> void;
        auto addSampler(core::ref<Sampler> sampler) -> DescriptorHandle;
        auto replaceSampler(DescriptorHandle handle, core::ref<Sampler> sampler) -> bool;
        auto addBuffer(core::ref<Buffer> buffer) -> DescriptorHandle;
        auto replaceBuffer(DescriptorHandle handle, core::ref<Buffer> buffer) -> bool;
        auto addTexture3D(core::ref<Texture> texture) -> DescriptorHandle;
        auto replaceTexture3D(DescriptorHandle handle, core::ref<Texture> texture) -> bool;

        auto getTextureDescriptorResource(DescriptorHandle handle) const -> core::ref<Texture>;
        auto getSamplerDescriptorResource(DescriptorHandle handle) const -> core::ref<Sampler>;
        auto getBufferDescriptorResource(DescriptorHandle handle) const -> core::ref<Buffer>;
        auto getTexture3DDescriptorResource(DescriptorHandle handle) const -> core::ref<Texture>;

        auto getMaterialTypeRegistry() const -> MaterialTypeRegistry const&;
        auto getMaterialTypeId(uint32_t materialIndex) const -> uint32_t;

        auto setMaterialParamLayout(std::vector<MaterialParamLayoutEntry> entries) -> void;
        auto setSerializedMaterialParams(std::vector<SerializedMaterialParam> params, std::vector<uint8_t> rawData) -> void;

        auto removeDuplicateMaterials() -> uint32_t;
        auto optimizeMaterials() -> uint32_t;

    private:
        core::ref<Device> m_device;
        core::ref<Sampler> m_defaultTextureSampler{};

        std::vector<core::ref<IMaterial>> m_materials;
        std::unordered_map<IMaterial*, uint32_t> m_materialIndices;

        core::ref<Buffer> m_materialDataBuffer;
        std::vector<generated::MaterialDataBlob> m_cpuMaterialData;
        MaterialTypeRegistry m_materialTypeRegistry{};

        std::vector<core::ref<Sampler>> m_samplerDescriptors;
        std::vector<core::ref<Buffer>> m_bufferDescriptors;
        MaterialTextureManager m_textureManager{};
        MaterialTextureManager m_texture3DManager{};
        std::unordered_map<Sampler*, DescriptorHandle> m_samplerDescriptorIndices;
        std::unordered_map<Buffer*, DescriptorHandle> m_bufferDescriptorIndices;

        std::vector<MaterialUpdateFlags> m_materialsUpdateFlags;
        std::vector<uint32_t> m_dynamicMaterialIndices;
        MaterialUpdateFlags m_materialUpdates{MaterialUpdateFlags::None};
        bool m_materialsChanged{true};
        bool m_dirty{true};

        mutable std::map<generated::MaterialType, TypeConformanceList> m_typeConformancesByType;
        mutable ProgramDesc::ShaderModuleList m_shaderModules;

        uint32_t m_textureDescCount{1};
        uint32_t m_bufferDescCount{1};
        uint32_t m_texture3DDescCount{1};

        std::vector<MaterialParamLayoutEntry> m_materialParamLayoutEntries{};
        std::vector<SerializedMaterialParam> m_serializedMaterialParams{};
        std::vector<uint8_t> m_serializedMaterialParamData{};
        core::ref<Buffer> m_materialParamLayoutBuffer{};
        core::ref<Buffer> m_serializedMaterialParamsBuffer{};
        core::ref<Buffer> m_serializedMaterialParamDataBuffer{};
        bool m_materialParamDataDirty{false};

        mutable core::ref<ParameterBlock> m_materialsBindingBlock{};
        mutable size_t m_materialsBindingBlockByteSize{0};

        static constexpr uint32_t kInitialBufferCapacity = 64;

        auto ensureBufferCapacity(uint32_t requiredCount) -> void;
        auto updateMetadata() -> void;
        auto uploadMaterial(uint32_t materialId) -> void;
    };

} // namespace april::graphics
