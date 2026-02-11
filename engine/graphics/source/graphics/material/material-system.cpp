#include "material-system.hpp"

#include "program/shader-variable.hpp"
#include "rhi/command-context.hpp"
#include "rhi/parameter-block.hpp"
#include "rhi/render-device.hpp"
#include "texture-analyzer.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <numeric>

namespace april::graphics
{
    namespace
    {
        auto constexpr kMaterialDataName = "materialData";
        auto constexpr kMaterialSamplersName = "materialSamplers";
        auto constexpr kMaterialTexturesName = "materialTextures";
        auto constexpr kMaterialBuffersName = "materialBuffers";
        auto constexpr kMaterialTextures3DName = "materialTextures3D";

        auto constexpr kMaxSamplerCount = 1ull << generated::MaterialHeader::kSamplerIDBits;
        auto constexpr kMaxTextureCount = 1ull << generated::TextureHandle::kTextureIDBits;

        auto isSpecGloss(core::ref<IMaterial> const& p_material) -> bool
        {
            (void)p_material;
            return false;
        }
    }

    MaterialSystem::MaterialSystem(core::ref<Device> p_device)
        : m_device(std::move(p_device))
    {
        AP_ASSERT(m_device);
        AP_ASSERT(kMaxSamplerCount <= m_device->getLimits().maxShaderVisibleSamplers);

        m_fence = m_device->createFence();
        m_textureManager = std::make_unique<MaterialTextureManager>();

        auto samplerDesc = Sampler::Desc{};
        samplerDesc.setFilterMode(TextureFilteringMode::Linear, TextureFilteringMode::Linear, TextureFilteringMode::Linear);
        samplerDesc.setMaxAnisotropy(8);
        m_defaultTextureSampler = m_device->createSampler(samplerDesc);
    }

    auto MaterialSystem::updateUI() -> void
    {
    }

    auto MaterialSystem::setDefaultTextureSampler(core::ref<Sampler> const& p_sampler) -> void
    {
        m_defaultTextureSampler = p_sampler;
        for (auto const& p_material : m_materials)
        {
            if (p_material)
            {
                p_material->setDefaultTextureSampler(p_sampler);
            }
        }
    }

    auto MaterialSystem::addTextureSampler(core::ref<Sampler> const& p_sampler) -> uint32_t
    {
        AP_ASSERT(p_sampler);

        auto const isEqual = [&p_sampler](core::ref<Sampler> const& p_other)
        {
            return p_other && (p_sampler->getDesc() == p_other->getDesc());
        };

        if (auto const it = std::find_if(m_textureSamplers.begin(), m_textureSamplers.end(), isEqual); it != m_textureSamplers.end())
        {
            return static_cast<uint32_t>(std::distance(m_textureSamplers.begin(), it));
        }

        if (m_textureSamplers.size() >= kMaxSamplerCount)
        {
            // TODO(falcor-align): FALCOR_CHECK/FALCOR_THROW on sampler overflow.
            AP_ERROR("Too many samplers");
            return 0;
        }

        auto const samplerId = static_cast<uint32_t>(m_textureSamplers.size());
        m_textureSamplers.push_back(p_sampler);
        m_samplersChanged = true;
        return samplerId;
    }

    auto MaterialSystem::addBuffer(core::ref<Buffer> const& p_buffer) -> uint32_t
    {
        AP_ASSERT(p_buffer);

        if (auto const it = std::find(m_buffers.begin(), m_buffers.end(), p_buffer); it != m_buffers.end())
        {
            return static_cast<uint32_t>(std::distance(m_buffers.begin(), it));
        }

        if (m_buffers.size() >= m_bufferDescCount)
        {
            // TODO(falcor-align): FALCOR_CHECK/FALCOR_THROW on buffer overflow.
            AP_ERROR("Too many buffers");
            return 0;
        }

        auto const bufferId = static_cast<uint32_t>(m_buffers.size());
        m_buffers.push_back(p_buffer);
        m_buffersChanged = true;
        return bufferId;
    }

    auto MaterialSystem::replaceBuffer(uint32_t id, core::ref<Buffer> const& p_buffer) -> void
    {
        AP_ASSERT(p_buffer);
        if (id >= m_buffers.size())
        {
            // TODO(falcor-align): FALCOR_CHECK on buffer id bounds.
            AP_ERROR("Buffer id out of bounds");
            return;
        }

        m_buffers[id] = p_buffer;
        m_buffersChanged = true;
    }

    auto MaterialSystem::addTexture3D(core::ref<Texture> const& p_texture) -> uint32_t
    {
        AP_ASSERT(p_texture);

        if (auto const it = std::find(m_textures3D.begin(), m_textures3D.end(), p_texture); it != m_textures3D.end())
        {
            return static_cast<uint32_t>(std::distance(m_textures3D.begin(), it));
        }

        if (m_textures3D.size() >= m_texture3DDescCount)
        {
            // TODO(falcor-align): FALCOR_CHECK/FALCOR_THROW on 3D texture overflow.
            AP_ERROR("Too many 3D textures");
            return 0;
        }

        auto const textureId = static_cast<uint32_t>(m_textures3D.size());
        m_textures3D.push_back(p_texture);
        m_textures3DChanged = true;
        return textureId;
    }

    auto MaterialSystem::addMaterial(core::ref<IMaterial> const& p_material) -> uint32_t
    {
        if (!p_material)
        {
            // TODO(falcor-align): FALCOR_CHECK for missing material.
            AP_ERROR("'p_material' is missing");
            return std::numeric_limits<uint32_t>::max();
        }

        if (auto const it = std::find(m_materials.begin(), m_materials.end(), p_material); it != m_materials.end())
        {
            return static_cast<uint32_t>(std::distance(m_materials.begin(), it));
        }

        if (m_materials.size() >= std::numeric_limits<uint32_t>::max())
        {
            // TODO(falcor-align): FALCOR_THROW when material count overflows uint32_t.
            AP_ERROR("Too many materials");
            return std::numeric_limits<uint32_t>::max();
        }

        if (p_material->getDefaultTextureSampler() == nullptr)
        {
            p_material->setDefaultTextureSampler(m_defaultTextureSampler);
        }

        p_material->registerUpdateCallback([this](auto const flags) { m_materialUpdates |= flags; });
        m_materials.push_back(p_material);
        m_materialsChanged = true;
        return static_cast<uint32_t>(m_materials.size() - 1);
    }

    auto MaterialSystem::removeMaterial(uint32_t materialId) -> void
    {
        if (materialId >= m_materials.size())
        {
            // TODO(falcor-align): FALCOR_CHECK for material id validity.
            AP_ERROR("Material id invalid");
            return;
        }

        auto const& p_material = m_materials[materialId];
        if (p_material)
        {
            m_reservedTextureDescCount += p_material->getMaxTextureCount();
            m_reservedBufferDescCount += p_material->getMaxBufferCount();
            m_reservedTexture3DDescCount += p_material->getMaxTexture3DCount();
        }

        m_materials[materialId] = nullptr;
        m_materialsChanged = true;
    }

    auto MaterialSystem::replaceMaterial(uint32_t materialId, core::ref<IMaterial> const& p_replacement) -> void
    {
        if (materialId >= m_materials.size())
        {
            // TODO(falcor-align): FALCOR_CHECK for material id validity.
            AP_ERROR("Material id invalid");
            return;
        }
        if (!p_replacement)
        {
            // TODO(falcor-align): FALCOR_CHECK for missing replacement material.
            AP_ERROR("Replacement material missing");
            return;
        }

        removeMaterial(materialId);

        if (p_replacement->getDefaultTextureSampler() == nullptr)
        {
            p_replacement->setDefaultTextureSampler(m_defaultTextureSampler);
        }
        p_replacement->registerUpdateCallback([this](auto const flags) { m_materialUpdates |= flags; });

        m_materials[materialId] = p_replacement;
        m_materialsChanged = true;
    }

    auto MaterialSystem::replaceMaterial(core::ref<IMaterial> const& p_material, core::ref<IMaterial> const& p_replacement) -> void
    {
        if (!p_material)
        {
            // TODO(falcor-align): FALCOR_CHECK for missing source material.
            AP_ERROR("'p_material' is missing");
            return;
        }

        if (auto const it = std::find(m_materials.begin(), m_materials.end(), p_material); it != m_materials.end())
        {
            replaceMaterial(static_cast<uint32_t>(std::distance(m_materials.begin(), it)), p_replacement);
        }
        else
        {
            // TODO(falcor-align): FALCOR_THROW when source material does not exist.
            AP_ERROR("Material does not exist");
        }
    }

    auto MaterialSystem::getMaterialCountByType(generated::MaterialType type) const -> uint32_t
    {
        // TODO(falcor-align): FALCOR_CHECK(!mMaterialsChanged, "Materials have changed. Call update() first.").
        auto const index = static_cast<size_t>(type);
        return index < m_materialCountByType.size() ? m_materialCountByType[index] : 0;
    }

    auto MaterialSystem::getMaterialTypes() const -> std::set<generated::MaterialType>
    {
        // TODO(falcor-align): FALCOR_CHECK(!mMaterialsChanged, "Materials have changed. Call update() first.").
        return m_materialTypes;
    }

    auto MaterialSystem::hasMaterialType(generated::MaterialType type) const -> bool
    {
        // TODO(falcor-align): FALCOR_CHECK(!mMaterialsChanged, "Materials have changed. Call update() first.").
        return m_materialTypes.find(type) != m_materialTypes.end();
    }

    auto MaterialSystem::hasMaterial(uint32_t materialId) const -> bool
    {
        return materialId < m_materials.size();
    }

    auto MaterialSystem::getMaterial(uint32_t materialId) const -> core::ref<IMaterial> const&
    {
        // TODO(falcor-align): FALCOR_CHECK on material id range.
        AP_ASSERT(materialId < m_materials.size());
        return m_materials[materialId];
    }

    auto MaterialSystem::getMaterialByName(std::string const& name) const -> core::ref<IMaterial>
    {
        for (auto const& p_material : m_materials)
        {
            if (p_material && p_material->getName() == name)
            {
                return p_material;
            }
        }
        return nullptr;
    }

    auto MaterialSystem::removeDuplicateMaterials(std::vector<uint32_t>& idMap) -> size_t
    {
        auto uniqueMaterials = std::vector<core::ref<IMaterial>>{};
        idMap.resize(m_materials.size());

        for (auto id = uint32_t{0}; id < m_materials.size(); ++id)
        {
            auto const& p_material = m_materials[id];
            auto const it = std::find_if(uniqueMaterials.begin(), uniqueMaterials.end(), [&p_material](auto const& p_other)
            {
                return p_other->isEqual(p_material);
            });

            if (it == uniqueMaterials.end())
            {
                idMap[id] = static_cast<uint32_t>(uniqueMaterials.size());
                uniqueMaterials.push_back(p_material);
            }
            else
            {
                AP_INFO("Removing duplicate material '{}' (duplicate of '{}').", p_material->getName(), (*it)->getName());
                idMap[id] = static_cast<uint32_t>(std::distance(uniqueMaterials.begin(), it));
            }
        }

        auto const removed = m_materials.size() - uniqueMaterials.size();
        if (removed > 0)
        {
            m_materials = uniqueMaterials;
            m_materialsChanged = true;
        }

        return removed;
    }

    auto MaterialSystem::optimizeMaterials() -> void
    {
        auto materialSlots = std::vector<std::pair<core::ref<IMaterial>, Material::TextureSlot>>{};
        auto textures = std::vector<core::ref<Texture>>{};
        auto const maxCount = m_materials.size() * static_cast<size_t>(Material::TextureSlot::Count);
        materialSlots.reserve(maxCount);
        textures.reserve(maxCount);

        for (auto const& p_material : m_materials)
        {
            if (!p_material)
            {
                continue;
            }

            for (auto i = uint32_t{0}; i < static_cast<uint32_t>(Material::TextureSlot::Count); ++i)
            {
                auto const slot = static_cast<Material::TextureSlot>(i);
                if (auto const p_texture = p_material->getTexture(slot))
                {
                    materialSlots.push_back({p_material, slot});
                    textures.push_back(p_texture);
                }
            }
        }

        if (textures.empty())
        {
            return;
        }

        AP_INFO("Analyzing {} material textures.", textures.size());

        auto const analyzer = TextureAnalyzer{};
        auto* p_context = m_device->getCommandContext();

        auto const resultBufferSize = textures.size() * TextureAnalyzer::getResultSize();
        auto const p_results = m_device->createBuffer(
            resultBufferSize,
            BufferUsage::UnorderedAccess | BufferUsage::CopySource,
            MemoryType::DeviceLocal,
            nullptr
        );
        analyzer.analyze(p_context, textures, p_results);

        auto const p_resultsStaging = m_device->createBuffer(
            resultBufferSize,
            BufferUsage::None,
            MemoryType::ReadBack,
            nullptr
        );

        p_context->copyBuffer(p_resultsStaging.get(), p_results.get());
        p_context->enqueueSignal(m_fence.get());
        p_context->submit(false);
        m_fence->wait();

        auto const* p_resultsData = static_cast<TextureAnalyzer::Result const*>(p_resultsStaging->map(rhi::CpuAccessMode::Read));

        auto stats = Material::TextureOptimizationStats{};
        for (auto i = size_t{0}; i < textures.size(); ++i)
        {
            materialSlots[i].first->optimizeTexture(materialSlots[i].second, p_resultsData[i], stats);
        }
        p_resultsStaging->unmap();

        auto const totalRemoved = std::accumulate(stats.texturesRemoved.begin(), stats.texturesRemoved.end(), size_t{0});
        if (totalRemoved > 0)
        {
            AP_INFO("Optimized materials by removing {} constant textures.", totalRemoved);
            for (auto slot = size_t{0}; slot < static_cast<size_t>(Material::TextureSlot::Count); ++slot)
            {
                AP_INFO("  slot {}: {}", slot, stats.texturesRemoved[slot]);
            }

            m_materialUpdates |= Material::UpdateFlags::ResourcesChanged;
        }

        if (stats.disabledAlpha > 0)
        {
            AP_INFO("Optimized materials by disabling alpha test for {} materials.", stats.disabledAlpha);
        }

        if (stats.constantBaseColor > 0)
        {
            AP_WARN("Materials have {} base color maps of constant value with non-constant alpha channel.", stats.constantBaseColor);
        }

        if (stats.constantNormalMaps > 0)
        {
            AP_WARN("Materials have {} normal maps of constant value. Please update the asset to optimize performance.", stats.constantNormalMaps);
        }
    }

    auto MaterialSystem::update(bool forceUpdate) -> Material::UpdateFlags
    {
        auto reupdateMetadata = false;

        if (forceUpdate || m_materialsChanged)
        {
            updateMetadata();
            updateUI();

            m_materialsBlock = nullptr;
            m_materialsChanged = false;
            forceUpdate = true;
            reupdateMetadata = true;
        }

        auto updateFlags = Material::UpdateFlags::None;
        m_materialsUpdateFlags.resize(m_materials.size());
        std::fill(m_materialsUpdateFlags.begin(), m_materialsUpdateFlags.end(), Material::UpdateFlags::None);

        auto updateMaterial = [this, &updateFlags](uint32_t materialId)
        {
            auto& p_material = m_materials[materialId];
            if (!p_material)
            {
                return;
            }

            // TODO(falcor-align): Check material device matches material-system device and throw on mismatch.

            auto const flags = p_material->update(this);
            m_materialsUpdateFlags[materialId] = flags;
            updateFlags |= flags;
        };

        if (forceUpdate || m_materialUpdates != Material::UpdateFlags::None)
        {
            // TODO(falcor-align): beginDeferredLoading().
            for (auto materialId = uint32_t{0}; materialId < static_cast<uint32_t>(m_materials.size()); ++materialId)
            {
                updateMaterial(materialId);
            }
            // TODO(falcor-align): endDeferredLoading().
        }
        else
        {
            for (auto const& materialId : m_dynamicMaterialIds)
            {
                updateMaterial(materialId);
            }
        }

        if (reupdateMetadata)
        {
            updateMetadata();
            reupdateMetadata = false;
        }

        if (m_lightProfile && !m_lightProfileBaked)
        {
            // TODO(falcor-align): Bake light profile using render context before marking baked.
            m_lightProfileBaked = true;
        }

        updateFlags |= m_materialUpdates;
        m_materialUpdates = Material::UpdateFlags::None;

        if (!m_materialsBlock)
        {
            createParameterBlock();
            if (!m_materialsBlock)
            {
                // TODO(falcor-align): Ensure createParameterBlock() always succeeds once reflection bootstrap is aligned.
                return updateFlags;
            }
            updateFlags |= Material::UpdateFlags::DataChanged | Material::UpdateFlags::ResourcesChanged;
            forceUpdate = true;
        }

        if (forceUpdate || enum_has_any_flags(updateFlags, Material::UpdateFlags::DataChanged))
        {
            if (!m_materials.empty() && (!m_materialDataBuffer || m_materialDataBuffer->getElementCount() < m_materials.size()))
            {
                m_materialDataBuffer = core::make_ref<Buffer>(
                    m_device,
                    static_cast<uint32_t>(sizeof(generated::MaterialDataBlob)),
                    static_cast<uint32_t>(m_materials.size()),
                    BufferUsage::ShaderResource,
                    MemoryType::Upload,
                    nullptr,
                    false
                );
            }

            for (auto materialId = uint32_t{0}; materialId < static_cast<uint32_t>(m_materials.size()); ++materialId)
            {
                if (forceUpdate || enum_has_any_flags(m_materialsUpdateFlags[materialId], Material::UpdateFlags::DataChanged))
                {
                    uploadMaterial(materialId);
                }
            }
        }

        auto blockVar = m_materialsBlock->getRootVariable();

        if (forceUpdate || m_samplersChanged)
        {
            auto var = blockVar[kMaterialSamplersName];
            for (auto i = size_t{0}; i < m_textureSamplers.size(); ++i)
            {
                var[i].setSampler(m_textureSamplers[i]);
            }
            m_samplersChanged = false;
        }

        if (forceUpdate || enum_has_any_flags(updateFlags, Material::UpdateFlags::ResourcesChanged))
        {
            // TODO(falcor-align): Bind material textures through deferred-loading aware texture-manager shader-data path.
            // TODO(falcor-align): Assert !m_materialsChanged before binding textures.
            auto var = blockVar[kMaterialTexturesName];
            for (auto i = size_t{0}; i < m_textureDescCount; ++i)
            {
                if (auto const p_texture = m_textureManager->getTexture(static_cast<uint32_t>(i)))
                {
                    var[i].setTexture(p_texture);
                }
            }
        }

        if (forceUpdate || m_buffersChanged)
        {
            auto var = blockVar[kMaterialBuffersName];
            for (auto i = size_t{0}; i < m_buffers.size(); ++i)
            {
                var[i].setBuffer(m_buffers[i]);
            }
            m_buffersChanged = false;
        }

        if (forceUpdate || m_textures3DChanged)
        {
            auto var = blockVar[kMaterialTextures3DName];
            for (auto i = size_t{0}; i < m_textures3D.size(); ++i)
            {
                var[i].setTexture(m_textures3D[i]);
            }
            m_textures3DChanged = false;
        }

        if (forceUpdate || enum_has_any_flags(updateFlags, Material::UpdateFlags::CodeChanged))
        {
            m_shaderModules.clear();
            m_typeConformances.clear();

            for (auto const& p_material : m_materials)
            {
                if (!p_material)
                {
                    continue;
                }

                auto const materialType = p_material->getType();
                if (m_typeConformances.find(materialType) == m_typeConformances.end())
                {
                    auto const modules = p_material->getShaderModules();
                    m_shaderModules.insert(m_shaderModules.end(), modules.begin(), modules.end());
                    m_typeConformances[materialType] = p_material->getTypeConformances();
                }
            }
        }

        AP_ASSERT(m_materialUpdates == Material::UpdateFlags::None);
        return updateFlags;
    }

    auto MaterialSystem::updateMetadata() -> void
    {
        m_textureDescCount = m_reservedTextureDescCount;
        m_bufferDescCount = m_reservedBufferDescCount;
        m_texture3DDescCount = m_reservedTexture3DDescCount;

        m_materialCountByType.resize(static_cast<size_t>(generated::MaterialType::Count));
        std::fill(m_materialCountByType.begin(), m_materialCountByType.end(), 0);
        m_materialTypes.clear();
        m_hasSpecGlossStandardMaterial = false;
        m_dynamicMaterialIds.clear();

        for (auto materialIdx = size_t{0}; materialIdx < m_materials.size(); ++materialIdx)
        {
            auto const& p_material = m_materials[materialIdx];
            if (!p_material)
            {
                continue;
            }

            if (p_material->isDynamic())
            {
                m_dynamicMaterialIds.push_back(static_cast<uint32_t>(materialIdx));
            }

            m_textureDescCount += p_material->getMaxTextureCount();
            m_bufferDescCount += p_material->getMaxBufferCount();
            m_texture3DDescCount += p_material->getMaxTexture3DCount();

            auto const typeIndex = static_cast<size_t>(p_material->getType());
            AP_ASSERT(typeIndex < m_materialCountByType.size());
            ++m_materialCountByType[typeIndex];
            m_materialTypes.insert(p_material->getType());
            if (isSpecGloss(p_material))
            {
                m_hasSpecGlossStandardMaterial = true;
            }
        }
        // TODO(falcor-align): FALCOR_CHECK(mMaterialTypes.find(MaterialType::Unknown) == mMaterialTypes.end(), "Unknown material type found. Make sure all material types are registered.").
    }

    auto MaterialSystem::getStats() const -> MaterialStats
    {
        // TODO(falcor-align): FALCOR_CHECK(!mMaterialsChanged, "Materials have changed. Call update() first.").
        auto stats = MaterialStats{};
        stats.materialTypeCount = m_materialTypes.size();
        stats.materialCount = m_materials.size();
        stats.materialOpaqueCount = 0;
        stats.materialMemoryInBytes += m_materialDataBuffer ? m_materialDataBuffer->getSize() : 0;

        for (auto const& p_material : m_materials)
        {
            if (p_material && p_material->isOpaque())
            {
                ++stats.materialOpaqueCount;
            }
        }

        auto const textureStats = m_textureManager->getStats();
        stats.textureCount = textureStats.textureCount;
        stats.textureCompressedCount = textureStats.textureCompressedCount;
        stats.textureTexelCount = textureStats.textureTexelCount;
        stats.textureTexelChannelCount = textureStats.textureTexelChannelCount;
        stats.textureMemoryInBytes = textureStats.textureMemoryInBytes;

        return stats;
    }

    auto MaterialSystem::loadLightProfile(std::filesystem::path const& absoluteFilename, bool normalize) -> void
    {
        // TODO:
        (void)absoluteFilename;
        (void)normalize;
        m_lightProfileBaked = false;
    }

    auto MaterialSystem::getDefines(DefineList& defines) const -> void
    {
        // TODO(falcor-align): FALCOR_CHECK(!mMaterialsChanged, "Materials have changed. Call update() first.").
        auto materialInstanceByteSize = size_t{0};
        for (auto const& p_material : m_materials)
        {
            if (p_material)
            {
                materialInstanceByteSize = std::max(materialInstanceByteSize, p_material->getMaterialInstanceByteSize());
            }
        }

        defines.add("MATERIAL_SYSTEM_SAMPLER_DESC_COUNT", std::to_string(kMaxSamplerCount));
        defines.add("MATERIAL_SYSTEM_TEXTURE_DESC_COUNT", std::to_string(m_textureDescCount));
        defines.add("MATERIAL_SYSTEM_BUFFER_DESC_COUNT", std::to_string(m_bufferDescCount));
        defines.add("MATERIAL_SYSTEM_TEXTURE_3D_DESC_COUNT", std::to_string(m_texture3DDescCount));
        defines.add("MATERIAL_SYSTEM_UDIM_INDIRECTION_ENABLED", "0");
        defines.add("MATERIAL_SYSTEM_HAS_SPEC_GLOSS_MATERIALS", m_hasSpecGlossStandardMaterial ? "1" : "0");
        defines.add("MATERIAL_SYSTEM_USE_LIGHT_PROFILE", m_lightProfile ? "1" : "0");
        defines.add("FALCOR_MATERIAL_INSTANCE_SIZE", std::to_string(materialInstanceByteSize));

        for (auto const& p_material : m_materials)
        {
            if (!p_material)
            {
                continue;
            }

            auto const materialDefines = p_material->getDefines();
            for (auto const& [name, value] : materialDefines)
            {
                if (auto const it = defines.find(name); it != defines.end())
                {
                    if (it->second != value)
                    {
                        AP_ERROR("Mismatching values '{}' and '{}' for material define '{}'.", it->second, value, name);
                    }
                }
                else
                {
                    defines.add(name, value);
                }
            }
        }
    }

    auto MaterialSystem::getTypeConformances(TypeConformanceList& typeConformances) const -> void
    {
        for (auto const& it : m_typeConformances)
        {
            typeConformances.add(it.second);
        }
        typeConformances.add("NullPhaseFunction", "IPhaseFunction", 0);
        typeConformances.add("IsotropicPhaseFunction", "IPhaseFunction", 1);
        typeConformances.add("HenyeyGreensteinPhaseFunction", "IPhaseFunction", 2);
        typeConformances.add("DualHenyeyGreensteinPhaseFunction", "IPhaseFunction", 3);
    }

    auto MaterialSystem::getTypeConformances(generated::MaterialType type) const -> TypeConformanceList
    {
        if (auto const it = m_typeConformances.find(type); it != m_typeConformances.end())
        {
            return it->second;
        }

        AP_ERROR("No type conformances for material type '{}'.", static_cast<uint32_t>(type));
        return {};
    }

    auto MaterialSystem::getShaderModules() const -> ProgramDesc::ShaderModuleList
    {
        // TODO(falcor-align): FALCOR_CHECK(!mMaterialsChanged, "Materials have changed. Call update() first.").
        return m_shaderModules;
    }

    auto MaterialSystem::getShaderModules(ProgramDesc::ShaderModuleList& shaderModuleList) const -> void
    {
        // TODO(falcor-align): FALCOR_CHECK(!mMaterialsChanged, "Materials have changed. Call update() first.").
        shaderModuleList.insert(shaderModuleList.end(), m_shaderModules.begin(), m_shaderModules.end());
    }

    auto MaterialSystem::bindShaderData(ShaderVariable const& var) const -> void
    {
        if (!m_materialsBlock || m_materialsChanged)
        {
            // TODO(falcor-align): FALCOR_CHECK(m_materialsBlock != nullptr && !m_materialsChanged, "Parameter block is not ready. Call update() first.").
            AP_ERROR("Parameter block is not ready. Call update() first.");
            return;
        }

        var = m_materialsBlock;
    }

    auto MaterialSystem::createParameterBlock() -> void
    {
        if (m_materialsBlock || !m_device)
        {
            return;
        }

        // TODO(falcor-align): Create temporary compute pass from material-system shader and fetch
        // gMaterialsBlock reflection via pass->program->reflector->getParameterBlock("gMaterialsBlock").
        // Current code relies on externally supplied reflection until ComputePass path is available.
        if (!m_materialsBlockReflection)
        {
            AP_WARN("MaterialSystem parameter-block reflection is not available yet.");
            return;
        }

        m_materialsBlock = ParameterBlock::create(m_device, m_materialsBlockReflection);
        if (!m_materialsBlock)
        {
            AP_ERROR("Failed to create MaterialSystem parameter block.");
            return;
        }

        auto const p_reflVar = m_materialsBlock->getReflection()->findMember(kMaterialDataName);
        AP_ASSERT(p_reflVar);
        auto const* p_reflResType = p_reflVar->getType()->asResourceType();
        AP_ASSERT(p_reflResType && p_reflResType->getType() == ReflectionResourceType::Type::StructuredBuffer);
        auto const byteSize = p_reflResType->getStructType()->getByteSize();
        if (byteSize != sizeof(generated::MaterialDataBlob))
        {
            // TODO(falcor-align): Match FALCOR_THROW behavior for unexpected material-data struct size.
            AP_ERROR("MaterialSystem material data buffer has unexpected struct size");
            return;
        }

        auto blockVar = m_materialsBlock->getRootVariable();
        if (!m_materials.empty() && (!m_materialDataBuffer || m_materialDataBuffer->getElementCount() < m_materials.size()))
        {
            m_materialDataBuffer = m_device->createStructuredBuffer(
                blockVar[kMaterialDataName],
                static_cast<uint32_t>(m_materials.size()),
                BufferUsage::ShaderResource,
                MemoryType::DeviceLocal,
                nullptr,
                false
            );
            m_materialDataBuffer->setName("MaterialSystem::m_materialDataBuffer");
        }

        if (blockVar.hasMember(kMaterialDataName))
        {
            blockVar[kMaterialDataName].setBuffer(!m_materials.empty() ? m_materialDataBuffer : nullptr);
        }
        if (blockVar.hasMember("materialCount"))
        {
            blockVar["materialCount"].set(getMaterialCount());
        }
        if (m_lightProfile)
        {
            // FALCOR_ASSERT(m_lightProfileBaked);
            // m_lightProfile->bindShaderData(blockVar[kLightProfile]);
        }
    }

    auto MaterialSystem::uploadMaterial(uint32_t materialID) -> void
    {
        AP_ASSERT(materialID < m_materials.size());
        auto const& p_material = m_materials[materialID];
        AP_ASSERT(p_material);
        AP_ASSERT(m_materialDataBuffer);

        m_materialDataBuffer->setElement(materialID, p_material->getDataBlob());
    }
}
