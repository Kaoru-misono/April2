#include "slang-rhi-imgui-backend.hpp"
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/vertex-array-object.hpp>
#include <graphics/rhi/resource-views.hpp>
#include <graphics/program/shader-variable.hpp>
#include <core/math/type.hpp>
#include <core/log/logger.hpp>
#include <core/error/assert.hpp>

#include <imgui.h>

namespace april::graphics
{
    SlangRHIImGuiBackend::~SlangRHIImGuiBackend()
    {
        shutdown();
    }

    auto SlangRHIImGuiBackend::init(core::ref<Device> pDevice, float dpiScale) -> void
    {
        mp_device = pDevice;
        m_dpiScale = dpiScale;
        initResources();
        createFontsTexture();
    }

    auto SlangRHIImGuiBackend::shutdown() -> void
    {
        m_program = nullptr;
        m_vars = nullptr;
        m_pipeline = nullptr;
        m_fontTexture = nullptr;
        m_fontSampler = nullptr;
        m_layout = nullptr;
        m_frameResources.clear();
    }

    auto SlangRHIImGuiBackend::newFrame() -> void
    {
        m_frameIndex = (m_frameIndex + 1) % (uint32_t)Device::kInFlightFrameCount;
    }

    auto SlangRHIImGuiBackend::initResources() -> void
    {
        // 1. Load Shader
        m_program = Program::createGraphics(mp_device, "ui/imgui.slang", "vertexMain", "fragmentMain");
        m_vars = ProgramVariables::create(mp_device, m_program->getActiveVersion()->getReflector());

        // 2. Create Vertex Layout
        m_layout = VertexLayout::create();
        auto bufferLayout = VertexBufferLayout::create();
        bufferLayout->addElement("POSITION", 0, ResourceFormat::RG32Float, 1, 0);
        bufferLayout->addElement("TEXCOORD", (uint32_t)IM_OFFSETOF(ImDrawVert, uv), ResourceFormat::RG32Float, 1, 1);
        bufferLayout->addElement("COLOR", (uint32_t)IM_OFFSETOF(ImDrawVert, col), ResourceFormat::RGBA8Unorm, 1, 2);
        m_layout->addBufferLayout(0, bufferLayout);

        // 3. Create Pipeline
        auto pipeDesc = GraphicsPipelineDesc{};
        pipeDesc.programKernels = m_program->getActiveVersion()->getKernels(mp_device.get(), nullptr);
        pipeDesc.vertexLayout = m_layout;

        BlendState::Desc blendDesc;
        blendDesc.setRtBlend(0, true);
        blendDesc.setRtParams(0,
            BlendState::BlendOp::Add,
            BlendState::BlendOp::Add,
            BlendState::BlendFunc::SrcAlpha,
            BlendState::BlendFunc::OneMinusSrcAlpha,
            BlendState::BlendFunc::One,
            BlendState::BlendFunc::OneMinusSrcAlpha
        );
        pipeDesc.blendState = BlendState::create(blendDesc);

        RasterizerState::Desc rasterizerDesc;
        rasterizerDesc.setCullMode(RasterizerState::CullMode::None);
        rasterizerDesc.setScissorTest(true);
        pipeDesc.rasterizerState = RasterizerState::create(rasterizerDesc);

        DepthStencilState::Desc dsDesc;
        dsDesc.setDepthEnabled(false);
        dsDesc.setDepthWriteMask(false);
        pipeDesc.depthStencilState = DepthStencilState::create(dsDesc);

        pipeDesc.renderTargetCount = 1;
        pipeDesc.renderTargetFormats[0] = rhi::Format::RGBA8Unorm;

        m_pipeline = mp_device->createGraphicsPipeline(pipeDesc);

        // 4. Create Sampler
        Sampler::Desc samplerDesc = {};
        samplerDesc.minFilter = TextureFilteringMode::Linear;
        samplerDesc.magFilter = TextureFilteringMode::Linear;
        samplerDesc.addressModeU = TextureAddressingMode::Wrap;
        samplerDesc.addressModeV = TextureAddressingMode::Wrap;
        m_fontSampler = mp_device->createSampler(samplerDesc);

        m_frameResources.resize(Device::kInFlightFrameCount);
    }

    auto SlangRHIImGuiBackend::createFontsTexture() -> void
    {
        auto& io = ImGui::GetIO();

        if (io.Fonts->Fonts.empty())
        {
            ImFontConfig config;
            config.OversampleH = 3;
            config.OversampleV = 3;

            const char* fontPath = "engine/core/library/imgui/misc/fonts/Roboto-Medium.ttf";
            if (std::filesystem::exists(fontPath))
            {
                io.Fonts->AddFontFromFileTTF(fontPath, 22.0f * m_dpiScale, &config);
            }
            else
            {
                io.Fonts->AddFontDefault(&config);
            }
        }

        unsigned char* pixels;
        int width, height;

        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        m_fontTexture = mp_device->createTexture2D(width, height, ResourceFormat::RGBA8Unorm, 1, 1, pixels, ResourceBindFlags::ShaderResource);

        // Initial state for uploaded data is usually ShaderResource if created with init data,
        // but let's be explicit if our RHI supports it.
        // In April RHI, createTexture2D with init data transitions it to ShaderResource internally.

        auto srv = m_fontTexture->getSRV();
        io.Fonts->SetTexID((ImTextureID)srv.get());
    }

    auto SlangRHIImGuiBackend::renderDrawData(ImDrawData* drawData, core::ref<RenderPassEncoder> pEncoder) -> void
    {
        float viewportWidth = drawData->DisplaySize.x * drawData->FramebufferScale.x;
        float viewportHeight = drawData->DisplaySize.y * drawData->FramebufferScale.y;

        if (viewportWidth <= 0 || viewportHeight <= 0) return;

        pEncoder->setViewport(0, Viewport::fromSize(viewportWidth, viewportHeight));

        size_t totalVtxSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
        size_t totalIdxSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);

        if (totalVtxSize == 0 || totalIdxSize == 0) return;

        auto& frameRes = m_frameResources[m_frameIndex];

        // Grow buffers if needed
        if (!frameRes.vertexBuffer || frameRes.vertexCount < (uint32_t)drawData->TotalVtxCount)
        {
            frameRes.vertexCount = (uint32_t)drawData->TotalVtxCount + 5000;
            frameRes.vertexBuffer = mp_device->createBuffer(frameRes.vertexCount * sizeof(ImDrawVert), ResourceBindFlags::Vertex, MemoryType::Upload);
        }

        if (!frameRes.indexBuffer || frameRes.indexCount < (uint32_t)drawData->TotalIdxCount)
        {
            frameRes.indexCount = (uint32_t)drawData->TotalIdxCount + 10000;
            frameRes.indexBuffer = mp_device->createBuffer(frameRes.indexCount * sizeof(ImDrawIdx), ResourceBindFlags::Index, MemoryType::Upload);
        }

        {
            auto* vtxDst = (ImDrawVert*)frameRes.vertexBuffer->map();
            auto* idxDst = (ImDrawIdx*)frameRes.indexBuffer->map();

            for (int n = 0; n < drawData->CmdListsCount; n++)
            {
                const ImDrawList* cmdList = drawData->CmdLists[n];
                memcpy(vtxDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
                memcpy(idxDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
                vtxDst += cmdList->VtxBuffer.Size;
                idxDst += cmdList->IdxBuffer.Size;
            }

            frameRes.vertexBuffer->unmap();
            frameRes.indexBuffer->unmap();
        }

        auto vao = VertexArrayObject::create(
            VertexArrayObject::Topology::TriangleList,
            m_layout,
            {frameRes.vertexBuffer},
            frameRes.indexBuffer,
            sizeof(ImDrawIdx) == 2 ? ResourceFormat::R16Uint : ResourceFormat::R32Uint
        );

        pEncoder->setVao(vao);

        {
            auto scale = float2{2.0f / drawData->DisplaySize.x, 2.0f / drawData->DisplaySize.y};
            auto translate = float2{-1.0f - drawData->DisplayPos.x * scale.x, -1.0f - drawData->DisplayPos.y * scale.y};
            auto root = m_vars->getRootVariable();
            root["ubo"]["scale"] = scale;
            root["ubo"]["translate"] = translate;
            root["fontSampler"].setSampler(m_fontSampler);
        }

        pEncoder->bindPipeline(m_pipeline.get(), m_vars.get());

        int globalVtxOffset = 0;
        int globalIdxOffset = 0;
        ImVec2 clipOff = drawData->DisplayPos;

        for (int n = 0; n < drawData->CmdListsCount; n++)
        {
            const ImDrawList* cmdList = drawData->CmdLists[n];
            for (int cmd_i = 0; cmd_i < cmdList->CmdBuffer.Size; cmd_i++)
            {
                const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmd_i];
                if (pcmd->UserCallback != nullptr)
                {
                    pcmd->UserCallback(cmdList, pcmd);
                }
                else
                {
                    auto clipScale = drawData->FramebufferScale;
                    float clipMinX = (pcmd->ClipRect.x - clipOff.x) * clipScale.x;
                    float clipMinY = (pcmd->ClipRect.y - clipOff.y) * clipScale.y;
                    float clipMaxX = (pcmd->ClipRect.z - clipOff.x) * clipScale.x;
                    float clipMaxY = (pcmd->ClipRect.w - clipOff.y) * clipScale.y;

                    if (clipMinX < 0.0f) { clipMinX = 0.0f; }
                    if (clipMinY < 0.0f) { clipMinY = 0.0f; }
                    if (clipMaxX > viewportWidth) { clipMaxX = (float)viewportWidth; }
                    if (clipMaxY > viewportHeight) { clipMaxY = (float)viewportHeight; }
                    if (clipMaxX <= clipMinX || clipMaxY <= clipMinY)
                        continue;

                    Scissor scissor{};
                    scissor.offsetX = (uint32_t)(clipMinX);
                    scissor.offsetY = (uint32_t)(clipMinY);
                    scissor.extentX = (uint32_t)(clipMaxX - clipMinX);
                    scissor.extentY = (uint32_t)(clipMaxY - clipMinY);

                    pEncoder->setScissor(0, scissor);

                    auto* srvToBind = (ShaderResourceView*)pcmd->TexRef.GetTexID();
                    if (!srvToBind)
                    {
                        srvToBind = m_fontTexture->getSRV().get();
                    }

                    m_vars->getRootVariable()["fontTexture"].setSrv(core::ref<ShaderResourceView>(srvToBind));

                    pEncoder->drawIndexed(pcmd->ElemCount, pcmd->IdxOffset + globalIdxOffset, pcmd->VtxOffset + globalVtxOffset);
                }
            }
            globalIdxOffset += cmdList->IdxBuffer.Size;
            globalVtxOffset += cmdList->VtxBuffer.Size;
        }
    }
}

