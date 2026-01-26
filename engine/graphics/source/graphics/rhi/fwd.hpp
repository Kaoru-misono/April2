// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include <core/foundation/object.hpp>

namespace april::graphics
{
    class Device;

    class CommandContext;

    struct GraphicsPipelineDesc;
    class GraphicsPipeline;

    struct ComputePipelineDesc;
    class ComputePipeline;

    struct RayTracingPipelineDesc;
    class RayTracingPipeline;

    class QueryHeap;

    class GpuTimer;
    class GpuMemoryHeap;

    class VertexLayout;
    class VertexBufferLayout;
    class VertexArrayObject;

    class BlendState;
    class DepthStencilState;
    class RasterizerState;

    class FrameBuffer;
    class Swapchain;

    class RtAccelerationStructure;
    class RtAccelerationStructurePostBuildInfoPool;
    class ShaderTablePtr;

    class ResourceView;
    class ShaderResourceView;
    class UnorderedAccessView;
    class RenderTargetView;
    class DepthStencilView;
} // namespace april::graphics