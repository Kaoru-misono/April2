#include <core/window/window.hpp>
#include <core/log/logger.hpp>
#include <core/math/math.hpp>
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/swapchain.hpp>
#include <graphics/rhi/texture.hpp>
#include <graphics/rhi/buffer.hpp>
#include <graphics/rhi/query-heap.hpp>
#include <graphics/rhi/resource-views.hpp>
#include <graphics/rhi/graphics-pipeline.hpp>
#include <graphics/rhi/rasterizer-state.hpp>
#include <graphics/program/program.hpp>
#include <graphics/program/program-variables.hpp>

// --- Profiler Includes ---
#include <core/profile/profiler.hpp>
#include <core/profile/profile-manager.hpp>
#include <graphics/profile/gpu-profiler.hpp>
#include <core/profile/profile-exporter.hpp>
#include <thread> // For sleep

#include "ui/imgui-layer.hpp"
#include "ui/element.hpp"
#include "editor/element/element-logger.hpp"
#include "editor/element/element-profiler.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <implot.h>
#include <GLFW/glfw3.h>
#include <iostream>

using namespace april;
using namespace april::graphics;
using namespace april::ui;

// --- Shaders for Load Simulation ---
char const* vs_code = R"(
struct VSOut { float4 pos : SV_Position; float4 color : COLOR; };
VSOut main(uint vertexId : SV_VertexID) {
    VSOut output;
    float2 positions[3] = { float2(0.0, 0.5), float2(0.5, -0.5), float2(-0.5, -0.5) };
    float3 colors[3] = { float3(1.0, 0.0, 0.0), float3(0.0, 1.0, 0.0), float3(0.0, 0.0, 1.0) };
    output.pos = float4(positions[vertexId], 0.0, 1.0);
    output.color = float4(colors[vertexId], 1.0);
    return output;
}
)";

char const* ps_code = R"(
float4 main(float4 pos : SV_Position, float4 color : COLOR) : SV_Target {
    return color;
}
)";

// --- CPU Load Simulator ---
void SimulateCpuWork(char const* name, int millis)
{
    APRIL_PROFILE_ZONE(name);
    std::this_thread::sleep_for(std::chrono::milliseconds(millis));
}

class SampleElement : public IElement
{
public:
    APRIL_OBJECT(SampleElement)

    void onAttach(ImGuiLayer* pLayer) override
    {
        m_pLayer = pLayer;
        auto device = pLayer->getFontTexture()->getDevice();

        // 1. Create Offscreen Texture (RenderTarget)
        m_viewportTexture = device->createTexture2D(
            1280, 720, ResourceFormat::RGBA8Unorm, 1, 1, nullptr,
            TextureUsage::ShaderResource | TextureUsage::RenderTarget
        );
        m_viewportTexture->setName("ViewportTexture");

        // 2. Create Pipeline for Heavy Draw
        ProgramDesc progDesc;
        progDesc.addShaderModule("TriangleVS").addString(vs_code, "TriangleVS.slang");
        progDesc.vsEntryPoint("main");
        progDesc.addShaderModule("TrianglePS").addString(ps_code, "TrianglePS.slang");
        progDesc.psEntryPoint("main");
        auto program = Program::create(device, progDesc);
        m_vars = ProgramVariables::create(device, program.get());

        GraphicsPipelineDesc pipelineDesc;
        pipelineDesc.programKernels = program->getActiveVersion()->getKernels(device.get(), nullptr);
        pipelineDesc.renderTargetCount = 1;
        pipelineDesc.renderTargetFormats[0] = rhi::Format::RGBA8Unorm;
        pipelineDesc.primitiveType = GraphicsPipelineDesc::PrimitiveType::TriangleList;
        RasterizerState::Desc rsDesc;
        rsDesc.setCullMode(RasterizerState::CullMode::None);
        pipelineDesc.rasterizerState = RasterizerState::create(rsDesc);
        m_pipeline = device->createGraphicsPipeline(pipelineDesc);
    }

    void onDetach() override
    {
        m_viewportTexture.reset();
        m_pipeline.reset();
        m_vars.reset();
    }

    void onResize(CommandContext*, float2 const& size) override
    {
        // Resize texture if needed (simplified for this test)
    }

    void onUIRender() override
    {
        ImGui::Begin("Settings");
        ImGui::Checkbox("Animated Viewport", &m_animate);
        ImGui::Checkbox("Simulate Heavy Load", &m_simulateLoad);
        ImGui::SliderInt("Draw Calls", &m_drawCount, 1, 500);

        ImGui::Separator();
        ImGui::TextDisabled("FPS: %.1f", ImGui::GetIO().Framerate);

        if (m_traceExported) {
             ImGui::TextColored(ImVec4(0,1,0,1), "Trace Exported!");
        } else {
             ImGui::Text("Tracing... (Frame %d/100)", m_frameCount);
        }

        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport");
        if (m_viewportTexture)
        {
            auto srv = m_viewportTexture->getSRV();
            // Simple resize logic: if window size != texture size, recreate (omitted for brevity in test)
            ImGui::Image((ImTextureID)srv.get(), ImGui::GetContentRegionAvail());
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void onPreRender() override {}

    void onRender(CommandContext* pContext) override
    {
        // Profiling the GPU work of this Element
        APRIL_GPU_ZONE(pContext, "SampleElement Render");

        if (m_animate && m_viewportTexture && m_pipeline)
        {
            float4 clearColor;
            ImGui::ColorConvertHSVtoRGB((float)ImGui::GetTime() * 0.1f, 0.8f, 0.8f, clearColor.x, clearColor.y, clearColor.z);
            clearColor.w = 1.0f;

            auto rtv = m_viewportTexture->getRTV();
            auto colorTarget = ColorTarget(rtv, LoadOp::Clear, StoreOp::Store, clearColor);

            auto encoder = pContext->beginRenderPass({colorTarget});

            // Set dynamic state
            Viewport vp = Viewport::fromSize((float)m_viewportTexture->getWidth(), (float)m_viewportTexture->getHeight());
            encoder->setViewport(0, vp);
            encoder->setScissor(0, {0, 0, (uint32_t)vp.width, (uint32_t)vp.height});

            // Bind Pipeline
            encoder->bindPipeline(m_pipeline.get(), m_vars.get());

            // Simulate Heavy GPU Load
            if (m_simulateLoad)
            {
                APRIL_GPU_ZONE(pContext, "Heavy Draw Loop");
                for(int i=0; i < m_drawCount; ++i)
                {
                    encoder->draw(3, 0);
                }
            }
            else
            {
                 encoder->draw(3, 0);
            }

            encoder->end();
        }
    }

    void onUIMenu() override { /* ... */ }
    void onFileDrop(std::filesystem::path const&) override {}

    void notifyFrame(int frame) {
        m_frameCount = frame;
        if (frame > 100) m_traceExported = true;
    }

private:
    ImGuiLayer* m_pLayer{nullptr};
    core::ref<Texture> m_viewportTexture;
    core::ref<GraphicsPipeline> m_pipeline;
    core::ref<ProgramVariables> m_vars;

    bool m_animate{true};
    bool m_simulateLoad{true};
    int m_drawCount{100};
    int m_frameCount{0};
    bool m_traceExported{false};
};

int main()
{
    try
    {
        // 1. Initialize Window
        WindowDesc windowDesc;
        windowDesc.title = "April Engine - Profiler Integration Test";
        windowDesc.width = 1280;
        windowDesc.height = 720;
        auto window = Window::create(windowDesc);
        if (!window) return -1;

        // 2. Initialize Device
        Device::Desc deviceDesc;
        deviceDesc.type = Device::Type::D3D12; // 建议用 Vulkan 测试 Profiler
        auto device = core::make_ref<Device>(deviceDesc);

        // --- PROFILER INIT ---
        AP_INFO("Profiler Initialized");

        // 3. Create Swapchain
        Swapchain::Desc swapchainDesc;
        swapchainDesc.format = ResourceFormat::RGBA8Unorm;
        swapchainDesc.width = window->getFramebufferWidth();
        swapchainDesc.height = window->getFramebufferHeight();
        auto swapchain = core::make_ref<Swapchain>(device, swapchainDesc, window->getNativeWindowHandle());

        // 4. Initialize ImGuiLayer
        ImGuiLayerDesc layerDesc;
        layerDesc.device = device;
        layerDesc.window = window.get();

        auto imguiLayer = core::make_ref<ImGuiLayer>();
        imguiLayer->init(layerDesc);

        // 5. Add Elements
        auto sampleElement = core::make_ref<SampleElement>();
        imguiLayer->addElement(sampleElement);
        imguiLayer->addElement(core::make_ref<ElementLogger>(true));
        imguiLayer->addElement(core::make_ref<ElementProfiler>(true));

        auto ctx = device->getCommandContext();
        bool closeWindow = false;
        window->subscribe<WindowCloseEvent>([&](WindowCloseEvent const&) { closeWindow = true; });

        AP_INFO("Starting main loop");
        int frame = 0;
        bool traceExported = false;

        while (!closeWindow)
        {
            {
                // Frame Update Profile
                APRIL_PROFILE_ZONE("Frame Update");

                {
                    APRIL_PROFILE_ZONE("Window Poll");
                window->onEvent();
            }

            // CPU Load Simulation
            {
                APRIL_PROFILE_ZONE("Game Logic");
                SimulateCpuWork("Physics", 2);
                SimulateCpuWork("AI", 1);
            }

            // Logic
            {
                frame++;
                sampleElement->notifyFrame(frame);

                auto fw = window->getFramebufferWidth();
                auto fh = window->getFramebufferHeight();

                if (fw > 0 && fh > 0)
                {
                    if (swapchain->getDesc().width != fw || swapchain->getDesc().height != fh)
                    {
                        swapchain->resize(fw, fh);
                    }
                }
            }
            }

            auto fw = window->getFramebufferWidth();
            auto fh = window->getFramebufferHeight();

            if (fw > 0 && fh > 0)
            {
                auto backBuffer = swapchain->acquireNextImage();
                if (!backBuffer) continue;

                // GPU Profiling: UI Pass (includes SampleElement render)
                {
                    APRIL_GPU_ZONE(ctx, "Frame Render");

                    // ImGuiLayer calls onRender for elements inside beginFrame/endFrame
                    // So SampleElement's heavy draw happens here
                    imguiLayer->beginFrame();
                    imguiLayer->endFrame(ctx, backBuffer->getRTV());
                }

                ctx->resourceBarrier(backBuffer.get(), Resource::State::Present);

                // Submit
                {
                    APRIL_PROFILE_ZONE("Submit");
                    ctx->submit();
                }

                // Profiler & Frame End
                device->endFrame();

                {
                    APRIL_PROFILE_ZONE("Present");
                    swapchain->present();
                }
            }

            // --- Export Trace Logic ---
            if (frame == 100 && !traceExported)
            {
                traceExported = true;
                AP_INFO("Exporting trace...");

                // Flush GPU
                device->getGfxCommandQueue()->waitOnHost();

                // Flush Profiler
                auto events = core::ProfileManager::get().flush();

                // Export
                core::ProfileExporter::exportToFile("trace.json", events);
                AP_INFO("Trace exported to trace.json");
            }
        }

        imguiLayer->terminate();
    }
    catch (std::exception const& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
