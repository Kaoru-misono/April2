// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "graphics-pipeline.hpp"
#include "render-device.hpp"
#include "low-level-context-data.hpp"
#include "rhi-tools.hpp"
#include "graphics/program/program-version.hpp"

#include <core/log/logger.hpp>
#include <core/error/assert.hpp>
#include <core/tools/enum.hpp>

namespace april::graphics
{
    namespace
    {
        auto getGFXBlendFactor(BlendState::BlendFunc func) -> rhi::BlendFactor
        {
            switch (func)
            {
            case BlendState::BlendFunc::Zero: return rhi::BlendFactor::Zero;
            case BlendState::BlendFunc::One: return rhi::BlendFactor::One;
            case BlendState::BlendFunc::SrcColor: return rhi::BlendFactor::SrcColor;
            case BlendState::BlendFunc::OneMinusSrcColor: return rhi::BlendFactor::InvSrcColor;
            case BlendState::BlendFunc::DstColor: return rhi::BlendFactor::DestColor;
            case BlendState::BlendFunc::OneMinusDstColor: return rhi::BlendFactor::InvDestColor;
            case BlendState::BlendFunc::SrcAlpha: return rhi::BlendFactor::SrcAlpha;
            case BlendState::BlendFunc::OneMinusSrcAlpha: return rhi::BlendFactor::InvSrcAlpha;
            case BlendState::BlendFunc::DstAlpha: return rhi::BlendFactor::DestAlpha;
            case BlendState::BlendFunc::OneMinusDstAlpha: return rhi::BlendFactor::InvDestAlpha;
            case BlendState::BlendFunc::BlendFactor: return rhi::BlendFactor::BlendColor;
            case BlendState::BlendFunc::OneMinusBlendFactor: return rhi::BlendFactor::InvBlendColor;
            case BlendState::BlendFunc::SrcAlphaSaturate: return rhi::BlendFactor::SrcAlphaSaturate;
            case BlendState::BlendFunc::Src1Color: return rhi::BlendFactor::SecondarySrcColor;
            case BlendState::BlendFunc::OneMinusSrc1Color: return rhi::BlendFactor::InvSecondarySrcColor;
            case BlendState::BlendFunc::Src1Alpha: return rhi::BlendFactor::SecondarySrcAlpha;
            case BlendState::BlendFunc::OneMinusSrc1Alpha: return rhi::BlendFactor::InvSecondarySrcAlpha;
            default: AP_CRITICAL("Unreachable code reached!"); return rhi::BlendFactor::Zero;
            }
        }

        auto getGFXBlendOp(BlendState::BlendOp op) -> rhi::BlendOp
        {
            switch (op)
            {
            case BlendState::BlendOp::Add: return rhi::BlendOp::Add;
            case BlendState::BlendOp::Subtract: return rhi::BlendOp::Subtract;
            case BlendState::BlendOp::ReverseSubtract: return rhi::BlendOp::ReverseSubtract;
            case BlendState::BlendOp::Min: return rhi::BlendOp::Min;
            case BlendState::BlendOp::Max: return rhi::BlendOp::Max;
            default: AP_CRITICAL("Unreachable code reached!"); return rhi::BlendOp::Add;
            }
        }

        auto getGFXStencilOp(DepthStencilState::StencilOp op) -> rhi::StencilOp
        {
            switch (op)
            {
            case DepthStencilState::StencilOp::Keep: return rhi::StencilOp::Keep;
            case DepthStencilState::StencilOp::Zero: return rhi::StencilOp::Zero;
            case DepthStencilState::StencilOp::Replace: return rhi::StencilOp::Replace;
            case DepthStencilState::StencilOp::Increase: return rhi::StencilOp::IncrementWrap;
            case DepthStencilState::StencilOp::IncreaseSaturate: return rhi::StencilOp::IncrementSaturate;
            case DepthStencilState::StencilOp::Decrease: return rhi::StencilOp::DecrementWrap;
            case DepthStencilState::StencilOp::DecreaseSaturate: return rhi::StencilOp::DecrementSaturate;
            case DepthStencilState::StencilOp::Invert: return rhi::StencilOp::Invert;
            default: AP_CRITICAL("Unreachable code reached!"); return rhi::StencilOp::Keep;
            }
        }

        auto getGFXComparisonFunc(ComparisonFunc func) -> rhi::ComparisonFunc
        {
            switch (func)
            {
            case ComparisonFunc::Disabled: return rhi::ComparisonFunc::Never;
            case ComparisonFunc::Never: return rhi::ComparisonFunc::Never;
            case ComparisonFunc::Always: return rhi::ComparisonFunc::Always;
            case ComparisonFunc::Less: return rhi::ComparisonFunc::Less;
            case ComparisonFunc::Equal: return rhi::ComparisonFunc::Equal;
            case ComparisonFunc::NotEqual: return rhi::ComparisonFunc::NotEqual;
            case ComparisonFunc::LessEqual: return rhi::ComparisonFunc::LessEqual;
            case ComparisonFunc::Greater: return rhi::ComparisonFunc::Greater;
            case ComparisonFunc::GreaterEqual: return rhi::ComparisonFunc::GreaterEqual;
            default: AP_UNREACHABLE();
            }
        }

        void getGFXStencilDesc(rhi::DepthStencilOpDesc& gfxDesc, DepthStencilState::StencilDesc desc)
        {
            gfxDesc.stencilDepthFailOp = getGFXStencilOp(desc.depthFailOp);
            gfxDesc.stencilFailOp = getGFXStencilOp(desc.stencilFailOp);
            gfxDesc.stencilPassOp = getGFXStencilOp(desc.depthStencilPassOp);
            gfxDesc.stencilFunc = getGFXComparisonFunc(desc.func);
        }

        auto getGFXPrimitiveType(GraphicsPipelineDesc::PrimitiveType primitiveType) -> rhi::PrimitiveTopology
        {
            switch (primitiveType)
            {
            case GraphicsPipelineDesc::PrimitiveType::PointList: return rhi::PrimitiveTopology::PointList;
            case GraphicsPipelineDesc::PrimitiveType::LineList: return rhi::PrimitiveTopology::LineList;
            case GraphicsPipelineDesc::PrimitiveType::LineStrip: return rhi::PrimitiveTopology::LineStrip;
            case GraphicsPipelineDesc::PrimitiveType::TriangleList: return rhi::PrimitiveTopology::TriangleList;
            case GraphicsPipelineDesc::PrimitiveType::TriangleStrip: return rhi::PrimitiveTopology::TriangleStrip;
            case GraphicsPipelineDesc::PrimitiveType::PatchList: return rhi::PrimitiveTopology::PatchList;
            default: AP_UNREACHABLE();
            }
        }

        auto getGFXCullMode(RasterizerState::CullMode mode) -> rhi::CullMode
        {
            switch (mode)
            {
            case RasterizerState::CullMode::None: return rhi::CullMode::None;
            case RasterizerState::CullMode::Front: return rhi::CullMode::Front;
            case RasterizerState::CullMode::Back: return rhi::CullMode::Back;
            default: AP_UNREACHABLE();
            }
        }

        auto getGFXFillMode(RasterizerState::FillMode mode) -> rhi::FillMode
        {
            switch (mode)
            {
            case RasterizerState::FillMode::Wireframe: return rhi::FillMode::Wireframe;
            case RasterizerState::FillMode::Solid: return rhi::FillMode::Solid;
            default: AP_CRITICAL("Unreachable code reached!"); return rhi::FillMode::Solid;
            }
        }

        auto getGFXInputSlotClass(VertexBufferLayout::InputClass cls) -> rhi::InputSlotClass
        {
            switch (cls)
            {
            case VertexBufferLayout::InputClass::PerVertexData: return rhi::InputSlotClass::PerVertex;
            case VertexBufferLayout::InputClass::PerInstanceData: return rhi::InputSlotClass::PerInstance;
            default: AP_CRITICAL("Unreachable code reached!"); return rhi::InputSlotClass::PerVertex;
            }
        }
    } // namespace

    core::ref<BlendState> GraphicsPipeline::sp_defaultBlendState; // TODO: REMOVEGLOBAL
    core::ref<RasterizerState> GraphicsPipeline::sp_defaultRasterizerState; // TODO: REMOVEGLOBAL
    core::ref<DepthStencilState> GraphicsPipeline::sp_defaultDepthStencilState; // TODO: REMOVEGLOBAL

    GraphicsPipeline::~GraphicsPipeline()
    {
    }

    GraphicsPipeline::GraphicsPipeline(core::ref<Device> device, GraphicsPipelineDesc const& desc)
        : mp_device(device), m_desc(desc)
    {
        if (!sp_defaultBlendState)
        {
            sp_defaultBlendState = BlendState::create(BlendState::Desc());
            sp_defaultDepthStencilState = DepthStencilState::create(DepthStencilState::Desc());
            sp_defaultRasterizerState = RasterizerState::create(RasterizerState::Desc());
        }

        if (!m_desc.blendState) m_desc.blendState = sp_defaultBlendState;
        if (!m_desc.rasterizerState) m_desc.rasterizerState = sp_defaultRasterizerState;
        if (!m_desc.depthStencilState) m_desc.depthStencilState = sp_defaultDepthStencilState;

        rhi::RenderPipelineDesc gfxDesc = {};

        // Blend State
        auto const& blendState = m_desc.blendState;
        AP_ASSERT(blendState->getRtCount() <= kMaxRenderTargetCount, "Too many render targets");

        std::vector<rhi::ColorTargetDesc> colorTargets(blendState->getRtCount());
        gfxDesc.targetCount = blendState->getRtCount();
        gfxDesc.targets = colorTargets.data();

        for (uint32_t i = 0; i < gfxDesc.targetCount; ++i)
        {
            auto const& rtDesc = blendState->getRtDesc(i);
            auto& gfxRtDesc = colorTargets[i];

            gfxRtDesc.format = m_desc.renderTargetFormats[i];
            gfxRtDesc.enableBlend = rtDesc.blendEnabled;

            gfxRtDesc.alpha.dstFactor = getGFXBlendFactor(rtDesc.dstAlphaFunc);
            gfxRtDesc.alpha.srcFactor = getGFXBlendFactor(rtDesc.srcAlphaFunc);
            gfxRtDesc.alpha.op = getGFXBlendOp(rtDesc.alphaBlendOp);

            gfxRtDesc.color.dstFactor = getGFXBlendFactor(rtDesc.dstRgbFunc);
            gfxRtDesc.color.srcFactor = getGFXBlendFactor(rtDesc.srcRgbFunc);
            gfxRtDesc.color.op = getGFXBlendOp(rtDesc.rgbBlendOp);

            gfxRtDesc.writeMask = rhi::RenderTargetWriteMask::None;
            if (rtDesc.writeMask.writeAlpha) gfxRtDesc.writeMask |= rhi::RenderTargetWriteMask::Alpha;
            if (rtDesc.writeMask.writeBlue) gfxRtDesc.writeMask |= rhi::RenderTargetWriteMask::Blue;
            if (rtDesc.writeMask.writeGreen) gfxRtDesc.writeMask |= rhi::RenderTargetWriteMask::Green;
            if (rtDesc.writeMask.writeRed) gfxRtDesc.writeMask |= rhi::RenderTargetWriteMask::Red;
        }

        // Depth Stencil State
        {
            auto const& dsState = m_desc.depthStencilState;
            gfxDesc.depthStencil.format = m_desc.depthStencilFormat;

            getGFXStencilDesc(gfxDesc.depthStencil.backFace, dsState->getStencilDesc(DepthStencilState::Face::Back));
            getGFXStencilDesc(gfxDesc.depthStencil.frontFace, dsState->getStencilDesc(DepthStencilState::Face::Front));
            gfxDesc.depthStencil.depthFunc = getGFXComparisonFunc(dsState->getDepthFunc());
            gfxDesc.depthStencil.depthTestEnable = dsState->isDepthTestEnabled();
            gfxDesc.depthStencil.depthWriteEnable = dsState->isDepthWriteEnabled();
            gfxDesc.depthStencil.stencilEnable = dsState->isStencilTestEnabled();
            gfxDesc.depthStencil.stencilReadMask = dsState->getStencilReadMask();
            gfxDesc.depthStencil.stencilWriteMask = dsState->getStencilWriteMask();
        }

        // Rasterizer State
        {
            auto const& rsState = m_desc.rasterizerState;
            gfxDesc.rasterizer.antialiasedLineEnable = rsState->isLineAntiAliasingEnabled();
            gfxDesc.rasterizer.cullMode = getGFXCullMode(rsState->getCullMode());
            gfxDesc.rasterizer.depthBias = rsState->getDepthBias();
            gfxDesc.rasterizer.slopeScaledDepthBias = rsState->getSlopeScaledDepthBias();
            gfxDesc.rasterizer.depthBiasClamp = 0.0f;
            gfxDesc.rasterizer.depthClipEnable = !rsState->isDepthClampEnabled();
            gfxDesc.rasterizer.fillMode = getGFXFillMode(rsState->getFillMode());
            gfxDesc.rasterizer.frontFace = rsState->isFrontCounterCW() ? rhi::FrontFaceMode::CounterClockwise : rhi::FrontFaceMode::Clockwise;
            gfxDesc.rasterizer.multisampleEnable = m_desc.sampleCount != 1;
            gfxDesc.rasterizer.scissorEnable = rsState->isScissorTestEnabled();
            gfxDesc.rasterizer.enableConservativeRasterization = rsState->isConservativeRasterizationEnabled();
            gfxDesc.rasterizer.forcedSampleCount = rsState->getForcedSampleCount();
        }

        // Input Layout
        if (m_desc.vertexLayout)
        {
            std::vector<rhi::VertexStreamDesc> vertexStreams(m_desc.vertexLayout->getBufferCount());
            std::vector<rhi::InputElementDesc> inputElements;

            for (size_t i = 0; i < m_desc.vertexLayout->getBufferCount(); ++i)
            {
                auto const& bufferLayout = m_desc.vertexLayout->getBufferLayout(i);
                vertexStreams[i].instanceDataStepRate = bufferLayout->getInstanceStepRate();
                vertexStreams[i].slotClass = getGFXInputSlotClass(bufferLayout->getInputClass());
                vertexStreams[i].stride = bufferLayout->getStride();

                for (uint32_t j = 0; j < bufferLayout->getElementCount(); ++j)
                {
                    rhi::InputElementDesc elementDesc = {};
                    elementDesc.format = getGFXFormat(bufferLayout->getElementFormat(j));
                    elementDesc.offset = bufferLayout->getElementOffset(j);
                    elementDesc.semanticName = bufferLayout->getElementName(j).c_str();
                    elementDesc.bufferSlotIndex = static_cast<uint32_t>(i);

                    for (uint32_t arrayIndex = 0; arrayIndex < bufferLayout->getElementArraySize(j); arrayIndex++)
                    {
                        elementDesc.semanticIndex = arrayIndex;
                        inputElements.push_back(elementDesc);
                        elementDesc.offset += getFormatBytesPerBlock(bufferLayout->getElementFormat(j));
                    }
                }
            }

            rhi::InputLayoutDesc inputLayoutDesc = {};
            inputLayoutDesc.inputElementCount = static_cast<uint32_t>(inputElements.size());
            inputLayoutDesc.inputElements = inputElements.data();
            inputLayoutDesc.vertexStreamCount = static_cast<uint32_t>(vertexStreams.size());
            inputLayoutDesc.vertexStreams = vertexStreams.data();
            checkResult(mp_device->getGfxDevice()->createInputLayout(inputLayoutDesc, mp_gfxInputLayout.writeRef()), "Failed to create input layout");
        }
        gfxDesc.inputLayout = mp_gfxInputLayout;

        gfxDesc.primitiveTopology = getGFXPrimitiveType(m_desc.primitiveType);
        gfxDesc.program = m_desc.programKernels->getGfxShaderProgram();

        checkResult(mp_device->getGfxDevice()->createRenderPipeline(gfxDesc, m_gfxRenderPipeline.writeRef()), "Failed to create render pipeline");
    }

    void GraphicsPipeline::breakStrongReferenceToDevice()
    {
        mp_device.breakStrongReference();
    }

} // namespace april::graphics
