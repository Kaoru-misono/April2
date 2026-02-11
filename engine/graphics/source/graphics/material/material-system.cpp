// MaterialSystem implementation.

#include "material-system.hpp"
#include "basic-material.hpp"
#include "standard-material.hpp"
#include "program/shader-variable.hpp"
#include "rhi/parameter-block.hpp"
#include "rhi/render-device.hpp"

#include <format>
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <string_view>
#include <cstdlib>
#include <array>
#include <cmath>

namespace april::graphics
{

MaterialSystem::MaterialSystem(core::ref<Device> device)
    : m_device(std::move(device))
{
    m_materials.reserve(kInitialBufferCapacity);
    m_cpuMaterialData.reserve(kInitialBufferCapacity);

    m_samplerDescriptors.emplace_back();
    m_bufferDescriptors.emplace_back();

    if (m_device)
    {
        m_defaultTextureSampler = m_device->getDefaultSampler();
    }

    m_materialTypeRegistry.registerBuiltIn(
        "Standard",
        static_cast<uint32_t>(generated::MaterialType::Standard)
    );
}

auto MaterialSystem::getShaderDefines() const -> DefineList
{
    auto defines = DefineList{};

    defines.add("MATERIAL_SYSTEM_TEXTURE_DESC_COUNT", std::to_string(m_textureDescCount));
    defines.add("MATERIAL_SYSTEM_SAMPLER_DESC_COUNT", std::to_string(kMaxSamplerCount));
    defines.add("MATERIAL_SYSTEM_BUFFER_DESC_COUNT", std::to_string(m_bufferDescCount));
    defines.add("MATERIAL_SYSTEM_TEXTURE_3D_DESC_COUNT", std::to_string(m_texture3DDescCount));
    defines.add("MATERIAL_SYSTEM_UDIM_INDIRECTION_ENABLED", m_udimIndirectionEnabled ? "1" : "0");
    defines.add("MATERIAL_SYSTEM_USE_LIGHT_PROFILE", m_lightProfileEnabled ? "1" : "0");

    auto materialInstanceByteSize = size_t{128};
    for (auto const& material : m_materials)
    {
        if (!material)
        {
            continue;
        }
        materialInstanceByteSize = std::max(materialInstanceByteSize, material->getMaterialInstanceByteSize());
    }
    defines.add("FALCOR_MATERIAL_INSTANCE_SIZE", std::to_string(materialInstanceByteSize));

    for (auto const& material : m_materials)
    {
        if (!material)
        {
            continue;
        }

        auto materialDefines = material->getDefines();
        for (auto const& [name, value] : materialDefines)
        {
            if (auto existing = defines.find(name); existing != defines.end() && existing->second != value)
            {
                AP_ERROR(
                    "Mismatching values '{}' and '{}' for material define '{}'.",
                    existing->second,
                    value,
                    name
                );
                std::abort();
            }
            defines.add(name, value);
        }
    }

    return defines;
}

auto MaterialSystem::addMaterial(core::ref<IMaterial> material) -> uint32_t
{
    if (!material)
    {
        AP_WARN("MaterialSystem::addMaterial: null material provided");
        return UINT32_MAX;
    }

    auto it = m_materialIndices.find(material.get());
    if (it != m_materialIndices.end())
    {
        return it->second;
    }

    if (material->getDefaultTextureSampler() == nullptr)
    {
        material->setDefaultTextureSampler(m_defaultTextureSampler);
    }

    material->registerUpdateCallback([this](auto flags)
    {
        m_materialUpdates |= flags;
    });

    auto const index = static_cast<uint32_t>(m_materials.size());
    m_materials.push_back(material);
    m_materialIndices[material.get()] = index;

    m_cpuMaterialData.emplace_back();
    m_materialsUpdateFlags.emplace_back(MaterialUpdateFlags::None);

    m_materialsChanged = true;
    m_dirty = true;
    return index;
}

auto MaterialSystem::removeMaterial(uint32_t index) -> void
{
    if (index >= m_materials.size())
    {
        AP_WARN("MaterialSystem::removeMaterial: invalid index {}", index);
        return;
    }

    auto const material = m_materials[index];
    if (material)
    {
        m_materialIndices.erase(material.get());
    }

    m_materials[index] = nullptr;
    m_materialsChanged = true;
    m_dirty = true;
}

auto MaterialSystem::getMaterial(uint32_t index) const -> core::ref<IMaterial>
{
    if (index >= m_materials.size())
    {
        return nullptr;
    }
    return m_materials[index];
}

auto MaterialSystem::getMaterialCount() const -> uint32_t
{
    return static_cast<uint32_t>(m_materials.size());
}

auto MaterialSystem::update(bool forceUpdate) -> MaterialUpdateFlags
{
    if (m_textureManager.resolveDeferred(kMaxTextureCount) || m_texture3DManager.resolveDeferred(kMaxTextureCount))
    {
        m_materialUpdates |= MaterialUpdateFlags::ResourcesChanged;
        forceUpdate = true;
    }

    if (forceUpdate || m_materialsChanged)
    {
        updateMetadata();

        if (!m_useExternalParamData)
        {
            rebuildInternalMaterialParamData();
        }

        m_materialsChanged = false;
        forceUpdate = true;
    }

    m_materialsUpdateFlags.resize(m_materials.size());
    std::fill(m_materialsUpdateFlags.begin(), m_materialsUpdateFlags.end(), MaterialUpdateFlags::None);

    auto updateFlags = MaterialUpdateFlags::None;
    auto updateMaterial = [this, &updateFlags](uint32_t index)
    {
        if (index >= m_materials.size())
        {
            return;
        }

        auto& material = m_materials[index];
        if (!material)
        {
            return;
        }

        auto const flags = material->update(this);
        m_materialsUpdateFlags[index] = flags;
        updateFlags |= flags;
    };

    if (forceUpdate || m_materialUpdates != MaterialUpdateFlags::None)
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_materials.size()); ++i)
        {
            updateMaterial(i);
        }
    }
    else
    {
        for (auto const i : m_dynamicMaterialIndices)
        {
            updateMaterial(i);
        }
    }

    updateFlags |= m_materialUpdates;
    m_materialUpdates = MaterialUpdateFlags::None;

    if (m_materialParamDataDirty)
    {
        auto uploadStructured = [this](size_t elementSize, size_t elementCount, void const* data) -> core::ref<Buffer>
        {
            if (elementCount == 0)
            {
                return nullptr;
            }

            return core::make_ref<Buffer>(
                m_device,
                static_cast<uint32_t>(elementSize),
                static_cast<uint32_t>(elementCount),
                BufferUsage::ShaderResource,
                MemoryType::Upload,
                data,
                false
            );
        };

        m_materialParamLayoutBuffer = uploadStructured(
            sizeof(MaterialParamLayoutEntry),
            m_materialParamLayoutEntries.size(),
            m_materialParamLayoutEntries.empty() ? nullptr : m_materialParamLayoutEntries.data()
        );

        m_serializedMaterialParamsBuffer = uploadStructured(
            sizeof(SerializedMaterialParam),
            m_serializedMaterialParams.size(),
            m_serializedMaterialParams.empty() ? nullptr : m_serializedMaterialParams.data()
        );

        if (!m_serializedMaterialParamData.empty())
        {
            m_serializedMaterialParamDataBuffer = core::make_ref<Buffer>(
                m_device,
                1,
                static_cast<uint32_t>(m_serializedMaterialParamData.size()),
                BufferUsage::ShaderResource,
                MemoryType::Upload,
                m_serializedMaterialParamData.data(),
                false
            );
        }
        else
        {
            m_serializedMaterialParamDataBuffer = nullptr;
        }

        m_materialParamDataDirty = false;
    }

    if (!m_materials.empty())
    {
        ensureBufferCapacity(static_cast<uint32_t>(m_materials.size()));
    }

    if (forceUpdate || hasFlag(updateFlags, MaterialUpdateFlags::DataChanged) || m_dirty)
    {
        for (uint32_t i = 0; i < m_materials.size(); ++i)
        {
            if (forceUpdate || hasFlag(m_materialsUpdateFlags[i], MaterialUpdateFlags::DataChanged) || m_dirty)
            {
                uploadMaterial(i);
            }
        }
        m_dirty = false;
    }

    return updateFlags;
}

auto MaterialSystem::bindShaderData(ShaderVariable const& var) const -> void
{
    if (!var.isValid())
    {
        return;
    }

    auto bindingVar = var;
    if (auto parameterBlock = var.getParameterBlock())
    {
        auto const byteSize = parameterBlock->getElementSize();
        if (!m_materialsBindingBlock || m_materialsBindingBlockByteSize != byteSize)
        {
            m_materialsBindingBlock = ParameterBlock::create(m_device, parameterBlock->getReflection());
            m_materialsBindingBlockByteSize = byteSize;
        }

        bindingVar = m_materialsBindingBlock->getRootVariable();
    }

    if (bindingVar.hasMember("materialCount"))
    {
        bindingVar["materialCount"].set(getMaterialCount());
    }

    if (m_materialDataBuffer && bindingVar.hasMember("materialData"))
    {
        bindingVar["materialData"].setBuffer(m_materialDataBuffer);
    }

    if (bindingVar.hasMember("materialSamplers"))
    {
        for (uint32_t i = 0; i < getSamplerDescriptorCount(); ++i)
        {
            auto sampler = getSamplerDescriptorResource(i);
            if (!sampler)
            {
                sampler = m_defaultTextureSampler;
            }
            bindingVar["materialSamplers"][i].setSampler(sampler);
        }
    }

    if (bindingVar.hasMember("materialTextures"))
    {
        for (uint32_t i = 0; i < getTextureDescriptorCount(); ++i)
        {
            if (auto texture = getTextureDescriptorResource(i))
            {
                bindingVar["materialTextures"][i].setTexture(texture);
            }
        }
    }

    if (bindingVar.hasMember("materialBuffers"))
    {
        for (uint32_t i = 0; i < getBufferDescriptorCount(); ++i)
        {
            if (auto buffer = getBufferDescriptorResource(i))
            {
                bindingVar["materialBuffers"][i].setBuffer(buffer);
            }
        }
    }

    if (bindingVar.hasMember("materialTextures3D"))
    {
        for (uint32_t i = 0; i < getTexture3DDescriptorCount(); ++i)
        {
            if (auto texture = getTexture3DDescriptorResource(i))
            {
                bindingVar["materialTextures3D"][i].setTexture(texture);
            }
        }
    }

    if (bindingVar.hasMember("materialParamLayoutEntryCount"))
    {
        bindingVar["materialParamLayoutEntryCount"].set(static_cast<uint32_t>(m_materialParamLayoutEntries.size()));
    }

    if (bindingVar.hasMember("serializedMaterialParamCount"))
    {
        bindingVar["serializedMaterialParamCount"].set(static_cast<uint32_t>(m_serializedMaterialParams.size()));
    }

    if (bindingVar.hasMember("materialParamLayoutEntries") && m_materialParamLayoutBuffer)
    {
        bindingVar["materialParamLayoutEntries"].setBuffer(m_materialParamLayoutBuffer);
    }

    if (bindingVar.hasMember("serializedMaterialParamEntries") && m_serializedMaterialParamsBuffer)
    {
        bindingVar["serializedMaterialParamEntries"].setBuffer(m_serializedMaterialParamsBuffer);
    }

    if (bindingVar.hasMember("serializedMaterialParamData") && m_serializedMaterialParamDataBuffer)
    {
        bindingVar["serializedMaterialParamData"].setBuffer(m_serializedMaterialParamDataBuffer);
    }

    if (bindingVar.hasMember("udimIndirection") && m_udimIndirectionEnabled && m_udimIndirectionBuffer)
    {
        bindingVar["udimIndirection"].setBuffer(m_udimIndirectionBuffer);
    }

    if (m_materialsBindingBlock)
    {
        var.setParameterBlock(m_materialsBindingBlock);
    }
}

auto MaterialSystem::getStats() const -> MaterialStats
{
    auto stats = MaterialStats{};

    stats.materialCount = static_cast<uint64_t>(m_materials.size());
    stats.materialMemoryInBytes = static_cast<uint64_t>(m_materials.size()) * sizeof(generated::MaterialDataBlob);
    stats.materialTypeCount = static_cast<uint64_t>(m_typeConformancesByType.size());

    for (auto const& material : m_materials)
    {
        if (!material)
        {
            continue;
        }

        auto const alphaMasked = (material->getFlags() & static_cast<uint32_t>(generated::MaterialFlags::AlphaTested)) != 0;
        if (!alphaMasked)
        {
            ++stats.materialOpaqueCount;
        }
    }

    stats.textureCount = static_cast<uint64_t>(m_textureManager.size() > 0 ? m_textureManager.size() - 1 : 0);
    m_textureManager.forEach([&](core::ref<Texture> const& texture)
    {
        if (!texture)
        {
            return;
        }

        auto const format = texture->getFormat();
        if (isCompressedFormat(format))
        {
            ++stats.textureCompressedCount;
        }

        auto const channels = getChannelMask(format);
        auto const channelBits = getNumChannelBits(format, channels);
        auto const texels = static_cast<uint64_t>(texture->getWidth()) * texture->getHeight() * texture->getDepth();
        stats.textureTexelCount += texels;
        stats.textureTexelChannelCount += texels * channelBits;
    });

    return stats;
}

auto MaterialSystem::getMaterialDataBuffer() const -> core::ref<Buffer>
{
    return m_materialDataBuffer;
}

auto MaterialSystem::getTypeConformances() const -> TypeConformanceList
{
    TypeConformanceList conformances;
    for (auto const& [type, list] : m_typeConformancesByType)
    {
        (void)type;
        conformances.add(list);
    }

    // Falcor-style phase-function conformances used by volumetric paths.
    conformances.add("IsotropicPhaseFunction", "IPhaseFunction");
    conformances.add("HenyeyGreensteinPhaseFunction", "IPhaseFunction");

    return conformances;
}

auto MaterialSystem::getTypeConformances(generated::MaterialType type) const -> TypeConformanceList
{
    if (auto const it = m_typeConformancesByType.find(type); it != m_typeConformancesByType.end())
    {
        return it->second;
    }

    AP_ERROR("No type conformances for material type '{}'.", static_cast<uint32_t>(type));
    return {};
}

auto MaterialSystem::getShaderModules() const -> ProgramDesc::ShaderModuleList
{
    return m_shaderModules;
}

auto MaterialSystem::getShaderModules(ProgramDesc::ShaderModuleList& modules) const -> void
{
    auto materialModules = getShaderModules();
    modules.insert(modules.end(), materialModules.begin(), materialModules.end());
}

auto MaterialSystem::registerTextureDescriptor(core::ref<Texture> texture) -> DescriptorHandle
{
    auto const handle = m_textureManager.registerTexture(texture, kMaxTextureCount);
    if (handle == kInvalidDescriptorHandle && texture)
    {
        AP_ERROR("MaterialSystem texture descriptor overflow.");
    }
    return handle;
}

auto MaterialSystem::registerSamplerDescriptor(core::ref<Sampler> sampler) -> DescriptorHandle
{
    if (!sampler)
    {
        return kInvalidDescriptorHandle;
    }

    auto const iter = m_samplerDescriptorIndices.find(sampler.get());
    if (iter != m_samplerDescriptorIndices.end())
    {
        return iter->second;
    }

    if (m_samplerDescriptors.size() >= kMaxSamplerCount)
    {
        AP_ERROR("MaterialSystem sampler descriptor overflow.");
        return kInvalidDescriptorHandle;
    }

    auto const handle = static_cast<DescriptorHandle>(m_samplerDescriptors.size());
    m_samplerDescriptors.push_back(sampler);
    m_samplerDescriptorIndices[sampler.get()] = handle;
    return handle;
}

auto MaterialSystem::registerBufferDescriptor(core::ref<Buffer> buffer) -> DescriptorHandle
{
    if (!buffer)
    {
        return kInvalidDescriptorHandle;
    }

    auto const iter = m_bufferDescriptorIndices.find(buffer.get());
    if (iter != m_bufferDescriptorIndices.end())
    {
        return iter->second;
    }

    auto const handle = static_cast<DescriptorHandle>(m_bufferDescriptors.size());
    m_bufferDescriptors.push_back(buffer);
    m_bufferDescriptorIndices[buffer.get()] = handle;
    return handle;
}

auto MaterialSystem::registerTexture3DDescriptor(core::ref<Texture> texture) -> DescriptorHandle
{
    return m_texture3DManager.registerTexture(texture, kMaxTextureCount);
}

auto MaterialSystem::addTexture(core::ref<Texture> texture) -> DescriptorHandle
{
    auto const handle = registerTextureDescriptor(texture);
    if (handle != kInvalidDescriptorHandle)
    {
        m_materialUpdates |= MaterialUpdateFlags::ResourcesChanged;
    }
    return handle;
}

auto MaterialSystem::enqueueDeferredTextureLoad(MaterialTextureManager::DeferredTextureLoader loader) -> void
{
    m_textureManager.enqueueDeferred(std::move(loader));
}

auto MaterialSystem::enqueueDeferredTexture3DLoad(MaterialTextureManager::DeferredTextureLoader loader) -> void
{
    m_texture3DManager.enqueueDeferred(std::move(loader));
}

auto MaterialSystem::replaceTexture(DescriptorHandle handle, core::ref<Texture> texture) -> bool
{
    if (!m_textureManager.replaceTexture(handle, texture))
    {
        return false;
    }

    m_materialUpdates |= MaterialUpdateFlags::ResourcesChanged;
    return true;
}

auto MaterialSystem::addSampler(core::ref<Sampler> sampler) -> DescriptorHandle
{
    auto const handle = registerSamplerDescriptor(sampler);
    if (handle != kInvalidDescriptorHandle)
    {
        m_materialUpdates |= MaterialUpdateFlags::ResourcesChanged;
    }
    return handle;
}

auto MaterialSystem::replaceSampler(DescriptorHandle handle, core::ref<Sampler> sampler) -> bool
{
    if (handle == kInvalidDescriptorHandle || handle >= m_samplerDescriptors.size() || !sampler)
    {
        return false;
    }

    m_samplerDescriptors[handle] = sampler;
    m_samplerDescriptorIndices[sampler.get()] = handle;
    m_materialUpdates |= MaterialUpdateFlags::ResourcesChanged;
    return true;
}

auto MaterialSystem::addBuffer(core::ref<Buffer> buffer) -> DescriptorHandle
{
    auto const handle = registerBufferDescriptor(buffer);
    if (handle != kInvalidDescriptorHandle)
    {
        m_materialUpdates |= MaterialUpdateFlags::ResourcesChanged;
    }
    return handle;
}

auto MaterialSystem::replaceBuffer(DescriptorHandle handle, core::ref<Buffer> buffer) -> bool
{
    if (handle == kInvalidDescriptorHandle || handle >= m_bufferDescriptors.size() || !buffer)
    {
        return false;
    }

    m_bufferDescriptors[handle] = buffer;
    m_bufferDescriptorIndices[buffer.get()] = handle;
    m_materialUpdates |= MaterialUpdateFlags::ResourcesChanged;
    return true;
}

auto MaterialSystem::addTexture3D(core::ref<Texture> texture) -> DescriptorHandle
{
    auto const handle = registerTexture3DDescriptor(texture);
    if (handle != kInvalidDescriptorHandle)
    {
        m_materialUpdates |= MaterialUpdateFlags::ResourcesChanged;
    }
    return handle;
}

auto MaterialSystem::replaceTexture3D(DescriptorHandle handle, core::ref<Texture> texture) -> bool
{
    if (!m_texture3DManager.replaceTexture(handle, texture))
    {
        return false;
    }

    m_materialUpdates |= MaterialUpdateFlags::ResourcesChanged;
    return true;
}

auto MaterialSystem::getTextureDescriptorResource(DescriptorHandle handle) const -> core::ref<Texture>
{
    return m_textureManager.getTexture(handle);
}

auto MaterialSystem::getSamplerDescriptorResource(DescriptorHandle handle) const -> core::ref<Sampler>
{
    if (handle == kInvalidDescriptorHandle || handle >= m_samplerDescriptors.size())
    {
        return nullptr;
    }

    return m_samplerDescriptors[handle];
}

auto MaterialSystem::getBufferDescriptorResource(DescriptorHandle handle) const -> core::ref<Buffer>
{
    if (handle == kInvalidDescriptorHandle || handle >= m_bufferDescriptors.size())
    {
        return nullptr;
    }

    return m_bufferDescriptors[handle];
}

auto MaterialSystem::getTexture3DDescriptorResource(DescriptorHandle handle) const -> core::ref<Texture>
{
    return m_texture3DManager.getTexture(handle);
}

auto MaterialSystem::getMaterialTypeRegistry() const -> MaterialTypeRegistry const&
{
    return m_materialTypeRegistry;
}

auto MaterialSystem::getMaterialTypeId(uint32_t materialIndex) const -> uint32_t
{
    if (materialIndex >= m_cpuMaterialData.size())
    {
        return MaterialTypeRegistry::kInvalidMaterialTypeId;
    }

    return static_cast<uint32_t>(m_cpuMaterialData[materialIndex].header.getMaterialType());
}

auto MaterialSystem::setMaterialParamLayout(std::vector<MaterialParamLayoutEntry> entries) -> void
{
    m_materialParamLayoutEntries = std::move(entries);
    m_useExternalParamData = true;
    m_materialParamDataDirty = true;
}

auto MaterialSystem::setSerializedMaterialParams(std::vector<SerializedMaterialParam> params, std::vector<uint8_t> rawData) -> void
{
    m_serializedMaterialParams = std::move(params);
    m_serializedMaterialParamData = std::move(rawData);
    m_useExternalParamData = true;
    m_materialParamDataDirty = true;
}

auto MaterialSystem::setUdimIndirectionBuffer(core::ref<Buffer> buffer) -> void
{
    m_udimIndirectionBuffer = std::move(buffer);
    m_materialUpdates |= MaterialUpdateFlags::ResourcesChanged;
}

auto MaterialSystem::setUdimIndirectionEnabled(bool enabled) -> void
{
    m_udimIndirectionEnabled = enabled;
    m_materialUpdates |= MaterialUpdateFlags::ResourcesChanged;
}

auto MaterialSystem::setLightProfileEnabled(bool enabled) -> void
{
    m_lightProfileEnabled = enabled;
    m_materialUpdates |= MaterialUpdateFlags::ResourcesChanged;
}

auto MaterialSystem::removeDuplicateMaterials() -> uint32_t
{
    auto removed = uint32_t{0};
    auto canonicalByBlob = std::unordered_map<size_t, uint32_t>{};

    for (uint32_t i = 0; i < static_cast<uint32_t>(m_materials.size()); ++i)
    {
        auto const& material = m_materials[i];
        if (!material)
        {
            continue;
        }

        auto const blob = material->getDataBlob();
        auto const hash = core::hash(
            static_cast<uint32_t>(material->getType()),
            std::string_view(reinterpret_cast<char const*>(&blob), sizeof(blob))
        );

        if (auto const it = canonicalByBlob.find(hash); it != canonicalByBlob.end())
        {
            m_materials[i] = m_materials[it->second];
            ++removed;
            continue;
        }

        canonicalByBlob.emplace(hash, i);
    }

    if (removed > 0)
    {
        m_materialIndices.clear();
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_materials.size()); ++i)
        {
            if (m_materials[i])
            {
                m_materialIndices[m_materials[i].get()] = i;
            }
        }
        m_materialsChanged = true;
        m_dirty = true;
    }

    return removed;
}

auto MaterialSystem::optimizeMaterials() -> uint32_t
{
    auto optimized = removeDuplicateMaterials();

    auto tryReadConstantRGBA = [](core::ref<Texture> const& texture, float4& outColor) -> bool
    {
        if (!texture)
        {
            return false;
        }

        if (texture->getWidth() != 1 || texture->getHeight() != 1 || texture->getDepth() != 1)
        {
            return false;
        }

        auto const format = texture->getFormat();
        if (format == ResourceFormat::RGBA8Unorm || format == ResourceFormat::RGBA8UnormSrgb)
        {
            std::array<uint8_t, 4> pixel{};
            texture->getSubresourceBlob(0, pixel.data(), pixel.size());
            outColor = float4(
                static_cast<float>(pixel[0]) / 255.0f,
                static_cast<float>(pixel[1]) / 255.0f,
                static_cast<float>(pixel[2]) / 255.0f,
                static_cast<float>(pixel[3]) / 255.0f
            );
            return true;
        }

        if (format == ResourceFormat::BGRA8Unorm || format == ResourceFormat::BGRA8UnormSrgb)
        {
            std::array<uint8_t, 4> pixel{};
            texture->getSubresourceBlob(0, pixel.data(), pixel.size());
            outColor = float4(
                static_cast<float>(pixel[2]) / 255.0f,
                static_cast<float>(pixel[1]) / 255.0f,
                static_cast<float>(pixel[0]) / 255.0f,
                static_cast<float>(pixel[3]) / 255.0f
            );
            return true;
        }

        if (format == ResourceFormat::RGBA32Float)
        {
            std::array<float, 4> pixel{};
            texture->getSubresourceBlob(0, pixel.data(), sizeof(pixel));
            outColor = float4(pixel[0], pixel[1], pixel[2], pixel[3]);
            return true;
        }

        return false;
    };

    for (auto const& material : m_materials)
    {
        auto basicMaterial = core::dynamic_ref_cast<BasicMaterial>(material);
        if (!basicMaterial)
        {
            continue;
        }

        if (length(basicMaterial->emissive) <= 0.0f && basicMaterial->emissiveTexture)
        {
            basicMaterial->emissiveTexture = nullptr;
            ++optimized;
        }
        else if (basicMaterial->emissiveTexture)
        {
            auto emissiveValue = float4(0.0f);
            if (tryReadConstantRGBA(basicMaterial->emissiveTexture, emissiveValue))
            {
                auto const emissiveRgb = float3(emissiveValue.x, emissiveValue.y, emissiveValue.z);
                if (length(emissiveRgb) <= 0.0f)
                {
                    basicMaterial->emissiveTexture = nullptr;
                    ++optimized;
                }
            }
        }

        if (basicMaterial->specularTransmission <= 0.0f && basicMaterial->diffuseTransmission <= 0.0f && basicMaterial->transmissionTexture)
        {
            basicMaterial->transmissionTexture = nullptr;
            ++optimized;
        }

        if (basicMaterial->alphaMode == generated::AlphaMode::Opaque && basicMaterial->baseColor.w >= 1.0f && basicMaterial->baseColorTexture)
        {
            // Conservative pruning rule: opaque alpha path does not require alpha sampled texture value.
            ++optimized;
        }

        if (auto standardMaterial = core::dynamic_ref_cast<StandardMaterial>(material))
        {
            if (standardMaterial->normalScale == 0.0f && standardMaterial->normalTexture)
            {
                standardMaterial->normalTexture = nullptr;
                ++optimized;
            }
            else if (standardMaterial->normalTexture)
            {
                auto normalValue = float4(0.0f);
                if (tryReadConstantRGBA(standardMaterial->normalTexture, normalValue))
                {
                    auto const epsilon = 1e-3f;
                    if (std::abs(normalValue.x - 0.5f) < epsilon && std::abs(normalValue.y - 0.5f) < epsilon &&
                        std::abs(normalValue.z - 1.0f) < epsilon)
                    {
                        standardMaterial->normalTexture = nullptr;
                        ++optimized;
                    }
                }
            }
        }
    }

    if (optimized > 0)
    {
        m_materialUpdates |= MaterialUpdateFlags::ResourcesChanged;
        m_dirty = true;
    }

    return optimized;
}

auto MaterialSystem::rebuildInternalMaterialParamData() -> void
{
    auto hashName = [](std::string_view s) -> uint32_t
    {
        auto hash = uint32_t{2166136261u};
        for (auto const c : s)
        {
            hash ^= static_cast<uint8_t>(c);
            hash *= 16777619u;
        }
        return hash;
    };

    m_materialParamLayoutEntries.clear();
    m_serializedMaterialParams.clear();
    m_serializedMaterialParamData.clear();

    m_materialParamLayoutEntries.push_back(MaterialParamLayoutEntry{
        hashName("MaterialHeader"),
        0u,
        static_cast<uint32_t>(sizeof(generated::MaterialHeader)),
        0u
    });

    m_materialParamLayoutEntries.push_back(MaterialParamLayoutEntry{
        hashName("MaterialPayload"),
        static_cast<uint32_t>(sizeof(generated::MaterialHeader)),
        static_cast<uint32_t>(sizeof(generated::MaterialPayload)),
        0u
    });

    auto const blobName = hashName("material_blob");
    for (auto const& material : m_materials)
    {
        if (!material)
        {
            continue;
        }

        auto const blob = material->getDataBlob();
        auto const offset = static_cast<uint32_t>(m_serializedMaterialParamData.size());
        auto const* bytes = reinterpret_cast<uint8_t const*>(&blob);
        m_serializedMaterialParamData.insert(
            m_serializedMaterialParamData.end(),
            bytes,
            bytes + sizeof(generated::MaterialDataBlob)
        );

        m_serializedMaterialParams.push_back(SerializedMaterialParam{
            blobName,
            0u,
            offset,
            static_cast<uint32_t>(sizeof(generated::MaterialDataBlob))
        });
    }

    m_materialParamDataDirty = true;
}

auto MaterialSystem::ensureBufferCapacity(uint32_t requiredCount) -> void
{
    if (requiredCount == 0)
    {
        return;
    }

    auto const requiredSize = requiredCount * sizeof(generated::MaterialDataBlob);
    if (m_materialDataBuffer && m_materialDataBuffer->getSize() >= requiredSize)
    {
        return;
    }

    auto newCount = m_materialDataBuffer
        ? static_cast<uint32_t>(m_materialDataBuffer->getSize() / sizeof(generated::MaterialDataBlob))
        : kInitialBufferCapacity;

    while (newCount < requiredCount)
    {
        newCount *= 2;
    }

    auto const usage = BufferUsage::ShaderResource;
    m_materialDataBuffer = core::make_ref<Buffer>(
        m_device,
        static_cast<uint32_t>(sizeof(generated::MaterialDataBlob)),
        newCount,
        usage,
        MemoryType::Upload,
        nullptr,
        false
    );
}

auto MaterialSystem::updateMetadata() -> void
{
    m_typeConformancesByType.clear();
    m_shaderModules.clear();
    m_dynamicMaterialIndices.clear();

    size_t maxTextures = 1;
    size_t maxBuffers = 1;
    size_t maxTexture3D = 1;
    for (size_t materialIndex = 0; materialIndex < m_materials.size(); ++materialIndex)
    {
        auto const& material = m_materials[materialIndex];
        if (!material)
        {
            continue;
        }

        if (material->isDynamic())
        {
            m_dynamicMaterialIndices.push_back(static_cast<uint32_t>(materialIndex));
        }

        m_typeConformancesByType[material->getType()] = material->getTypeConformances();

        auto modules = material->getShaderModules();
        m_shaderModules.insert(m_shaderModules.end(), modules.begin(), modules.end());

        maxTextures += material->getMaxTextureCount();
        maxBuffers += material->getMaxBufferCount();
        maxTexture3D += material->getMaxTexture3DCount();
    }

    m_textureDescCount = static_cast<uint32_t>(std::min(maxTextures, kMaxTextureCount));
    m_bufferDescCount = static_cast<uint32_t>(maxBuffers);
    m_texture3DDescCount = static_cast<uint32_t>(maxTexture3D);
}

auto MaterialSystem::uploadMaterial(uint32_t materialId) -> void
{
    if (materialId >= m_materials.size())
    {
        return;
    }

    auto const& material = m_materials[materialId];
    if (!material)
    {
        if (materialId < m_cpuMaterialData.size())
        {
            m_cpuMaterialData[materialId] = {};
        }
        return;
    }

    if (auto basicMaterial = core::dynamic_ref_cast<BasicMaterial>(material))
    {
        basicMaterial->setDescriptorHandles(
            registerTextureDescriptor(basicMaterial->baseColorTexture),
            registerTextureDescriptor(basicMaterial->metallicRoughnessTexture),
            registerTextureDescriptor(basicMaterial->normalTexture),
            registerTextureDescriptor(basicMaterial->emissiveTexture),
            registerTextureDescriptor(basicMaterial->transmissionTexture),
            registerTextureDescriptor(basicMaterial->displacementTexture),
            registerSamplerDescriptor(material->getDefaultTextureSampler())
        );
    }

    auto const blob = material->getDataBlob();

    if (materialId >= m_cpuMaterialData.size())
    {
        m_cpuMaterialData.resize(materialId + 1);
    }

    auto& data = m_cpuMaterialData[materialId];
    data = blob;
    data.header.setMaterialType(material->getType());

    if (m_materialDataBuffer)
    {
        auto const offset = static_cast<size_t>(materialId) * sizeof(generated::MaterialDataBlob);
        m_materialDataBuffer->setBlob(&data, offset, sizeof(generated::MaterialDataBlob));
    }
}

} // namespace april::graphics
