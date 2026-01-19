// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "fwd.hpp"
#include "vertex-layout.hpp"
#include "rasterizer-state.hpp"
#include "depth-stencil-state.hpp"
#include "blend-state.hpp"

#include <core/foundation/object.hpp>
#include <cstdint>
#include <limits>
#include <slang-rhi.h>

namespace april::graphics
{
    class ProgramKernels;

    static constexpr uint32_t kMaxRenderTargetCount = 8;

    struct GraphicsPipelineDesc
    {
        static constexpr uint32_t kSampleMaskAll = std::numeric_limits<uint32_t>::max();

        enum class PrimitiveType
        {
            PointList,
            LineList,
            LineStrip,
            TriangleList,
            TriangleStrip,
            PatchList,
        };

        core::ref<const VertexLayout> vertexLayout{};
        core::ref<const ProgramKernels> programKernels{};
        core::ref<RasterizerState> rasterizerState{};
        core::ref<DepthStencilState> depthStencilState{};
        core::ref<BlendState> blendState{};
        uint32_t sampleMask{kSampleMaskAll};
        PrimitiveType primitiveType{PrimitiveType::TriangleList};

        uint32_t renderTargetCount{0};
        rhi::Format renderTargetFormats[kMaxRenderTargetCount]{rhi::Format::Undefined};
        rhi::Format depthStencilFormat{rhi::Format::Undefined};
        uint32_t sampleCount{1};

        auto operator==(GraphicsPipelineDesc const& other) const -> bool
        {
            if (vertexLayout != other.vertexLayout ||
                programKernels != other.programKernels ||
                sampleMask != other.sampleMask ||
                primitiveType != other.primitiveType ||
                rasterizerState != other.rasterizerState ||
                blendState != other.blendState ||
                depthStencilState != other.depthStencilState ||
                renderTargetCount != other.renderTargetCount ||
                depthStencilFormat != other.depthStencilFormat ||
                sampleCount != other.sampleCount)
            {
                return false;
            }

            for (uint32_t i = 0; i < renderTargetCount; ++i)
            {
                if (renderTargetFormats[i] != other.renderTargetFormats[i]) return false;
            }
            return true;
        }
    };

    class GraphicsPipeline : public core::Object
    {
        APRIL_OBJECT(GraphicsPipeline)
    public:
        GraphicsPipeline(core::ref<Device> device, GraphicsPipelineDesc const& desc);
        ~GraphicsPipeline();

        auto getGfxPipeline() const -> rhi::IRenderPipeline* { return m_gfxRenderPipeline; }
        auto getDesc() const -> GraphicsPipelineDesc const& { return m_desc; }
        auto breakStrongReferenceToDevice() -> void;

    private:
        core::BreakableReference<Device> mp_device;
        GraphicsPipelineDesc m_desc{};
        rhi::ComPtr<rhi::IInputLayout> mp_gfxInputLayout; // Need to store this too
        rhi::ComPtr<rhi::IRenderPipeline> m_gfxRenderPipeline{};

        // Default state objects // TODO: Remove Global
        static core::ref<BlendState> sp_defaultBlendState;
        static core::ref<RasterizerState> sp_defaultRasterizerState;
        static core::ref<DepthStencilState> sp_defaultDepthStencilState;
    };
}
