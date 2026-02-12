#pragma once

#include "i-material.hpp"
#include "material-texture-manager.hpp"
#include "program/define-list.hpp"
#include "program/program.hpp"
#include "rhi/buffer.hpp"
#include "rhi/sampler.hpp"
#include "rhi/texture.hpp"

#include <cstdint>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace april::graphics
{
    class Device;
    class Fence;
    class LightProfile;
    class ParameterBlock;
    class ParameterBlockReflection;
    class ShaderVariable;

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
        explicit MaterialSystem(core::ref<Device> p_device);

        auto update(bool forceUpdate) -> Material::UpdateFlags;

        auto getDefines(DefineList& defines) const -> void;
        auto getDefines() const -> DefineList
        {
            auto result = DefineList{};
            getDefines(result);
            return result;
        }

        auto getTypeConformances(TypeConformanceList& conformances) const -> void;
        auto getTypeConformances() const -> TypeConformanceList
        {
            auto typeConformances = TypeConformanceList{};
            getTypeConformances(typeConformances);
            return typeConformances;
        }

        auto getTypeConformances(generated::MaterialType type) const -> TypeConformanceList;

        auto getShaderModules(ProgramDesc::ShaderModuleList& shaderModuleList) const -> void;
        auto getShaderModules() const -> ProgramDesc::ShaderModuleList;

        auto bindShaderData(ShaderVariable const& var) const -> void;

        auto setDefaultTextureSampler(core::ref<Sampler> const& p_sampler) -> void;

        auto addTextureSampler(core::ref<Sampler> const& p_sampler) -> uint32_t;

        auto getTextureSamplerCount() const -> uint32_t
        {
            return static_cast<uint32_t>(m_textureSamplers.size());
        }

        auto getTextureSampler(uint32_t const samplerId) const -> core::ref<Sampler> const&
        {
            return m_textureSamplers[samplerId];
        }

        auto addBuffer(core::ref<Buffer> const& p_buffer) -> uint32_t;
        auto replaceBuffer(uint32_t id, core::ref<Buffer> const& p_buffer) -> void;

        auto getBufferCount() const -> uint32_t
        {
            return static_cast<uint32_t>(m_buffers.size());
        }

        auto addTexture3D(core::ref<Texture> const& p_texture) -> uint32_t;

        auto getTexture3DCount() const -> uint32_t
        {
            return static_cast<uint32_t>(m_textures3D.size());
        }

        // TODO(falcor-align): MaterialID type is not implemented yet; use uint32_t as temporary substitute.
        auto addMaterial(core::ref<IMaterial> const& p_material) -> uint32_t;
        auto removeMaterial(uint32_t materialId) -> void;
        auto replaceMaterial(uint32_t materialId, core::ref<IMaterial> const& p_replacement) -> void;
        auto replaceMaterial(core::ref<IMaterial> const& p_material, core::ref<IMaterial> const& p_replacement) -> void;

        auto getMaterials() const -> std::vector<core::ref<IMaterial>> const&
        {
            return m_materials;
        }

        auto getMaterialCount() const -> uint32_t
        {
            return static_cast<uint32_t>(m_materials.size());
        }

        auto getMaterialCountByType(generated::MaterialType type) const -> uint32_t;
        auto getMaterialTypes() const -> std::set<generated::MaterialType>;
        auto hasMaterialType(generated::MaterialType type) const -> bool;
        auto hasMaterial(uint32_t materialId) const -> bool;
        auto getMaterial(uint32_t materialId) const -> core::ref<IMaterial> const&;
        auto getMaterialByName(std::string const& name) const -> core::ref<IMaterial>;

        auto removeDuplicateMaterials(std::vector<uint32_t>& idMap) -> size_t;

        auto optimizeMaterials() -> void;

        auto getStats() const -> MaterialStats;

        auto getTextureManager() -> MaterialTextureManager&
        {
            return *m_textureManager;
        }

        auto loadLightProfile(std::filesystem::path const& absoluteFilename, bool normalize) -> void;

        auto getLightProfile() const -> LightProfile const*
        {
            return m_lightProfile.get();
        }

    private:
        auto updateMetadata() -> void;
        auto updateUI() -> void;
        auto createParameterBlock() -> void;
        auto uploadMaterial(uint32_t materialID) -> void;

        core::ref<Device> m_device;

        std::vector<core::ref<IMaterial>> m_materials;
        std::vector<Material::UpdateFlags> m_materialsUpdateFlags;
        std::unique_ptr<MaterialTextureManager> m_textureManager{};
        ProgramDesc::ShaderModuleList m_shaderModules;
        std::map<generated::MaterialType, TypeConformanceList> m_typeConformances;
        core::ref<LightProfile> m_lightProfile{};
        bool m_lightProfileBaked = true;

        size_t m_textureDescCount = 0;
        size_t m_bufferDescCount = 0;
        size_t m_texture3DDescCount = 0;
        size_t m_reservedTextureDescCount = 0;
        size_t m_reservedBufferDescCount = 0;
        size_t m_reservedTexture3DDescCount = 0;
        std::vector<uint32_t> m_materialCountByType;
        std::set<generated::MaterialType> m_materialTypes;
        bool m_hasSpecGlossStandardMaterial = false;
        std::vector<uint32_t> m_dynamicMaterialIds;

        bool m_samplersChanged = false;
        bool m_buffersChanged = false;
        bool m_textures3DChanged = false;
        bool m_materialsChanged = false;

        Material::UpdateFlags m_materialUpdates = Material::UpdateFlags::None;

        core::ref<Fence> m_fence{};
        core::ref<ParameterBlock> m_materialsBlock{};
        core::ref<Buffer> m_materialDataBuffer{};
        core::ref<Sampler> m_defaultTextureSampler{};
        std::vector<core::ref<Sampler>> m_textureSamplers;
        std::vector<core::ref<Buffer>> m_buffers;
        std::vector<core::ref<Texture>> m_textures3D;
        core::ref<ParameterBlockReflection const> m_materialsBlockReflection{};

    };
}
