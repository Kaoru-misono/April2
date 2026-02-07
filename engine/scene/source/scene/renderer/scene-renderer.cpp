#include "scene-renderer.hpp"

#include <core/error/assert.hpp>
#include <graphics/rhi/depth-stencil-state.hpp>

namespace april::scene
{
    namespace
    {
        constexpr char const* kMeshVs = R"(
struct VSIn
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texCoord : TEXCOORD;
};

struct VSOut
{
    float4 pos : SV_Position;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
};

struct PerFrame
{
    float4x4 viewProj;
    float4x4 model;
    float time;
};
ParameterBlock<PerFrame> perFrame;

VSOut main(VSIn input)
{
    VSOut output;
    float4 worldPos = mul(perFrame.model, float4(input.position, 1.0));
    output.pos = mul(perFrame.viewProj, worldPos);
    output.normal = mul((float3x3)perFrame.model, input.normal);
    output.texCoord = input.texCoord;
    return output;
}
)";

        constexpr char const* kMeshPs = R"(
struct PSIn
{
    float4 pos : SV_Position;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
};

float4 main(PSIn input) : SV_Target
{
    float3 normal = normalize(input.normal);
    float3 lightDir = normalize(float3(1.0, 1.0, 1.0));
    float diffuse = max(dot(normal, lightDir), 0.0) * 0.8 + 0.2;

    float3 baseColor = float3(0.8, 0.3, 0.3);
    return float4(baseColor * diffuse, 1.0);
}
)";
    }

    SceneRenderer::SceneRenderer(core::ref<graphics::Device> device, asset::AssetManager* assetManager)
        : m_device(std::move(device))
        , m_resources(m_device, assetManager)
    {
        AP_ASSERT(m_device, "SceneRenderer requires a valid device.");
        AP_ASSERT(assetManager, "SceneRenderer requires a valid asset manager.");

        auto bufferLayout = graphics::VertexBufferLayout::create();
        bufferLayout->addElement("POSITION", 0, graphics::ResourceFormat::RGB32Float, 1, 0);
        bufferLayout->addElement("NORMAL", 12, graphics::ResourceFormat::RGB32Float, 1, 1);
        bufferLayout->addElement("TANGENT", 24, graphics::ResourceFormat::RGBA32Float, 1, 2);
        bufferLayout->addElement("TEXCOORD", 40, graphics::ResourceFormat::RG32Float, 1, 3);

        auto vertexLayout = graphics::VertexLayout::create();
        vertexLayout->addBufferLayout(0, bufferLayout);

        graphics::ProgramDesc progDesc;
        progDesc.addShaderModule("SceneMeshVS").addString(kMeshVs, "SceneMeshVS.slang");
        progDesc.vsEntryPoint("main");
        progDesc.addShaderModule("SceneMeshPS").addString(kMeshPs, "SceneMeshPS.slang");
        progDesc.psEntryPoint("main");

        auto program = graphics::Program::create(m_device, progDesc);
        m_vars = graphics::ProgramVariables::create(m_device, program.get());

        graphics::GraphicsPipelineDesc pipelineDesc;
        pipelineDesc.programKernels = program->getActiveVersion()->getKernels(m_device.get(), nullptr);
        pipelineDesc.vertexLayout = vertexLayout;
        pipelineDesc.renderTargetCount = 1;
        pipelineDesc.renderTargetFormats[0] = graphics::ResourceFormat::RGBA16Float;
        pipelineDesc.depthStencilFormat = graphics::ResourceFormat::D32Float;
        pipelineDesc.primitiveType = graphics::GraphicsPipelineDesc::PrimitiveType::TriangleList;

        graphics::RasterizerState::Desc rsDesc;
        rsDesc.setCullMode(graphics::RasterizerState::CullMode::Back);
        pipelineDesc.rasterizerState = graphics::RasterizerState::create(rsDesc);

        graphics::DepthStencilState::Desc dsDesc;
        dsDesc.setDepthEnabled(true);
        dsDesc.setDepthFunc(graphics::ComparisonFunc::Less);
        pipelineDesc.depthStencilState = graphics::DepthStencilState::create(dsDesc);

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

    auto SceneRenderer::getSceneColorTexture() const -> core::ref<graphics::Texture>
    {
        return m_sceneColor;
    }

    auto SceneRenderer::getSceneColorSrv() const -> core::ref<graphics::TextureView>
    {
        return m_sceneColorSrv;
    }

    auto SceneRenderer::getResourceRegistry() -> RenderResourceRegistry&
    {
        return m_resources;
    }

    auto SceneRenderer::acquireSnapshotForWrite() -> FrameSnapshot&
    {
        return m_snapshotBuffer.acquireWrite();
    }

    auto SceneRenderer::submitSnapshot() -> void
    {
        m_snapshotBuffer.submitWrite();
    }

    auto SceneRenderer::getSnapshotForRead() const -> FrameSnapshot const&
    {
        return m_snapshotBuffer.getRead();
    }

    auto SceneRenderer::render(graphics::CommandContext* pContext, FrameSnapshot const& snapshot, float4 const& clearColor) -> void
    {
        if (!pContext || !m_device)
        {
            return;
        }

        if (!m_sceneColor || m_width == 0 || m_height == 0)
        {
            return;
        }

        if (!snapshot.mainView.hasCamera)
        {
            return;
        }

        m_viewProjectionMatrix = snapshot.mainView.projectionMatrix * snapshot.mainView.viewMatrix;

        pContext->resourceBarrier(m_sceneColor.get(), graphics::Resource::State::RenderTarget);
        pContext->resourceBarrier(m_sceneDepth.get(), graphics::Resource::State::DepthStencil);

        auto colorTarget = graphics::ColorTarget(m_sceneColorRtv, graphics::LoadOp::Clear, graphics::StoreOp::Store, clearColor);
        auto depthTarget = graphics::DepthStencilTarget(m_sceneDepthDsv, graphics::LoadOp::Clear, graphics::StoreOp::Store, 1.0f, 0);
        auto encoder = pContext->beginRenderPass({colorTarget}, depthTarget);

        auto vp = graphics::Viewport::fromSize((float)m_width, (float)m_height);
        encoder->setViewport(0, vp);
        encoder->setScissor(0, {0, 0, (uint32_t)vp.width, (uint32_t)vp.height});
        encoder->pushDebugGroup("SceneRenderer");
        encoder->setViewport(0, vp);
        encoder->setScissor(0, {0, 0, (uint32_t)vp.width, (uint32_t)vp.height});

        renderMeshInstances(encoder, snapshot);

        encoder->popDebugGroup();
        encoder->end();

        pContext->resourceBarrier(m_sceneColor.get(), graphics::Resource::State::ShaderResource);
    }

    auto SceneRenderer::renderMeshInstances(core::ref<graphics::RenderPassEncoder> encoder, FrameSnapshot const& snapshot) -> void
    {
        auto rootVar = m_vars->getRootVariable();
        rootVar["perFrame"]["viewProj"].setBlob(&m_viewProjectionMatrix, sizeof(float4x4));

        auto drawList = [&](std::vector<MeshInstance> const& meshes) {
            for (auto const& instance : meshes)
            {
                if (instance.meshId == kInvalidRenderID)
                {
                    continue;
                }

                auto mesh = m_resources.getMesh(instance.meshId);
                if (!mesh)
                {
                    continue;
                }

                rootVar["perFrame"]["model"].setBlob(&instance.worldTransform, sizeof(float4x4));

                encoder->setVao(mesh->getVAO());
                encoder->bindPipeline(m_pipeline.get(), m_vars.get());

                for (size_t s = 0; s < mesh->getSubmeshCount(); ++s)
                {
                    auto const& submesh = mesh->getSubmesh(s);
                    encoder->drawIndexed(submesh.indexCount, submesh.indexOffset, 0);
                }
            }
        };

        if (!snapshot.staticMeshes.empty())
        {
            drawList(snapshot.staticMeshes);
        }

        if (!snapshot.dynamicMeshes.empty())
        {
            drawList(snapshot.dynamicMeshes);
        }
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
            graphics::TextureUsage::RenderTarget | graphics::TextureUsage::ShaderResource
        );
        m_sceneColor->setName("SceneRenderer.SceneColor");
        m_sceneColorRtv = m_sceneColor->getRTV();
        m_sceneColorSrv = m_sceneColor->getSRV();

        m_sceneDepth = m_device->createTexture2D(
            width,
            height,
            graphics::ResourceFormat::D32Float,
            1,
            1,
            nullptr,
            graphics::TextureUsage::DepthStencil
        );
        m_sceneDepth->setName("SceneRenderer.SceneDepth");
        m_sceneDepthDsv = m_sceneDepth->getDSV();
    }
}
