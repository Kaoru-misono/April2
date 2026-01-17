// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "program.hpp"

#include <core/foundation/object.hpp>
#include <vector>

namespace april::graphics
{
    /**
     * This class describes the binding of ray tracing shaders for raygen/miss/hits.
     */
    class RayTracingBindingTable : public core::Object
    {
        APRIL_OBJECT(RayTracingBindingTable)
    public:
        using ShaderID = ProgramDesc::ShaderID;

        static auto create(uint32_t missCount, uint32_t rayTypeCount, uint32_t geometryCount) -> core::ref<RayTracingBindingTable>;

        auto setRayGeneration(ShaderID shaderID) -> void;
        auto setMiss(uint32_t missIndex, ShaderID shaderID) -> void;
        auto setHitGroup(uint32_t rayType, uint32_t geometryID, ShaderID shaderID) -> void;
        // using GlobalGeometryID = ObjectID<SceneObjectKind, SceneObjectKind::kGlobalGeometry, uint32_t>;
        // auto setHitGroup(uint32_t rayType, GlobalGeometryID geometryID, ShaderID shaderID) -> void
        // {
        //     setHitGroup(rayType, geometryID.get(), std::move(shaderID));
        // }
        auto setHitGroup(uint32_t rayType, std::vector<uint32_t> const& geometryIDs, ShaderID shaderID) -> void;
        // auto setHitGroup(uint32_t rayType, const std::vector<GlobalGeometryID>& geometryIDs, ShaderID shaderID) -> void;

        auto getRayGeneration() const -> ShaderID { return m_shaderTable[0]; }
        auto getMiss(uint32_t missIndex) const -> ShaderID;
        auto getHitGroup(uint32_t rayType, uint32_t geometryID) const -> ShaderID;

        auto getMissCount() const -> uint32_t { return m_missCount; }
        auto getRayTypeCount() const -> uint32_t { return m_rayTypeCount; }
        auto getGeometryCount() const -> uint32_t { return m_geometryCount; }

    protected:
        RayTracingBindingTable(uint32_t missCount, uint32_t rayTypeCount, uint32_t geometryCount);

        auto getMissOffset(uint32_t missIndex) const -> uint32_t;
        auto getHitGroupOffset(uint32_t rayType, uint32_t geometryID) const -> uint32_t;

        uint32_t m_missCount{0};
        uint32_t m_rayTypeCount{0};
        uint32_t m_geometryCount{0};

        std::vector<ShaderID> m_shaderTable{};
    };
}
