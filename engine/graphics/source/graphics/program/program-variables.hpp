// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "program.hpp"
#include "program-reflection.hpp"
#include "rt-binding-table.hpp"
#include "rhi/fwd.hpp"
#include "rhi/parameter-block.hpp"
#include "rhi/shader-table.hpp"

#include <core/foundation/object.hpp>
#include <slang-rhi.h>

namespace april::graphics
{
    class Device;

    class ProgramVariables : public ParameterBlock
    {
    public:
        static auto create(core::ref<Device> pDevice, core::ref<ProgramReflection const> const& pReflector) -> core::ref<ProgramVariables>;
        static auto create(core::ref<Device> pDevice, Program const* pProgram) -> core::ref<ProgramVariables>;

        auto getReflection() const -> core::ref<ProgramReflection const> { return m_reflector; }

    protected:
        ProgramVariables(core::ref<Device> pDevice, core::ref<ProgramReflection const> const& pReflector);

        core::ref<ProgramReflection const> m_reflector{};
    };

    class RayTracingPipeline;

    /**
     * This class manages a raytracing program's reflection and variable assignment.
     */
    class RtProgramVariables : public ProgramVariables
    {
    public:
        static auto create(core::ref<Device> pDevice, core::ref<Program> const& pProgram, core::ref<RayTracingBindingTable> const& pBindingTable) -> core::ref<RtProgramVariables>;

        auto prepareShaderTable(CommandContext* pCtx, RayTracingPipeline* pRtso) -> bool;

        auto getShaderTable() const -> rhi::IShaderTable* { return m_gfxShaderTable; }
        auto getMissVarsCount() const -> uint32_t { return uint32_t(m_missVars.size()); }
        auto getTotalHitVarsCount() const -> uint32_t { return uint32_t(m_hitVars.size()); }
        auto getRayTypeCount() const -> uint32_t { return m_rayTypeCount; }
        auto getGeometryCount() const -> uint32_t { return m_geometryCount; }

        auto getUniqueEntryPointGroupIndices() const -> std::vector<int32_t> const& { return m_uniqueEntryPointGroupIndices; }

    private:
        struct EntryPointGroupInfo
        {
            int32_t entryPointGroupIndex = -1;
        };

        using VarsVector = std::vector<EntryPointGroupInfo>;

        RtProgramVariables(core::ref<Device> pDevice, core::ref<Program> const& pProgram, core::ref<RayTracingBindingTable> const& pBindingTable);

        void init(core::ref<RayTracingBindingTable> const& pBindingTable);

        uint32_t m_rayTypeCount{};                          ///< Number of ray types (= number of hit groups per geometry).
        uint32_t m_geometryCount{};                         ///< Number of geometries.
        std::vector<int32_t> m_uniqueEntryPointGroupIndices{};  ///< Indices of all unique entry point groups that we use in the associated program.

        mutable rhi::ComPtr<rhi::IShaderTable> m_gfxShaderTable{nullptr};      ///< GPU shader table.
        mutable RayTracingPipeline* mp_currentRayTracingPipeline{};   ///< The RayTracingPipeline used to create the current shader table.

        VarsVector m_rayGenVars{};
        VarsVector m_missVars{};
        VarsVector m_hitVars{};
    };
}
