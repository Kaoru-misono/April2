// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "rt-binding-table.hpp"

#include <core/log/logger.hpp>
#include <core/error/assert.hpp>
#include <limits>

namespace april::graphics
{
    namespace
    {
        // Define API limitations.
        // See https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html
        const uint32_t kMaxMissCount = (1 << 16);
        const uint32_t kMaxRayTypeCount = (1 << 4);
    } // namespace

    auto RayTracingBindingTable::create(uint32_t missCount, uint32_t rayTypeCount, uint32_t geometryCount) -> core::ref<RayTracingBindingTable>
    {
        return core::ref<RayTracingBindingTable>(new RayTracingBindingTable(missCount, rayTypeCount, geometryCount));
    }

    RayTracingBindingTable::RayTracingBindingTable(uint32_t missCount, uint32_t rayTypeCount, uint32_t geometryCount)
        : m_missCount(missCount), m_rayTypeCount(rayTypeCount), m_geometryCount(geometryCount)
    {
        AP_ASSERT(missCount <= kMaxMissCount, "'missCount' exceeds the maximum supported ({})", kMaxMissCount);
        AP_ASSERT(rayTypeCount <= kMaxRayTypeCount, "'rayTypeCount' exceeds the maximum supported ({})", kMaxRayTypeCount);
        size_t recordCount = 1ull + missCount + rayTypeCount * geometryCount;
        AP_ASSERT(recordCount <= std::numeric_limits<uint32_t>::max(), "Raytracing binding table is too large");

        // Create the binding table. All entries will be assigned a null shader initially.
        m_shaderTable.resize(recordCount);
    }

    auto RayTracingBindingTable::setRayGeneration(ShaderID shaderID) -> void
    {
        m_shaderTable[0] = shaderID;
    }

    auto RayTracingBindingTable::setMiss(uint32_t missIndex, ShaderID shaderID) -> void
    {
        AP_ASSERT(missIndex < m_missCount, "'missIndex' is out of range");
        m_shaderTable[getMissOffset(missIndex)] = shaderID;
    }

    auto RayTracingBindingTable::setHitGroup(uint32_t rayType, uint32_t geometryID, ShaderID shaderID) -> void
    {
        AP_ASSERT(rayType < m_rayTypeCount, "'rayType' is out of range");
        AP_ASSERT(geometryID < m_geometryCount, "'geometryID' is out of range");
        m_shaderTable[getHitGroupOffset(rayType, geometryID)] = shaderID;
    }

    auto RayTracingBindingTable::setHitGroup(uint32_t rayType, std::vector<uint32_t> const& geometryIDs, ShaderID shaderID) -> void
    {
        for (uint32_t geometryID : geometryIDs)
        {
            setHitGroup(rayType, geometryID, shaderID);
        }
    }

    auto RayTracingBindingTable::getMiss(uint32_t missIndex) const -> ShaderID
    {
        AP_ASSERT(missIndex < m_missCount, "'missIndex' is out of range");
        return m_shaderTable[getMissOffset(missIndex)];
    }

    auto RayTracingBindingTable::getHitGroup(uint32_t rayType, uint32_t geometryID) const -> ShaderID
    {
        AP_ASSERT(rayType < m_rayTypeCount, "'rayType' is out of range");
        AP_ASSERT(geometryID < m_geometryCount, "'geometryID' is out of range");
        return m_shaderTable[getHitGroupOffset(rayType, geometryID)];
    }

    auto RayTracingBindingTable::getMissOffset(uint32_t missIndex) const -> uint32_t
    {
        return 1 + missIndex;
    }

    auto RayTracingBindingTable::getHitGroupOffset(uint32_t rayType, uint32_t geometryID) const -> uint32_t
    {
        return 1 + m_missCount + (geometryID * m_rayTypeCount + rayType);
    }

} // namespace april::graphics
