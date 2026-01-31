#include "imgui-layer.hpp"
#include "style.hpp"
#include "font/fonts.hpp"
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/resource-views.hpp>
#include <graphics/rhi/vertex-array-object.hpp>
#include <core/window/window.hpp>
#include <core/math/type.hpp>
#include <core/log/logger.hpp>
#include <core/error/assert.hpp>

#include <imgui.h>
#include <imgui_internal.h>
#include <implot.h>
#include <backends/imgui_impl_glfw.h>
#include <GLFW/glfw3.h>

namespace april::ui
{
    ImGuiLayer::ImGuiLayer()
        : m_fontTexture(nullptr)
        , m_frameIndex(0)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
    }

    ImGuiLayer::~ImGuiLayer()
    {

    }

    auto ImGuiLayer::init(ImGuiLayerDesc const& desc) -> void
    {
        m_window = desc.window;
        mp_device = desc.device;
        m_vsync = desc.vSync;
        m_useMenubar = desc.useMenu;
        m_dockSetup = desc.dockSetup;
        m_imguiConfigFlags = desc.imguiConfigFlags;

        if(desc.hasUndockableViewport == true)
        {
            m_imguiConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        }

        // initializeImGuiContext.
        setupStyle(false);

        m_settingsHandler.setHandlerName("ImGuiLayer");
        m_settingsHandler.addImGuiHandler();

        // ImGui::LoadIniSettingsFromDisk(m_iniFilename.c_str());

        auto& io = ImGui::GetIO();
        addDefaultFont();
        io.FontDefault = getDefaultFont();
        addMonospaceFont();

        io.ConfigFlags = m_imguiConfigFlags;
        auto* glfwWindow = static_cast<GLFWwindow*>(m_window->getBackendWindow());
        if (mp_device->getType() == graphics::Device::Type::Vulkan)
        {
            ImGui_ImplGlfw_InitForVulkan(glfwWindow, true);
        }
        else
        {
            ImGui_ImplGlfw_InitForOther(glfwWindow, true);
        }

        // Create font texture
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        m_fontTexture = mp_device->createTexture2D(
            (uint32_t)width,
            (uint32_t)height,
            graphics::ResourceFormat::RGBA8Unorm,
            1,
            1,
            pixels,
            graphics::TextureUsage::ShaderResource
        );

        m_fontSampler = mp_device->getDefaultSampler();

        io.Fonts->SetTexID(m_fontTexture->getSRV().get());

        // 1. Load Shader
        using namespace graphics;
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
        pipeDesc.renderTargetFormats[0] = ResourceFormat::RGBA8Unorm;

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

    auto ImGuiLayer::terminate() -> void
    {
        for (auto& e: m_elements)
        {
            e->onDetach();
        }
        m_elements.clear();

        m_fontTexture.reset();
        m_fontSampler.reset();

        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
    }

    auto ImGuiLayer::setupImguiDock() -> void
    {
        const ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingInCentralNode;
        ImGuiID dockID = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), dockFlags);
        // Docking layout, must be done only if it doesn't exist
        if(!ImGui::DockBuilderGetNode(dockID)->IsSplitNode() && !ImGui::FindWindowByName("Viewport"))
        {
            ImGui::DockBuilderDockWindow("Viewport", dockID);  // Dock "Viewport" to  central node
            ImGui::DockBuilderGetCentralNode(dockID)->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;  // Remove "Tab" from the central node
            if(m_dockSetup)
            {
                // This override allow to create the layout of windows by default.
                m_dockSetup(dockID);
            }
            else
            {
                ImGuiID leftID = ImGui::DockBuilderSplitNode(dockID, ImGuiDir_Left, 0.2f, nullptr, &dockID);  // Split the central node
                ImGui::DockBuilderDockWindow("Settings", leftID);  // Dock "Settings" to the left node
            }
        }
    }

    auto ImGuiLayer::onViewportSizeChange(float2 size) -> void
    {
        auto scale = m_window->getWindowContentScale();
        ImGui::GetIO().FontGlobalScale *= scale.x / m_dpiScale;
        m_dpiScale = scale.x;

        m_viewportSize = size;

        // TODO: wait for queue.
        for (auto& e: m_elements)
        {
            e->onResize(mp_device->getCommandContext(), m_viewportSize);
        }
    }

    auto ImGuiLayer::renderFrame(graphics::CommandContext* pContext) -> void
    {
        for (auto& e: m_elements)
        {
            e->onUIRender();
        }

        {
            auto viewportSize = float2{};
            if (auto const viewport = ImGui::FindWindowByName("Viewport"); viewport)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                ImGui::Begin("Viewport");
                auto avail = ImGui::GetContentRegionAvail();
                viewportSize = {uint32_t(avail.x), uint32_t(avail.y)};
                ImGui::End();
                ImGui::PopStyleVar();
            }

            if (m_viewportSize.x != viewportSize.x || m_viewportSize.y != viewportSize.y)
            {
                onViewportSizeChange(viewportSize);
            }
        }

        ImGui::Render();

        for (auto& e: m_elements)
        {
            e->onPreRender();
        }

        for (auto& e: m_elements)
        {
            e->onRender(pContext);
        }
    }

    auto ImGuiLayer::renderDrawData(graphics::RenderPassEncoder* pEncoder, ImDrawData* pDrawData) -> void
    {
        using namespace graphics;
        // 2. Render
        float viewportWidth = pDrawData->DisplaySize.x * pDrawData->FramebufferScale.x;
        float viewportHeight = pDrawData->DisplaySize.y * pDrawData->FramebufferScale.y;

        if (viewportWidth <= 0 || viewportHeight <= 0) return;

        pEncoder->setViewport(0, graphics::Viewport::fromSize(viewportWidth, viewportHeight));

        size_t totalVtxSize = pDrawData->TotalVtxCount * sizeof(ImDrawVert);
        size_t totalIdxSize = pDrawData->TotalIdxCount * sizeof(ImDrawIdx);

        if (totalVtxSize == 0 || totalIdxSize == 0) return;

        auto& frameRes = m_frameResources[m_frameIndex];

        // Grow buffers if needed
        if (!frameRes.vertexBuffer || frameRes.vertexCount < (uint32_t)pDrawData->TotalVtxCount)
        {
            frameRes.vertexCount = (uint32_t)pDrawData->TotalVtxCount + 5000;
            frameRes.vertexBuffer = mp_device->createBuffer(frameRes.vertexCount * sizeof(ImDrawVert), BufferUsage::VertexBuffer, MemoryType::Upload);
        }

        if (!frameRes.indexBuffer || frameRes.indexCount < (uint32_t)pDrawData->TotalIdxCount)
        {
            frameRes.indexCount = (uint32_t)pDrawData->TotalIdxCount + 10000;
            frameRes.indexBuffer = mp_device->createBuffer(frameRes.indexCount * sizeof(ImDrawIdx), BufferUsage::IndexBuffer, MemoryType::Upload);
        }

        {
            auto* vtxDst = (ImDrawVert*)frameRes.vertexBuffer->map();
            auto* idxDst = (ImDrawIdx*)frameRes.indexBuffer->map();

            for (int n = 0; n < pDrawData->CmdListsCount; n++)
            {
                ImDrawList const* cmdList = pDrawData->CmdLists[n];
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
            auto scale = float2{
                2.0f / pDrawData->DisplaySize.x,
                2.0f / pDrawData->DisplaySize.y
            };
            auto translate = float2{
                -1.0f - pDrawData->DisplayPos.x * scale.x,
                -1.0f - pDrawData->DisplayPos.y * scale.y
            };
            auto root = m_vars->getRootVariable();
            root["ubo"]["scale"] = scale;
            root["ubo"]["translate"] = translate;
            root["fontSampler"].setSampler(m_fontSampler);
        }

        pEncoder->bindPipeline(m_pipeline.get(), m_vars.get());

        int globalVtxOffset = 0;
        int globalIdxOffset = 0;
        auto clipOff = pDrawData->DisplayPos;
        auto clipScale = pDrawData->FramebufferScale;

        for (int n = 0; n < pDrawData->CmdListsCount; n++)
        {
            ImDrawList const* cmdList = pDrawData->CmdLists[n];
            for (int cmd_i = 0; cmd_i < cmdList->CmdBuffer.Size; cmd_i++)
            {
                ImDrawCmd const* pcmd = &cmdList->CmdBuffer[cmd_i];
                if (pcmd->UserCallback != nullptr)
                {
                    pcmd->UserCallback(cmdList, pcmd);
                }
                else
                {
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
                        // Fallback to default if somehow missing, though RendererHasTextures should handle this.
                        // We don't have a default font texture anymore, so we rely on TexRef.
                        continue;
                    }

                    m_vars->getRootVariable()["fontTexture"].setSrv(core::ref<ShaderResourceView>(srvToBind));

                    pEncoder->drawIndexed(pcmd->ElemCount, pcmd->IdxOffset + globalIdxOffset, pcmd->VtxOffset + globalVtxOffset);
                }
            }
            globalIdxOffset += cmdList->IdxBuffer.Size;
            globalVtxOffset += cmdList->VtxBuffer.Size;
        }
    }

    auto ImGuiLayer::beginFrame() -> void
    {
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        setupImguiDock();

        if(m_useMenubar && ImGui::BeginMainMenuBar())
        {
            for(auto& e: m_elements)
            {
                e->onUIMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // Viewport size is updated after UI is built in renderFrame().
    }

    auto ImGuiLayer::endFrame(graphics::CommandContext* pCtx, core::ref<graphics::RenderTargetView> const& pTargetView) -> void
    {
        renderFrame(pCtx);

        if (pTargetView)
        {
            auto* drawData = ImGui::GetDrawData();
            if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f) return;

            auto colorTargets = {
                graphics::ColorTarget(pTargetView, graphics::LoadOp::Load, graphics::StoreOp::Store)
            };
            auto pEncoder = pCtx->beginRenderPass(colorTargets);
            pEncoder->pushDebugGroup("ImGui");
            renderDrawData(pEncoder.get(), drawData);
            pEncoder->popDebugGroup();
            pEncoder->end();
        }

        ImGui::EndFrame();

        if((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        m_frameIndex = (m_frameIndex + 1) % graphics::Device::kInFlightFrameCount;
    }

    auto ImGuiLayer::addElement(core::ref<IElement> const& element) -> void
    {
        m_elements.emplace_back(element)->onAttach(this);
    }
}
