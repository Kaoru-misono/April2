// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "ray-tracing-pipeline.hpp"
#include "render-device.hpp"
#include "rhi-tools.hpp"
#include "graphics/program/program.hpp"
#include "graphics/program/program-version.hpp"

#include <core/error/assert.hpp>
#include <vector>

namespace april::graphics
{
    RayTracingPipeline::RayTracingPipeline(core::ref<Device> const& pDevice, RayTracingPipelineDesc const& desc)
        : mp_device(pDevice), m_desc(desc)
    {
        auto pKernels = m_desc.programKernels;
        rhi::RayTracingPipelineDesc rtpDesc = {};
        std::vector<rhi::HitGroupDesc> hitGroups;

        for (auto const& pEntryPointGroup : pKernels->getUniqueEntryPointGroups())
        {
            if (pEntryPointGroup->getType() == EntryPointGroupKernels::Type::RayTracingHitGroup)
            {
                auto const* pIntersection = pEntryPointGroup->getKernel(ShaderType::Intersection);
                auto const* pAhs = pEntryPointGroup->getKernel(ShaderType::AnyHit);
                auto const* pChs = pEntryPointGroup->getKernel(ShaderType::ClosestHit);

                rhi::HitGroupDesc hitgroupDesc = {};
                hitgroupDesc.anyHitEntryPoint = pAhs ? pAhs->getEntryPointName().c_str() : nullptr;
                hitgroupDesc.closestHitEntryPoint = pChs ? pChs->getEntryPointName().c_str() : nullptr;
                hitgroupDesc.intersectionEntryPoint = pIntersection ? pIntersection->getEntryPointName().c_str() : nullptr;
                hitgroupDesc.hitGroupName = pEntryPointGroup->getExportName().c_str();
                hitGroups.push_back(hitgroupDesc);
            }
        }

        rtpDesc.hitGroupCount = (uint32_t)hitGroups.size();
        rtpDesc.hitGroups = hitGroups.data();
        rtpDesc.maxRecursion = m_desc.maxTraceRecursionDepth;

        static_assert((uint32_t)rhi::RayTracingPipelineFlags::SkipProcedurals == (uint32_t)RtPipelineFlags::SkipProcedurals);
        static_assert((uint32_t)rhi::RayTracingPipelineFlags::SkipTriangles == (uint32_t)RtPipelineFlags::SkipTriangles);

        rtpDesc.flags = (rhi::RayTracingPipelineFlags)m_desc.pipelineFlags;

        auto rtProgram = dynamic_cast<Program*>(m_desc.programKernels->getProgramVersion()->getProgram());
        AP_ASSERT(rtProgram, "Program cast failed");

        rtpDesc.maxRayPayloadSize = rtProgram->getDescription().maxPayloadSize;
        rtpDesc.maxAttributeSizeInBytes = rtProgram->getDescription().maxAttributeSize;
        rtpDesc.program = m_desc.programKernels->getGfxShaderProgram();

        checkResult(mp_device->getGfxDevice()->createRayTracingPipeline(rtpDesc, m_gfxRayTracingPipeline.writeRef()), "Failed to create ray tracing pipeline state");

        for (auto const& pEntryPointGroup : pKernels->getUniqueEntryPointGroups())
        {
            m_entryPointGroupExportNames.push_back(pEntryPointGroup->getExportName());
        }
    }

    RayTracingPipeline::~RayTracingPipeline()
    {
    }

    auto RayTracingPipeline::getShaderIdentifier(uint32_t index) const -> void const*
    {
        return m_entryPointGroupExportNames[index].c_str();
    }

} // namespace april::graphics
