// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "fwd.hpp"
#include "native-handle.hpp"

#include <core/foundation/object.hpp>
#include <slang-com-ptr.h>
#include <slang-rhi.h>

namespace april::graphics
{
    class ProgramKernels;
    class Device;

    struct ComputePipelineDesc
    {
        core::ref<ProgramKernels const> programKernels{};

        auto operator==(ComputePipelineDesc const& other) const -> bool
        {
            return programKernels == other.programKernels;
        }
    };

    class ComputePipeline : public core::Object
    {
        APRIL_OBJECT(ComputePipeline)
    public:
        ComputePipeline(core::ref<Device> const& p_device, ComputePipelineDesc const& desc);
        ~ComputePipeline();

        auto getGfxPipelineState() const -> rhi::IComputePipeline* { return m_gfxComputePipeline; }
        auto getNativeHandle() const -> rhi::NativeHandle;
        auto getDesc() const -> ComputePipelineDesc const& { return m_desc; }

    private:
        core::ref<Device> m_device{};
        ComputePipelineDesc m_desc{};
        Slang::ComPtr<rhi::IComputePipeline> m_gfxComputePipeline{};
    };
}
