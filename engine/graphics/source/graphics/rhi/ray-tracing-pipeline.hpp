// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "ray-tracing.hpp"
#include "program/program-version.hpp"
#include "fwd.hpp"

#include <core/foundation/object.hpp>
#include <slang-gfx.h>
#include <slang-com-ptr.h>
#include <string>
#include <vector>

namespace april::graphics
{
    class Device;

    struct RayTracingPipelineDesc
    {
        core::ref<ProgramKernels const> programKernels;
        uint32_t maxTraceRecursionDepth{0};
        RtPipelineFlags pipelineFlags{RtPipelineFlags::None};

        auto operator==(RayTracingPipelineDesc const& other) const -> bool
        {
            return programKernels == other.programKernels &&
                   maxTraceRecursionDepth == other.maxTraceRecursionDepth &&
                   pipelineFlags == other.pipelineFlags;
        }
    };

    class RayTracingPipeline : public core::Object
    {
        APRIL_OBJECT(RayTracingPipeline)
    public:
        RayTracingPipeline(core::ref<Device> const& p_device, RayTracingPipelineDesc const& desc);
        virtual ~RayTracingPipeline();

        auto getGfxPipelineState() const -> rhi::IRayTracingPipeline* { return m_gfxRayTracingPipeline; }

        auto getKernels() const -> core::ref<ProgramKernels const> const& { return m_desc.programKernels; }
        auto getMaxTraceRecursionDepth() const -> uint32_t { return m_desc.maxTraceRecursionDepth; }
        auto getShaderIdentifier(uint32_t index) const -> void const*;
        auto getDesc() const -> RayTracingPipelineDesc const& { return m_desc; }

    private:
        core::ref<Device> mp_device{};
        RayTracingPipelineDesc m_desc{};
        Slang::ComPtr<rhi::IRayTracingPipeline> m_gfxRayTracingPipeline{};
        std::vector<std::string> m_entryPointGroupExportNames{};
    };
}
