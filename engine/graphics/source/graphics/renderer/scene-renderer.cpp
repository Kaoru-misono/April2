#include "scene-renderer.hpp"

#include <core/error/assert.hpp>
#include <core/log/logger.hpp>
#include <asset/static-mesh-asset.hpp>
#include <graphics/rhi/depth-stencil-state.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace april::graphics
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

    SceneRenderer::SceneRenderer(core::ref<Device> device, asset::AssetManager* assetManager)
        : m_device(std::move(device))
        , m_assetManager(assetManager)
    {
        AP_ASSERT(m_device, "SceneRenderer requires a valid device.");
        AP_ASSERT(m_assetManager, "SceneRenderer requires a valid asset manager.");

        // Load cube mesh
        auto assetPath = std::string{"E:/github/April2/content/model/cube.asset"};
        if (std::filesystem::exists(assetPath))
        {
            auto meshAsset = m_assetManager->loadAsset<asset::StaticMeshAsset>(assetPath);
            if (meshAsset)
            {
                m_cubeMesh = m_device->createMeshFromAsset(*m_assetManager, *meshAsset);
                if (m_cubeMesh)
                {
                    AP_INFO("[SceneRenderer] Loaded cube mesh with {} submeshes", m_cubeMesh->getSubmeshCount());
                }
                else
                {
                    AP_ERROR("[SceneRenderer] Failed to create mesh from asset");
                }
            }
            else
            {
                AP_ERROR("[SceneRenderer] Failed to load mesh asset: {}", assetPath);
            }
        }
        else
        {
            auto meshAsset = std::static_pointer_cast<asset::StaticMeshAsset>(m_assetManager->importAsset("E:/github/April2/content/model/cube.gltf"));
            if (meshAsset)
            {
                m_cubeMesh = m_device->createMeshFromAsset(*m_assetManager, *meshAsset);
                if (m_cubeMesh)
                {
                    AP_INFO("[SceneRenderer] Loaded cube mesh with {} submeshes", m_cubeMesh->getSubmeshCount());
                }
                else
                {
                    AP_ERROR("[SceneRenderer] Failed to create mesh from asset");
                }
            }
            else
            {
                AP_ERROR("[SceneRenderer] Failed to load mesh asset: {}", assetPath);
            }
        }


        // Create standard vertex layout for mesh rendering
        auto bufferLayout = VertexBufferLayout::create();
        bufferLayout->addElement("POSITION", 0, ResourceFormat::RGB32Float, 1, 0);
        bufferLayout->addElement("NORMAL", 12, ResourceFormat::RGB32Float, 1, 1);
        bufferLayout->addElement("TANGENT", 24, ResourceFormat::RGBA32Float, 1, 2);
        bufferLayout->addElement("TEXCOORD", 40, ResourceFormat::RG32Float, 1, 3);

        auto vertexLayout = VertexLayout::create();
        vertexLayout->addBufferLayout(0, bufferLayout);

        // Create shader program
        ProgramDesc progDesc;
        progDesc.addShaderModule("SceneMeshVS").addString(kMeshVs, "SceneMeshVS.slang");
        progDesc.vsEntryPoint("main");
        progDesc.addShaderModule("SceneMeshPS").addString(kMeshPs, "SceneMeshPS.slang");
        progDesc.psEntryPoint("main");

        auto program = Program::create(m_device, progDesc);
        m_vars = ProgramVariables::create(m_device, program.get());

        // Create graphics pipeline
        GraphicsPipelineDesc pipelineDesc;
        pipelineDesc.programKernels = program->getActiveVersion()->getKernels(m_device.get(), nullptr);
        pipelineDesc.vertexLayout = vertexLayout;
        pipelineDesc.renderTargetCount = 1;
        pipelineDesc.renderTargetFormats[0] = ResourceFormat::RGBA16Float;
        pipelineDesc.depthStencilFormat = ResourceFormat::D32Float;
        pipelineDesc.primitiveType = GraphicsPipelineDesc::PrimitiveType::TriangleList;

        RasterizerState::Desc rsDesc;
        rsDesc.setCullMode(RasterizerState::CullMode::Back);
        pipelineDesc.rasterizerState = RasterizerState::create(rsDesc);

        DepthStencilState::Desc dsDesc;
        dsDesc.setDepthEnabled(true);
        dsDesc.setDepthFunc(ComparisonFunc::Less);
        pipelineDesc.depthStencilState = DepthStencilState::create(dsDesc);

        m_pipeline = m_device->createGraphicsPipeline(pipelineDesc);

        auto constexpr kDefaultFov = 45.0f;
        m_gameCamera = std::make_unique<FixedCamera>(
            float3{0.0f, 3.0f, 10.0f},
            float3{0.0f, 0.0f, 0.0f},
            float3{0.0f, 1.0f, 0.0f},
            glm::radians(kDefaultFov),
            1.777f,
            0.1f,
            1000.0f
        );
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
        if (m_gameCamera)
        {
            m_gameCamera->setViewportSize(width, height);
        }
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

        m_time += 0.016f;

        pContext->resourceBarrier(m_sceneColor.get(), Resource::State::RenderTarget);
        pContext->resourceBarrier(m_sceneDepth.get(), Resource::State::DepthStencil);

        auto colorTarget = ColorTarget(m_sceneColorRtv, LoadOp::Clear, StoreOp::Store, clearColor);
        auto depthTarget = DepthStencilTarget(m_sceneDepthDsv, LoadOp::Clear, StoreOp::Store, 1.0f, 0);
        auto encoder = pContext->beginRenderPass({colorTarget}, depthTarget);
        encoder->pushDebugGroup("SceneRenderer");

        Viewport vp = Viewport::fromSize((float)m_width, (float)m_height);
        encoder->setViewport(0, vp);
        encoder->setScissor(0, {0, 0, (uint32_t)vp.width, (uint32_t)vp.height});

        if (m_pipeline && m_vars && m_cubeMesh)
        {
            auto viewProj = glm::mat4(1.0f);
            if (m_useExternalCamera && m_hasExternalViewProj)
            {
                viewProj = m_externalViewProj;
            }
            else if (m_gameCamera)
            {
                m_gameCamera->onUpdate(0.016f);
                viewProj = m_gameCamera->getViewProjectionMatrix();
            }

            auto model = glm::rotate(glm::mat4(1.0f), m_time * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));

            // Set shader uniforms
            auto rootVar = m_vars->getRootVariable();
            rootVar["perFrame"]["viewProj"].setBlob(&viewProj, sizeof(glm::mat4));
            rootVar["perFrame"]["model"].setBlob(&model, sizeof(glm::mat4));
            rootVar["perFrame"]["time"].set(m_time);

            encoder->setVao(m_cubeMesh->getVAO());
            encoder->bindPipeline(m_pipeline.get(), m_vars.get());

            // Draw all submeshes
            for (size_t i = 0; i < m_cubeMesh->getSubmeshCount(); ++i)
            {
                auto const& submesh = m_cubeMesh->getSubmesh(i);
                encoder->drawIndexed(submesh.indexCount, submesh.indexOffset, 0);
            }
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

        m_sceneDepth = m_device->createTexture2D(
            width,
            height,
            ResourceFormat::D32Float,
            1,
            1,
            nullptr,
            TextureUsage::DepthStencil
        );
        m_sceneDepth->setName("SceneRenderer.SceneDepth");
        m_sceneDepthDsv = m_sceneDepth->getDSV();
    }
}
