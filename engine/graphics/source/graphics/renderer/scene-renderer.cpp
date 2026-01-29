#include "scene-renderer.hpp"

#include <core/error/assert.hpp>

namespace april::graphics
{
    namespace
    {
        constexpr char const* kTriangleVs = R"(
struct VSOut { float4 pos : SV_Position; float4 color : COLOR; };
VSOut main(uint vertexId : SV_VertexID)
{
    VSOut output;
    float2 positions[3] = { float2(0.0, 0.6), float2(0.6, -0.6), float2(-0.6, -0.6) };
    float3 colors[3] = { float3(1.0, 0.2, 0.2), float3(0.2, 1.0, 0.2), float3(0.2, 0.4, 1.0) };
    output.pos = float4(positions[vertexId], 0.0, 1.0);
    output.color = float4(colors[vertexId], 1.0);
    return output;
}
)";

        constexpr char const* kTrianglePs = R"(
float4 main(float4 pos : SV_Position, float4 color : COLOR) : SV_Target
{
    return color;
}
)";
    }

    SceneRenderer::SceneRenderer(core::ref<Device> device)
        : m_device(std::move(device))
    {
        AP_ASSERT(m_device, "SceneRenderer requires a valid device.");

        ProgramDesc progDesc;
        progDesc.addShaderModule("SceneTriangleVS").addString(kTriangleVs, "SceneTriangleVS.slang");
        progDesc.vsEntryPoint("main");
        progDesc.addShaderModule("SceneTrianglePS").addString(kTrianglePs, "SceneTrianglePS.slang");
        progDesc.psEntryPoint("main");

        auto program = Program::create(m_device, progDesc);
        m_vars = ProgramVariables::create(m_device, program.get());

        GraphicsPipelineDesc pipelineDesc;
        pipelineDesc.programKernels = program->getActiveVersion()->getKernels(m_device.get(), nullptr);
        pipelineDesc.renderTargetCount = 1;
        pipelineDesc.renderTargetFormats[0] = rhi::Format::RGBA16Float;
        pipelineDesc.primitiveType = GraphicsPipelineDesc::PrimitiveType::TriangleList;

        RasterizerState::Desc rsDesc;
        rsDesc.setCullMode(RasterizerState::CullMode::None);
        pipelineDesc.rasterizerState = RasterizerState::create(rsDesc);

        m_pipeline = m_device->createGraphicsPipeline(pipelineDesc);
    }

    auto SceneRenderer::setViewportSize(uint32_t width, uint32_t height) -> void
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        if (width == m_width && height == m_height)
        {
            return;
        }

        m_width = width;
        m_height = height;
        ensureTarget(m_width, m_height);
    }

    auto SceneRenderer::render(CommandContext* pContext, float4 const& clearColor) -> void
    {
        if (!pContext || !m_device)
        {
            return;
        }

        if (!m_sceneColor || m_width == 0 || m_height == 0)
        {
            return;
        }

        pContext->resourceBarrier(m_sceneColor.get(), Resource::State::RenderTarget);

        auto colorTarget = ColorTarget(m_sceneColorRtv, LoadOp::Clear, StoreOp::Store, clearColor);
        auto encoder = pContext->beginRenderPass({colorTarget});
        encoder->pushDebugGroup("SceneRenderer");

        Viewport vp = Viewport::fromSize((float)m_width, (float)m_height);
        encoder->setViewport(0, vp);
        encoder->setScissor(0, {0, 0, (uint32_t)vp.width, (uint32_t)vp.height});

        if (m_pipeline && m_vars)
        {
            encoder->bindPipeline(m_pipeline.get(), m_vars.get());
            encoder->draw(3, 0);
        }

        encoder->popDebugGroup();
        encoder->end();

        pContext->resourceBarrier(m_sceneColor.get(), Resource::State::ShaderResource);
    }

    auto SceneRenderer::ensureTarget(uint32_t width, uint32_t height) -> void
    {
        if (!m_device || width == 0 || height == 0)
        {
            return;
        }

        m_sceneColor = m_device->createTexture2D(
            width,
            height,
            m_format,
            1,
            1,
            nullptr,
            TextureUsage::RenderTarget | TextureUsage::ShaderResource
        );
        m_sceneColor->setName("SceneRenderer.SceneColor");
        m_sceneColorRtv = m_sceneColor->getRTV();
        m_sceneColorSrv = m_sceneColor->getSRV();
    }
}
