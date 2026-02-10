#include "scene-renderer.hpp"

#include <core/error/assert.hpp>
#include <graphics/material/standard-material.hpp>
#include <graphics/rhi/depth-stencil-state.hpp>

#include <cstdlib>

namespace april::scene
{
    static constexpr uint32_t kMaterialTextureTableSize = 128;
    static constexpr uint32_t kMaterialSamplerTableSize = 8;

    SceneRenderer::SceneRenderer(core::ref<graphics::Device> device, asset::AssetManager* assetManager)
        : m_device(std::move(device))
        , m_resources(m_device, assetManager)
    {
        AP_ASSERT(m_device, "SceneRenderer requires a valid device.");
        AP_ASSERT(assetManager, "SceneRenderer requires a valid asset manager.");

        // Create default/fallback resources
        createDefaultResources();

        auto bufferLayout = graphics::VertexBufferLayout::create();
        bufferLayout->addElement("POSITION", 0, graphics::ResourceFormat::RGB32Float, 1, 0);
        bufferLayout->addElement("NORMAL", 12, graphics::ResourceFormat::RGB32Float, 1, 1);
        bufferLayout->addElement("TANGENT", 24, graphics::ResourceFormat::RGBA32Float, 1, 2);
        bufferLayout->addElement("TEXCOORD", 40, graphics::ResourceFormat::RG32Float, 1, 3);

        auto vertexLayout = graphics::VertexLayout::create();
        vertexLayout->addBufferLayout(0, bufferLayout);

        // Create program from shader file
        graphics::ProgramDesc progDesc;
        progDesc.addShaderLibrary("graphics/scene/scene-mesh.slang");
        progDesc.vsEntryPoint("vsMain");
        progDesc.psEntryPoint("psMain");

        graphics::DefineList programDefines;

        if (auto* materialSystem = m_resources.getMaterialSystem())
        {
            auto const shaderModules = materialSystem->getShaderModules();
            progDesc.addShaderModules(shaderModules);
            programDefines.add(materialSystem->getShaderDefines());
            // Enforce material instance storage budget expected by shader interfaces.
            programDefines.add("FALCOR_MATERIAL_INSTANCE_SIZE", "256");

            auto const typeConformances = materialSystem->getTypeConformances();
            progDesc.addTypeConformances(typeConformances);

            AP_INFO(
                "[SceneRenderer] Material program setup: {} shader module(s), {} type conformance(s), FALCOR_MATERIAL_INSTANCE_SIZE={}",
                shaderModules.size(),
                typeConformances.size(),
                programDefines.contains("FALCOR_MATERIAL_INSTANCE_SIZE") ? programDefines.at("FALCOR_MATERIAL_INSTANCE_SIZE") : "<unset>"
            );

            auto hasMaterialConformance = false;
            for (auto const& [conformance, id] : typeConformances)
            {
                (void)id;
                if (conformance.interfaceName == "IMaterial")
                {
                    hasMaterialConformance = true;
                    break;
                }
            }

            if (!hasMaterialConformance)
            {
                AP_WARN("[SceneRenderer] No IMaterial type conformance registered; using shader defaults.");
            }
        }

        m_program = graphics::Program::create(m_device, progDesc, programDefines);
        m_vars = graphics::ProgramVariables::create(m_device, m_program.get());

        graphics::GraphicsPipelineDesc pipelineDesc;
        pipelineDesc.programKernels = m_program->getActiveVersion()->getKernels(m_device.get(), nullptr);
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

    auto SceneRenderer::createDefaultResources() -> void
    {
        // Create 1x1 white texture for missing textures
        auto constexpr whitePixel = uint32_t{0xFFFFFFFF};
        m_defaultWhiteTexture = m_device->createTexture2D(
            1, 1,
            graphics::ResourceFormat::RGBA8Unorm,
            1, 1,
            &whitePixel,
            graphics::TextureUsage::ShaderResource
        );
        m_defaultWhiteTexture->setName("SceneRenderer.DefaultWhite");

        // Create default sampler with linear filtering and wrap addressing
        graphics::Sampler::Desc samplerDesc;
        samplerDesc.setFilterMode(
            graphics::TextureFilteringMode::Linear,
            graphics::TextureFilteringMode::Linear,
            graphics::TextureFilteringMode::Linear
        );
        samplerDesc.setAddressingMode(
            graphics::TextureAddressingMode::Wrap,
            graphics::TextureAddressingMode::Wrap,
            graphics::TextureAddressingMode::Wrap
        );
        samplerDesc.setMaxAnisotropy(8);
        m_defaultSampler = m_device->createSampler(samplerDesc);

        // Create default material (white, non-metallic, medium roughness)
        m_defaultMaterial = core::make_ref<graphics::StandardMaterial>();
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
        m_cameraPosition = snapshot.mainView.cameraPosition;

        // Update material GPU buffers
        auto* materialSystem = m_resources.getMaterialSystem();
        if (materialSystem)
        {
            materialSystem->updateGpuBuffers();
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

        renderMeshInstances(encoder, snapshot);

        encoder->popDebugGroup();
        encoder->end();

        pContext->resourceBarrier(m_sceneColor.get(), graphics::Resource::State::ShaderResource);
    }

    auto SceneRenderer::renderMeshInstances(core::ref<graphics::RenderPassEncoder> encoder, FrameSnapshot const& snapshot) -> void
    {
        auto const shouldDumpMaterialBindings = []() -> bool
        {
            auto const* envValue = std::getenv("APRIL_SCENE_DUMP_MATERIAL_BINDINGS");
            return envValue && envValue[0] != '\0' && envValue[0] != '0';
        }();

        auto rootVar = m_vars->getRootVariable();

        // Set per-frame data
        rootVar["perFrame"]["viewProj"].setBlob(&m_viewProjectionMatrix, sizeof(float4x4));
        rootVar["perFrame"]["cameraPos"].setBlob(&m_cameraPosition, sizeof(float3));

        // Bind material data buffer
        auto* materialSystem = m_resources.getMaterialSystem();
        if (materialSystem && materialSystem->getMaterialDataBuffer())
        {
            rootVar["materials"].setBuffer(materialSystem->getMaterialDataBuffer());
        }

        for (uint32_t textureIndex = 0; textureIndex < kMaterialTextureTableSize; ++textureIndex)
        {
            auto texture = m_defaultWhiteTexture;
            if (materialSystem && textureIndex != 0)
            {
                if (auto registered = materialSystem->getTextureDescriptorResource(textureIndex))
                {
                    texture = registered;
                }
            }
            rootVar["materialTextures"][textureIndex].setTexture(texture);
        }

        for (uint32_t samplerIndex = 0; samplerIndex < kMaterialSamplerTableSize; ++samplerIndex)
        {
            auto sampler = m_defaultSampler;
            if (materialSystem && samplerIndex != 0)
            {
                if (auto registered = materialSystem->getSamplerDescriptorResource(samplerIndex))
                {
                    sampler = registered;
                }
            }
            rootVar["materialSamplers"][samplerIndex].setSampler(sampler);
        }

        auto warnMissingSlot = [&](RenderID meshId, uint32_t slotIndex)
        {
            if (meshId == kInvalidRenderID)
            {
                return;
            }

            auto const key = (static_cast<uint64_t>(meshId) << 32) | slotIndex;
            if (m_missingMaterialSlots.insert(key).second)
            {
                AP_WARN("[SceneRenderer] Mesh {} missing material slot {}", meshId, slotIndex);
            }
        };

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

                // Set per-instance data
                rootVar["perInstance"]["model"].setBlob(&instance.worldTransform, sizeof(float4x4));

                encoder->setVao(mesh->getVAO());
                encoder->bindPipeline(m_pipeline.get(), m_vars.get());

                for (size_t s = 0; s < mesh->getSubmeshCount(); ++s)
                {
                    auto const& submesh = mesh->getSubmesh(s);
                    auto materialId = instance.materialId;
                    if (materialId == kInvalidRenderID)
                    {
                        materialId = m_resources.getMeshMaterialId(instance.meshId, submesh.materialIndex);
                        if (materialId == kInvalidRenderID)
                        {
                            warnMissingSlot(instance.meshId, submesh.materialIndex);
                        }
                    }

                    // Resolve explicit GPU material buffer index via registry mapping.
                    auto materialIndex = m_resources.resolveGpuMaterialIndex(
                        instance.meshId,
                        submesh.materialIndex,
                        materialId
                    );
                    if (materialSystem)
                    {
                        auto const materialCount = materialSystem->getMaterialCount();
                        if (materialCount == 0 || materialIndex >= materialCount)
                        {
                            auto const warningKey = (static_cast<uint64_t>(instance.meshId) << 32) |
                                                    static_cast<uint64_t>(submesh.materialIndex);
                            if (m_invalidMaterialIndices.insert(warningKey).second)
                            {
                                AP_WARN("[SceneRenderer] Invalid GPU material index {} (count={}) for mesh {}, slot {}",
                                        materialIndex, materialCount, instance.meshId, submesh.materialIndex);
                            }
                            materialIndex = m_resources.getMaterialBufferIndex(kInvalidRenderID);
                        }
                    }
                    rootVar["perInstance"]["materialIndex"].set(materialIndex);

                    if (shouldDumpMaterialBindings && !m_materialBindingDumped)
                    {
                        AP_INFO(
                            "[SceneRenderer] MaterialBinding mesh={} submesh={} slot={} materialId={} gpuMaterialIndex={}",
                            instance.meshId,
                            s,
                            submesh.materialIndex,
                            materialId,
                            materialIndex
                        );
                    }

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

        if (shouldDumpMaterialBindings)
        {
            m_materialBindingDumped = true;
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
