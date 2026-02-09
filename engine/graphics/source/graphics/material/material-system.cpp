// MaterialSystem implementation.

#include "material-system.hpp"
#include "basic-material.hpp"
#include "standard-material.hpp"
#include "unlit-material.hpp"
#include "program/shader-variable.hpp"
#include "rhi/render-device.hpp"

#include <core/log/logger.hpp>
#include <core/error/assert.hpp>
#include <format>

namespace april::graphics
{

MaterialSystem::MaterialSystem(core::ref<Device> device, MaterialSystemConfig config)
    : m_device(std::move(device))
    , m_config(config)
{
    m_materials.reserve(kInitialBufferCapacity);
    m_cpuMaterialData.reserve(kInitialBufferCapacity);

    m_textureDescriptors.emplace_back();
    m_samplerDescriptors.emplace_back();
    m_bufferDescriptors.emplace_back();

    m_materialTypeRegistry.registerBuiltIn(
        "Standard",
        static_cast<uint32_t>(generated::MaterialType::Standard)
    );
    m_materialTypeRegistry.registerBuiltIn(
        "Unlit",
        static_cast<uint32_t>(generated::MaterialType::Unlit)
    );

    AP_INFO("MaterialSystem: initialized with texture={}, sampler={}, buffer={} table capacities",
        m_config.textureTableSize, m_config.samplerTableSize, m_config.bufferTableSize);
}

auto MaterialSystem::getShaderDefines() const -> DefineList
{
    auto defines = DefineList{};
    defines["MATERIAL_TEXTURE_TABLE_SIZE"] = std::to_string(m_config.textureTableSize);
    defines["MATERIAL_SAMPLER_TABLE_SIZE"] = std::to_string(m_config.samplerTableSize);
    defines["MATERIAL_BUFFER_TABLE_SIZE"] = std::to_string(m_config.bufferTableSize);
    return defines;
}

auto MaterialSystem::getDiagnostics() const -> MaterialSystemDiagnostics
{
    auto diag = MaterialSystemDiagnostics{};

    diag.totalMaterialCount = static_cast<uint32_t>(m_materials.size());

    for (auto const& material : m_materials)
    {
        if (!material) continue;

        if (core::dynamic_ref_cast<StandardMaterial>(material))
        {
            ++diag.standardMaterialCount;
        }
        else if (core::dynamic_ref_cast<UnlitMaterial>(material))
        {
            ++diag.unlitMaterialCount;
        }
        else
        {
            ++diag.otherMaterialCount;
        }
    }

    diag.textureDescriptorCount = static_cast<uint32_t>(m_textureDescriptors.size());
    diag.textureDescriptorCapacity = m_config.textureTableSize;
    diag.samplerDescriptorCount = static_cast<uint32_t>(m_samplerDescriptors.size());
    diag.samplerDescriptorCapacity = m_config.samplerTableSize;
    diag.bufferDescriptorCount = static_cast<uint32_t>(m_bufferDescriptors.size());
    diag.bufferDescriptorCapacity = m_config.bufferTableSize;

    diag.textureOverflowCount = m_textureOverflowCount;
    diag.samplerOverflowCount = m_samplerOverflowCount;
    diag.bufferOverflowCount = m_bufferOverflowCount;
    diag.invalidHandleCount = m_invalidHandleCount;

    return diag;
}

auto MaterialSystem::validateAndClampDescriptorHandle(
    uint32_t& handle,
    uint32_t maxCount,
    uint32_t& overflowCounter,
    uint32_t materialIndex,
    char const* slotName
) const -> void
{
    if (handle < maxCount)
    {
        return;
    }

    // Debug fail-fast: assert in debug builds to catch issues early.
    AP_ASSERT(false, "Descriptor overflow: handle {} >= capacity {} for slot '{}'", handle, maxCount, slotName);

    // Runtime fallback: clamp to fallback slot 0 and log warning.
    AP_WARN(
        "MaterialSystem: descriptor overflow for material {} slot '{}' (handle={}, capacity={}), using fallback 0",
        materialIndex,
        slotName,
        handle,
        maxCount
    );

    handle = kInvalidDescriptorHandle;
    ++overflowCounter;
    ++m_invalidHandleCount;
}

auto MaterialSystem::addMaterial(core::ref<IMaterial> material) -> uint32_t
{
    if (!material)
    {
        AP_WARN("MaterialSystem::addMaterial: null material provided");
        return UINT32_MAX;
    }

    // Check if material already exists
    auto it = m_materialIndices.find(material.get());
    if (it != m_materialIndices.end())
    {
        return it->second;
    }

    auto const index = static_cast<uint32_t>(m_materials.size());
    m_materials.push_back(material);
    m_materialIndices[material.get()] = index;

    // Add placeholder for CPU data
    m_cpuMaterialData.emplace_back();

    markDirty();
    return index;
}

auto MaterialSystem::removeMaterial(uint32_t index) -> void
{
    if (index >= m_materials.size())
    {
        AP_WARN("MaterialSystem::removeMaterial: invalid index {}", index);
        return;
    }

    auto material = m_materials[index];
    if (material)
    {
        m_materialIndices.erase(material.get());
    }

    // For now, we don't compact the array to maintain indices
    // Instead, we null out the entry
    m_materials[index] = nullptr;

    markDirty();
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

auto MaterialSystem::updateGpuBuffers() -> void
{
    // Check if any materials have pending updates.
    auto anyDirty = m_dirty;
    if (!anyDirty)
    {
        for (auto const& material : m_materials)
        {
            if (material && material->isDirty())
            {
                anyDirty = true;
                break;
            }
        }
    }

    if (!anyDirty)
        return;

    if (m_dirty)
    {
        rebuildMaterialData();

        if (m_cpuMaterialData.empty())
        {
            m_dirty = false;
            return;
        }

        ensureBufferCapacity(static_cast<uint32_t>(m_cpuMaterialData.size()));

        if (m_materialDataBuffer)
        {
            auto const dataSize = m_cpuMaterialData.size() * sizeof(generated::StandardMaterialData);
            m_materialDataBuffer->setBlob(m_cpuMaterialData.data(), 0, dataSize);
        }

        for (auto const& material : m_materials)
        {
            if (material)
            {
                material->clearDirty();
            }
        }

        m_dirty = false;
        return;
    }

    for (uint32_t i = 0; i < m_materials.size(); ++i)
    {
        auto const& material = m_materials[i];
        if (!material || !material->isDirty())
        {
            continue;
        }

        writeMaterialData(i);

        if (m_materialDataBuffer)
        {
            auto const offset = static_cast<size_t>(i) * sizeof(generated::StandardMaterialData);
            m_materialDataBuffer->setBlob(&m_cpuMaterialData[i], offset, sizeof(generated::StandardMaterialData));
        }

        material->clearDirty();
    }
}

auto MaterialSystem::bindToShader(ShaderVariable& var) const -> void
{
    if (m_materialDataBuffer && var.isValid())
    {
        var.setBuffer(m_materialDataBuffer);
    }
}

auto MaterialSystem::getMaterialDataBuffer() const -> core::ref<Buffer>
{
    return m_materialDataBuffer;
}

auto MaterialSystem::getTypeConformances() const -> TypeConformanceList
{
    TypeConformanceList conformances;

    for (auto const& material : m_materials)
    {
        if (material)
        {
            conformances.add(material->getTypeConformances());
        }
    }

    return conformances;
}

auto MaterialSystem::registerTextureDescriptor(core::ref<Texture> texture) -> DescriptorHandle
{
    if (!texture)
    {
        return kInvalidDescriptorHandle;
    }

    auto const iter = m_textureDescriptorIndices.find(texture.get());
    if (iter != m_textureDescriptorIndices.end())
    {
        return iter->second;
    }

    auto const handle = static_cast<DescriptorHandle>(m_textureDescriptors.size());
    m_textureDescriptors.push_back(texture);
    m_textureDescriptorIndices[texture.get()] = handle;
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

auto MaterialSystem::getTextureDescriptorResource(DescriptorHandle handle) const -> core::ref<Texture>
{
    if (handle == kInvalidDescriptorHandle || handle >= m_textureDescriptors.size())
    {
        return nullptr;
    }

    return m_textureDescriptors[handle];
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

    return m_cpuMaterialData[materialIndex].header.materialType;
}

auto MaterialSystem::markDirty() -> void
{
    m_dirty = true;
}

auto MaterialSystem::isDirty() const -> bool
{
    return m_dirty;
}

auto MaterialSystem::ensureBufferCapacity(uint32_t requiredCount) -> void
{
    if (requiredCount == 0)
        return;

    auto const requiredSize = requiredCount * sizeof(generated::StandardMaterialData);

    // Check if we need to resize
    if (m_materialDataBuffer && m_materialDataBuffer->getSize() >= requiredSize)
        return;

    // Calculate new capacity (grow by 2x or to required, whichever is larger)
    auto newCount = m_materialDataBuffer ?
        static_cast<uint32_t>(m_materialDataBuffer->getSize() / sizeof(generated::StandardMaterialData)) :
        kInitialBufferCapacity;

    while (newCount < requiredCount)
        newCount *= 2;

    auto const newSize = newCount * sizeof(generated::StandardMaterialData);

    // Create new buffer
    auto const usage = BufferUsage::ShaderResource;
    m_materialDataBuffer = core::make_ref<Buffer>(
        m_device,
        static_cast<uint32_t>(sizeof(generated::StandardMaterialData)),
        newCount,
        usage,
        MemoryType::Upload,
        nullptr,
        false  // no UAV counter
    );

    AP_INFO("MaterialSystem: Resized material buffer to {} materials ({} bytes)", newCount, newSize);
}

auto MaterialSystem::rebuildMaterialData() -> void
{
    m_cpuMaterialData.resize(m_materials.size());

    m_textureOverflowCount = 0;
    m_samplerOverflowCount = 0;
    m_bufferOverflowCount = 0;
    m_invalidHandleCount = 0;

    for (size_t i = 0; i < m_materials.size(); ++i)
    {
        writeMaterialData(static_cast<uint32_t>(i));
    }
}

auto MaterialSystem::writeMaterialData(uint32_t index) -> void
{
    if (index >= m_materials.size())
    {
        return;
    }

    auto const& material = m_materials[index];
    if (!material)
    {
        if (index < m_cpuMaterialData.size())
        {
            m_cpuMaterialData[index] = {};
        }
        return;
    }

    if (auto basicMaterial = core::dynamic_ref_cast<BasicMaterial>(material))
    {
        basicMaterial->setDescriptorHandles(
            registerTextureDescriptor(basicMaterial->baseColorTexture),
            registerTextureDescriptor(basicMaterial->metallicRoughnessTexture),
            registerTextureDescriptor(basicMaterial->normalTexture),
            registerTextureDescriptor(basicMaterial->occlusionTexture),
            registerTextureDescriptor(basicMaterial->emissiveTexture),
            kInvalidDescriptorHandle,
            kInvalidDescriptorHandle
        );
    }

    material->writeData(m_cpuMaterialData[index]);

    auto& data = m_cpuMaterialData[index];
    data.header.materialType = static_cast<uint32_t>(material->getType());

    validateAndClampDescriptorHandle(data.baseColorTextureHandle, m_config.textureTableSize, m_textureOverflowCount, index, "baseColor");
    validateAndClampDescriptorHandle(data.metallicRoughnessTextureHandle, m_config.textureTableSize, m_textureOverflowCount, index, "metallicRoughness");
    validateAndClampDescriptorHandle(data.normalTextureHandle, m_config.textureTableSize, m_textureOverflowCount, index, "normal");
    validateAndClampDescriptorHandle(data.occlusionTextureHandle, m_config.textureTableSize, m_textureOverflowCount, index, "occlusion");
    validateAndClampDescriptorHandle(data.emissiveTextureHandle, m_config.textureTableSize, m_textureOverflowCount, index, "emissive");
    validateAndClampDescriptorHandle(data.samplerHandle, m_config.samplerTableSize, m_samplerOverflowCount, index, "sampler");
    validateAndClampDescriptorHandle(data.bufferHandle, m_config.bufferTableSize, m_bufferOverflowCount, index, "buffer");
}

} // namespace april::graphics
