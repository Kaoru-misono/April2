// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "compute-pipeline.hpp"
#include "render-device.hpp"
#include "rhi-tools.hpp"
#include "graphics/program/program-version.hpp"

#include <core/error/assert.hpp>

namespace april::graphics
{
    ComputePipeline::ComputePipeline(core::ref<Device> const& device, ComputePipelineDesc const& desc)
        : m_device(device), m_desc(desc)
    {
        rhi::ComputePipelineDesc computePipelineDesc = {};
        computePipelineDesc.program = m_desc.programKernels->getGfxShaderProgram();

        checkResult(m_device->getGfxDevice()->createComputePipeline(computePipelineDesc, m_gfxComputePipeline.writeRef()), "Failed to create compute pipeline state");
    }

    ComputePipeline::~ComputePipeline()
    {
    }

    rhi::NativeHandle ComputePipeline::getNativeHandle() const
    {
        rhi::NativeHandle gfxNativeHandle = {};
        checkResult(m_gfxComputePipeline->getNativeHandle(&gfxNativeHandle), "Failed to get native handle");
        return gfxNativeHandle;
    }

} // namespace april::graphics
