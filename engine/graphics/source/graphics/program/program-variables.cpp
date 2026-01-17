// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "program-variables.hpp"
#include "program.hpp"
#include "rt-binding-table.hpp"
#include "rhi/render-device.hpp"
#include "rhi/shader-table.hpp"
#include "rhi/ray-tracing-pipeline.hpp"

#include <core/log/logger.hpp>
#include <core/error/assert.hpp>
#include <set>

namespace april::graphics
{
    ProgramVariables::ProgramVariables(core::ref<Device> device, core::ref<ProgramReflection const> const& reflector)
        : ParameterBlock(device, reflector), m_reflector(reflector)
    {
        AP_ASSERT(reflector, "Reflector is null");
    }

    auto ProgramVariables::create(core::ref<Device> device, core::ref<ProgramReflection const> const& reflector) -> core::ref<ProgramVariables>
    {
        AP_ASSERT(reflector, "Can't create a ProgramVars object without a program reflector");
        return core::ref<ProgramVariables>(new ProgramVariables(device, reflector));
    }

    auto ProgramVariables::create(core::ref<Device> device, Program const* program) -> core::ref<ProgramVariables>
    {
        AP_ASSERT(program, "Can't create a ProgramVars object without a program");
        return create(device, program->getReflector());
    }

    // RtProgramVariables

    RtProgramVariables::RtProgramVariables(core::ref<Device> device, core::ref<Program> const& program, core::ref<RayTracingBindingTable> const& bindingTable)
        : ProgramVariables(device, program->getReflector())
    {
        AP_ASSERT(program, "RtProgramVars must have a raytracing program attached to it");
        // Check if bindingTable has valid rayGen
        AP_ASSERT(bindingTable && bindingTable->getRayGeneration().isValid(), "RtProgramVars must have a raygen program attached to it");

        init(bindingTable);
    }

    auto RtProgramVariables::create(core::ref<Device> device, core::ref<Program> const& program, core::ref<RayTracingBindingTable> const& bindingTable) -> core::ref<RtProgramVariables>
    {
        return core::ref<RtProgramVariables>(new RtProgramVariables(device, program, bindingTable));
    }

    void RtProgramVariables::init(core::ref<RayTracingBindingTable> const& bindingTable)
    {
        m_rayTypeCount = bindingTable->getRayTypeCount();
        m_geometryCount = bindingTable->getGeometryCount();

        AP_ASSERT(mp_programVersion, "RtProgramVars must have a program version");
        auto program = dynamic_cast<Program*>(mp_programVersion->getProgram());
        AP_ASSERT(program, "RtProgramVars must have a program");
        auto reflector = mp_programVersion->getReflector();

        std::set<int32_t> entryPointGroupIndices;

        auto const& rayGenInfo = bindingTable->getRayGeneration();
        AP_ASSERT(rayGenInfo.isValid(), "Raytracing binding table has no shader at raygen index");
        m_rayGenVars.resize(1);
        m_rayGenVars[0].entryPointGroupIndex = rayGenInfo.groupIndex;
        entryPointGroupIndices.insert(rayGenInfo.groupIndex);

        uint32_t missCount = bindingTable->getMissCount();
        m_missVars.resize(missCount);

        for (uint32_t i = 0; i < missCount; ++i)
        {
            auto const& missInfo = bindingTable->getMiss(i);
            if (!missInfo.isValid())
            {
                AP_WARN("Raytracing binding table has no shader at miss index {}. Is that intentional?", i);
                continue;
            }

            m_missVars[i].entryPointGroupIndex = missInfo.groupIndex;
            entryPointGroupIndices.insert(missInfo.groupIndex);
        }

        uint32_t hitCount = m_rayTypeCount * m_geometryCount;
        m_hitVars.resize(hitCount);

        for (uint32_t rayType = 0; rayType < m_rayTypeCount; rayType++)
        {
            for (uint32_t geometryID = 0; geometryID < m_geometryCount; geometryID++)
            {
                auto const& hitGroupInfo = bindingTable->getHitGroup(rayType, geometryID);
                if (!hitGroupInfo.isValid())
                    continue;

                m_hitVars[m_rayTypeCount * geometryID + rayType].entryPointGroupIndex = hitGroupInfo.groupIndex;
                entryPointGroupIndices.insert(hitGroupInfo.groupIndex);
            }
        }

        m_uniqueEntryPointGroupIndices.assign(entryPointGroupIndices.begin(), entryPointGroupIndices.end());
        AP_ASSERT(!m_uniqueEntryPointGroupIndices.empty(), "No entry points found in binding table");

        // Build list of vars for all entry point groups.
        // Note that there may be nullptr entries, as not all hit groups need to be assigned.
        AP_ASSERT(m_rayGenVars.size() == 1, "Raytracing binding table has more than one raygen shader");
    }

    auto RtProgramVariables::prepareShaderTable(CommandContext* pCtx, RayTracingPipeline* pRtso) -> bool
    {
        (void) pCtx;

        auto& pKernels = pRtso->getKernels(); // ProgramKernels

        bool needShaderTableUpdate = false;
        if (!m_gfxShaderTable) // check if wrapper has valid table
        {
            needShaderTableUpdate = true;
        }

        if (!needShaderTableUpdate)
        {
            if (pRtso != mp_currentRayTracingPipeline)
            {
                needShaderTableUpdate = true;
            }
        }

        if (needShaderTableUpdate)
        {
            auto getShaderNames = [&](VarsVector& varsVec, std::vector<const char*>& shaderNames)
            {
                for (uint32_t i = 0; i < (uint32_t)varsVec.size(); i++)
                {
                    auto& varsInfo = varsVec[i];

                    auto uniqueGroupIndex = varsInfo.entryPointGroupIndex;

                    auto pGroupKernels = uniqueGroupIndex >= 0 ? pKernels->getUniqueEntryPointGroup(uniqueGroupIndex) : nullptr;
                    if (!pGroupKernels)
                    {
                        shaderNames.push_back(nullptr);
                        continue;
                    }

                    shaderNames.push_back(static_cast<const char*>(pRtso->getShaderIdentifier(uniqueGroupIndex)));
                }
            };

            std::vector<const char*> rayGenShaders;
            getShaderNames(m_rayGenVars, rayGenShaders);

            std::vector<const char*> missShaders;
            getShaderNames(m_missVars, missShaders);

            std::vector<const char*> hitgroupShaders;
            getShaderNames(m_hitVars, hitgroupShaders);

            rhi::ShaderTableDesc desc = {};
            desc.rayGenShaderCount = (uint32_t)rayGenShaders.size();
            desc.rayGenShaderEntryPointNames = rayGenShaders.data();
            desc.missShaderCount = (uint32_t)missShaders.size();
            desc.missShaderEntryPointNames = missShaders.data();
            desc.hitGroupCount = (uint32_t)hitgroupShaders.size();
            desc.hitGroupNames = hitgroupShaders.data();
            desc.program = pRtso->getKernels()->getGfxShaderProgram();
            if (SLANG_FAILED(mp_device->getGfxDevice()->createShaderTable(desc, m_gfxShaderTable.writeRef())))
                return false;
            mp_currentRayTracingPipeline = pRtso;
        }

        return true;
    }

} // namespace april::graphics
