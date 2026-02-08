// MaterialSystem implementation.

#include "material-system.hpp"
#include "program/shader-variable.hpp"
#include "rhi/render-device.hpp"

#include <core/log/logger.hpp>

namespace april::graphics
{

MaterialSystem::MaterialSystem(core::ref<Device> device)
    : m_device(std::move(device))
{
    m_materials.reserve(kInitialBufferCapacity);
    m_cpuMaterialData.reserve(kInitialBufferCapacity);
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
    if (!m_dirty)
        return;

    rebuildMaterialData();

    if (m_cpuMaterialData.empty())
    {
        m_dirty = false;
        return;
    }

    ensureBufferCapacity(static_cast<uint32_t>(m_cpuMaterialData.size()));

    // Upload data to GPU buffer
    if (m_materialDataBuffer)
    {
        auto const dataSize = m_cpuMaterialData.size() * sizeof(generated::StandardMaterialData);
        m_materialDataBuffer->setBlob(m_cpuMaterialData.data(), 0, dataSize);
    }

    m_dirty = false;
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
    auto const usage = BufferUsage::ShaderResource | BufferUsage::CopyDestination;
    m_materialDataBuffer = core::make_ref<Buffer>(
        m_device,
        static_cast<uint32_t>(sizeof(generated::StandardMaterialData)),
        newCount,
        usage,
        MemoryType::DeviceLocal,
        nullptr,
        false  // no UAV counter
    );

    AP_INFO("MaterialSystem: Resized material buffer to {} materials ({} bytes)", newCount, newSize);
}

auto MaterialSystem::rebuildMaterialData() -> void
{
    m_cpuMaterialData.resize(m_materials.size());

    for (size_t i = 0; i < m_materials.size(); ++i)
    {
        auto const& material = m_materials[i];
        if (material)
        {
            material->writeData(m_cpuMaterialData[i]);
        }
        else
        {
            // Clear data for removed materials
            m_cpuMaterialData[i] = {};
        }
    }
}

} // namespace april::graphics
