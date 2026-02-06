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

#include "editor/imgui-backend.hpp"
#include "editor/window-manager.hpp"
#include "editor/editor-context.hpp"
#include "editor/tool-window.hpp"
#include "editor/window-registry.hpp"
#include "editor/window/console-window.hpp"
#include "editor/window/profiler-window.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <implot.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <utility>

using namespace april;
using namespace april::graphics;
using namespace april::editor;

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

struct SampleState
{
    bool animate{true};
    bool simulateLoad{true};
    int drawCount{100};
    int frameCount{0};
    bool traceExported{false};
};

class SampleSettingsWindow final : public ToolWindow
{
public:
    explicit SampleSettingsWindow(SampleState& state)
        : m_state(state)
    {
        open = true;
    }

    [[nodiscard]] auto title() const -> char const* override { return "Settings"; }

    auto onUIRender(EditorContext&) -> void override
    {
        if (!open)
        {
            return;
        }

        if (!ImGui::Begin(title(), &open))
        {
            ImGui::End();
            return;
        }

        ImGui::Checkbox("Animated Viewport", &m_state.animate);
        ImGui::Checkbox("Simulate Heavy Load", &m_state.simulateLoad);
        ImGui::SliderInt("Draw Calls", &m_state.drawCount, 1, 500);

        ImGui::Separator();
        ImGui::TextDisabled("FPS: %.1f", ImGui::GetIO().Framerate);

        if (m_state.traceExported)
        {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Trace Exported!");
        }
        else
        {
            ImGui::Text("Tracing... (Frame %d/100)", m_state.frameCount);
        }

        ImGui::End();
    }

private:
    SampleState& m_state;
};

class SampleViewportWindow final : public ToolWindow
{
public:
    SampleViewportWindow(core::ref<Device> device, SampleState& state)
        : m_device(std::move(device))
        , m_state(state)
    {
        open = true;
        initResources();
    }

    ~SampleViewportWindow() override
    {
        shutdownResources();
    }

    [[nodiscard]] auto title() const -> char const* override { return "Viewport"; }

    auto onUIRender(EditorContext&) -> void override
    {
        if (!open)
        {
            return;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        if (!ImGui::Begin(title(), &open))
        {
            ImGui::End();
            ImGui::PopStyleVar();
            return;
        }

        if (m_viewportTexture)
        {
            auto srv = m_viewportTexture->getSRV();
            ImGui::Image((ImTextureID)srv.get(), ImGui::GetContentRegionAvail());
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    auto render(CommandContext* pContext) -> void
    {
        if (!open)
        {
            return;
        }

        // Profiling the GPU work of this window
        APRIL_GPU_ZONE(pContext, "SampleViewport Render");

        if (m_state.animate && m_viewportTexture && m_pipeline)
        {
            float4 clearColor;
            ImGui::ColorConvertHSVtoRGB(
                static_cast<float>(ImGui::GetTime()) * 0.1f,
                0.8f,
                0.8f,
                clearColor.x,
                clearColor.y,
                clearColor.z
            );
            clearColor.w = 1.0f;

            auto rtv = m_viewportTexture->getRTV();
            auto colorTarget = ColorTarget(rtv, LoadOp::Clear, StoreOp::Store, clearColor);

            auto encoder = pContext->beginRenderPass({colorTarget});

            // Set dynamic state
            Viewport vp = Viewport::fromSize(
                static_cast<float>(m_viewportTexture->getWidth()),
                static_cast<float>(m_viewportTexture->getHeight())
            );
            encoder->setViewport(0, vp);
            encoder->setScissor(0, {0, 0, static_cast<uint32_t>(vp.width), static_cast<uint32_t>(vp.height)});

            // Bind Pipeline
            encoder->bindPipeline(m_pipeline.get(), m_vars.get());

            // Simulate Heavy GPU Load
            if (m_state.simulateLoad)
            {
                APRIL_GPU_ZONE(pContext, "Heavy Draw Loop");
                for (int i = 0; i < m_state.drawCount; ++i)
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

private:
    auto initResources() -> void
    {
        if (!m_device)
        {
            return;
        }

        // 1. Create Offscreen Texture (RenderTarget)
        m_viewportTexture = m_device->createTexture2D(
            1280,
            720,
            ResourceFormat::RGBA8Unorm,
            1,
            1,
            nullptr,
            TextureUsage::ShaderResource | TextureUsage::RenderTarget
        );
        m_viewportTexture->setName("ViewportTexture");

        // 2. Create Pipeline for Heavy Draw
        ProgramDesc progDesc;
        progDesc.addShaderModule("TriangleVS").addString(vs_code, "TriangleVS.slang");
        progDesc.vsEntryPoint("main");
        progDesc.addShaderModule("TrianglePS").addString(ps_code, "TrianglePS.slang");
        progDesc.psEntryPoint("main");
        auto program = Program::create(m_device, progDesc);
        m_vars = ProgramVariables::create(m_device, program.get());

        GraphicsPipelineDesc pipelineDesc;
        pipelineDesc.programKernels = program->getActiveVersion()->getKernels(m_device.get(), nullptr);
        pipelineDesc.renderTargetCount = 1;
        pipelineDesc.renderTargetFormats[0] = ResourceFormat::RGBA8Unorm;
        pipelineDesc.primitiveType = GraphicsPipelineDesc::PrimitiveType::TriangleList;
        RasterizerState::Desc rsDesc;
        rsDesc.setCullMode(RasterizerState::CullMode::None);
        pipelineDesc.rasterizerState = RasterizerState::create(rsDesc);
        m_pipeline = m_device->createGraphicsPipeline(pipelineDesc);
    }

    auto shutdownResources() -> void
    {
        m_viewportTexture.reset();
        m_pipeline.reset();
        m_vars.reset();
    }

    core::ref<Device> m_device{};
    core::ref<Texture> m_viewportTexture;
    core::ref<GraphicsPipeline> m_pipeline;
    core::ref<ProgramVariables> m_vars;
    SampleState& m_state;
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

        // 4. Initialize ImGui Backend + Editor Shell
        auto backendDesc = ImGuiBackendDesc{};
        backendDesc.device = device;
        backendDesc.window = window.get();

        auto imguiBackend = ImGuiBackend{};
        imguiBackend.init(backendDesc);

        auto managerDesc = WindowManagerDesc{};
        managerDesc.imguiConfigFlags = backendDesc.imguiConfigFlags;

        auto windowManager = WindowManager{};
        windowManager.init(managerDesc);

        // 5. Add Windows
        auto editorContext = EditorContext{};
        auto windows = WindowRegistry{};

        auto sampleState = SampleState{};
        auto settingsWindow = std::make_unique<SampleSettingsWindow>(sampleState);
        auto viewportWindow = std::make_unique<SampleViewportWindow>(device, sampleState);
        auto* viewportPtr = viewportWindow.get();

        windows.add(std::move(settingsWindow));
        windows.add(std::move(viewportWindow));
        windows.add(std::make_unique<ConsoleWindow>(true));
        windows.add(std::make_unique<ProfilerWindow>(true));

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
                    sampleState.frameCount = frame;

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

                // GPU Profiling: UI Pass (includes SampleViewport render)
                {
                    APRIL_GPU_ZONE(ctx, "Frame Render");

                    // Render the offscreen viewport first, then the UI.
                    if (viewportPtr)
                    {
                        viewportPtr->render(ctx);
                    }

                    imguiBackend.newFrame();
                    windowManager.beginFrame();
                    windowManager.renderWindows(editorContext, windows);
                    windowManager.endFrame();
                    ImGui::Render();
                    imguiBackend.render(ctx, backBuffer->getRTV());
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
                sampleState.traceExported = true;
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

        imguiBackend.terminate();
        windowManager.terminate();
    }
    catch (std::exception const& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
