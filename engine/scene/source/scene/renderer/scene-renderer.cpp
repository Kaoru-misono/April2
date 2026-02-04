#include "scene-renderer.hpp"

#include <asset/static-mesh-asset.hpp>
#include <cmath>
#include <core/error/assert.hpp>
#include <core/log/logger.hpp>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
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
        , m_assetManager(assetManager)
    {
        AP_ASSERT(m_device, "SceneRenderer requires a valid device.");
        AP_ASSERT(m_assetManager, "SceneRenderer requires a valid asset manager.");

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

    auto SceneRenderer::render(graphics::CommandContext* pContext, SceneGraph const& sceneGraph, float4 const& clearColor) -> void
    {
        if (!pContext || !m_device)
        {
            return;
        }

        if (!m_sceneColor || m_width == 0 || m_height == 0)
        {
            return;
        }

        auto const& registry = sceneGraph.getRegistry();

        pContext->resourceBarrier(m_sceneColor.get(), graphics::Resource::State::RenderTarget);
        pContext->resourceBarrier(m_sceneDepth.get(), graphics::Resource::State::DepthStencil);

        updateActiveCamera(sceneGraph);
        if (!m_hasActiveCamera)
        {
            return;
        }

        auto const* meshPool = registry.getPool<MeshRendererComponent>();
        if (meshPool)
        {
            for (size_t i = 0; i < meshPool->data().size(); ++i)
            {
                auto const& meshComp = meshPool->data()[i];
                if (meshComp.enabled && !meshComp.meshAssetPath.empty())
                {
                    getMeshForPath(meshComp.meshAssetPath);
                }
            }
        }

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

        renderMeshEntities(encoder, registry);

        encoder->popDebugGroup();
        encoder->end();

        pContext->resourceBarrier(m_sceneColor.get(), graphics::Resource::State::ShaderResource);
    }

    auto SceneRenderer::getMeshForPath(std::string const& path) -> core::ref<graphics::StaticMesh>
    {
        auto it = m_meshCache.find(path);
        if (it != m_meshCache.end())
        {
            return it->second;
        }

        auto mesh = core::ref<graphics::StaticMesh>{};
        if (std::filesystem::exists(path))
        {
            auto meshAsset = m_assetManager->loadAsset<asset::StaticMeshAsset>(path);
            if (meshAsset)
            {
                mesh = m_device->createMeshFromAsset(*m_assetManager, *meshAsset);
                if (mesh)
                {
                    AP_INFO("[SceneRenderer] Loaded mesh from asset: {} ({} submeshes)", path, mesh->getSubmeshCount());
                }
                else
                {
                    AP_ERROR("[SceneRenderer] Failed to create mesh from asset: {}", path);
                }
            }
            else
            {
                AP_ERROR("[SceneRenderer] Failed to load mesh asset: {}", path);
            }
        }
        else
        {
            AP_ERROR("[SceneRenderer] Mesh asset not found: {}", path);
        }

        if (mesh)
        {
            m_meshCache[path] = mesh;
        }

        return mesh;
    }

    auto SceneRenderer::updateActiveCamera(SceneGraph const& sceneGraph) -> void
    {
        auto const activeCamera = sceneGraph.getActiveCamera();
        if (activeCamera == NullEntity)
        {
            AP_WARN("[SceneRenderer] No active camera found");
            m_hasActiveCamera = false;
            return;
        }

        auto const& registry = sceneGraph.getRegistry();
        if (!registry.allOf<CameraComponent>(activeCamera))
        {
            AP_WARN("[SceneRenderer] Active camera missing CameraComponent");
            m_hasActiveCamera = false;
            return;
        }

        auto const& camComp = registry.get<CameraComponent>(activeCamera);
        m_viewProjectionMatrix = camComp.viewProjectionMatrix;
        m_hasActiveCamera = true;
    }

    auto SceneRenderer::renderMeshEntities(core::ref<graphics::RenderPassEncoder> encoder, Registry const& registry) -> void
    {
        auto const* meshPool = registry.getPool<MeshRendererComponent>();
        if (!meshPool)
        {
            return;
        }

        for (size_t i = 0; i < meshPool->data().size(); ++i)
        {
            auto const entity = meshPool->getEntity(i);
            auto const& meshComp = meshPool->data()[i];

            if (!meshComp.enabled)
            {
                continue;
            }

            if (!registry.allOf<TransformComponent>(entity))
            {
                continue;
            }

            auto const& transform = registry.get<TransformComponent>(entity);
            auto mesh = getMeshForPath(meshComp.meshAssetPath);
            if (!mesh)
            {
                continue;
            }

            auto rootVar = m_vars->getRootVariable();
            rootVar["perFrame"]["viewProj"].setBlob(&m_viewProjectionMatrix, sizeof(float4x4));
            rootVar["perFrame"]["model"].setBlob(&transform.worldMatrix, sizeof(float4x4));

            encoder->setVao(mesh->getVAO());
            encoder->bindPipeline(m_pipeline.get(), m_vars.get());

            for (size_t s = 0; s < mesh->getSubmeshCount(); ++s)
            {
                auto const& submesh = mesh->getSubmesh(s);
                encoder->drawIndexed(submesh.indexCount, submesh.indexOffset, 0);
            }
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
