#include <core/window/window.hpp>
#include <core/log/logger.hpp>
#include <core/math/math.hpp>
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/swapchain.hpp>
#include <graphics/rhi/texture.hpp>
#include <graphics/rhi/buffer.hpp>      // [新增] Buffer
#include <graphics/rhi/query-heap.hpp>  // [新增] QueryHeap
#include <graphics/rhi/resource-views.hpp>
#include "ui/imgui-layer.hpp"
#include "ui/element.hpp"
#include "ui/element/element-logger.hpp"
#include "ui/element/element-profiler.hpp"
#include <core/profile/profiler.hpp>

#include <imgui.h>
#include <imgui_internal.h>
#include <implot.h>
#include <GLFW/glfw3.h>
#include <iostream>

using namespace april;
using namespace april::graphics;
using namespace april::ui;

class SampleElement : public IElement
{
public:
    APRIL_OBJECT(SampleElement)
    void onAttach(ImGuiLayer* pLayer) override
    {
        m_pLayer = pLayer;
        auto device = pLayer->getFontTexture()->getDevice();

        m_viewportTexture = device->createTexture2D(
            1, 1, ResourceFormat::RGBA32Float, 1, 1, nullptr,
            ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget
        );
        m_viewportTexture->setName("ViewportTexture");
    }

    void onDetach() override
    {
        m_viewportTexture.reset();
    }

    void onResize(CommandContext*, float2 const&) override {}

    void onUIRender() override
    {
        ImGui::Begin("Settings");
        ImGui::Checkbox("Animated Viewport", &m_animate);
        ImGui::TextDisabled("CPU: %d FPS / %.3fms", static_cast<int>(ImGui::GetIO().Framerate), 1000.F / ImGui::GetIO().Framerate);

        if (m_lastGpuTime > 0.0f) {
            ImGui::TextColored(ImVec4(0,1,0,1), "GPU: %.3f ms", m_lastGpuTime);
        } else {
            ImGui::TextDisabled("GPU: Collecting...");
        }

        ImGui::End();

        ImGui::Begin("Viewport");
        if (m_viewportTexture)
        {
            auto srv = m_viewportTexture->getSRV();
            ImGui::Image((ImTextureID)srv.get(), ImGui::GetContentRegionAvail());
        }
        ImGui::End();
    }

    void onPreRender() override {}

    void onRender(CommandContext* pContext) override
    {
        if (m_animate && m_viewportTexture)
        {
            float4 clearColor;
            ImGui::ColorConvertHSVtoRGB((float)ImGui::GetTime() * 0.05f, 1.0f, 1.0f, clearColor.x, clearColor.y, clearColor.z);
            clearColor.w = 1.0f;

            auto rtv = m_viewportTexture->getRTV();
            auto colorTarget = ColorTarget(rtv, LoadOp::Clear, StoreOp::Store, clearColor);

            auto encoder = pContext->beginRenderPass({colorTarget});
            encoder->end();
        }
    }

    void onUIMenu() override { /* ... */ }
    void onFileDrop(std::filesystem::path const&) override {}

    void setGpuTime(double ms) { m_lastGpuTime = ms; }

private:
    ImGuiLayer* m_pLayer{nullptr};
    core::ref<Texture> m_viewportTexture;
    bool m_animate{true};
    double m_lastGpuTime{0.0};
};

int main()
{
    try
    {
        // 1. Initialize Window
        WindowDesc windowDesc;
        windowDesc.title = "April Engine - NV Template Style";
        windowDesc.width = 1280;
        windowDesc.height = 720;
        auto window = Window::create(windowDesc);
        if (!window) return -1;

        // 2. Initialize Device
        Device::Desc deviceDesc;
        deviceDesc.type = Device::Type::D3D12;
        auto device = core::make_ref<Device>(deviceDesc);

        // 3. Create Swapchain
        Swapchain::Desc swapchainDesc;
        swapchainDesc.format = ResourceFormat::RGBA8Unorm;
        swapchainDesc.width = window->getFramebufferWidth();
        swapchainDesc.height = window->getFramebufferHeight();
        auto swapchain = core::make_ref<Swapchain>(device, swapchainDesc, window->getNativeWindowHandle());

        auto queryHeap = QueryHeap::create(device, QueryHeap::Type::Timestamp, 2);
        auto timestampBuffer = device->createBuffer(
            sizeof(uint64_t) * 2,
            ResourceBindFlags::None,
            MemoryType::ReadBack
        );
        timestampBuffer->setName("Timestamp Readback");

        // 4. Initialize ImGuiLayer
        ImGuiLayerDesc layerDesc;
        layerDesc.device = device;
        layerDesc.window = window.get();

        auto imguiLayer = core::make_ref<ImGuiLayer>();
        imguiLayer->init(layerDesc);

        ImPlot::CreateContext(); // Fix crash

        // 5. Add Elements
        auto sampleElement = core::make_ref<SampleElement>();
        imguiLayer->addElement(sampleElement);
        imguiLayer->addElement(core::make_ref<ElementLogger>(true));

        // april::core::GlobalProfiler::init("TestProfile");
        // imguiLayer->addElement(core::make_ref<ElementProfiler>());

        auto ctx = device->getCommandContext();
        bool closeWindow = false;
        window->subscribe<WindowCloseEvent>([&](WindowCloseEvent const&) { closeWindow = true; });

        AP_INFO("Starting main loop");
        int frame = 0;

        while (!closeWindow)
        {
            // april::core::GlobalProfiler::getTimeline()->frameAdvance();
            // AP_PROFILE_SCOPE("Frame");

            window->onEvent();

            ctx->writeTimestamp(queryHeap.get(), 0);

            // Logic
            if (frame % 60 == 0) AP_INFO("Frame {}", frame);
            frame++;

            auto fw = window->getFramebufferWidth();
            auto fh = window->getFramebufferHeight();

            if (fw > 0 && fh > 0)
            {
                if (swapchain->getDesc().width != fw || swapchain->getDesc().height != fh)
                {
                    swapchain->resize(fw, fh);
                }

                auto backBuffer = swapchain->acquireNextImage();
                if (!backBuffer) continue;

                imguiLayer->beginFrame();
                imguiLayer->endFrame(ctx, backBuffer->getRTV());

                ctx->resourceBarrier(backBuffer.get(), Resource::State::Present);

                ctx->writeTimestamp(queryHeap.get(), 1);

                ctx->resolveQuery(queryHeap.get(), 0, 2, timestampBuffer.get(), 0);

                ctx->submit();
                swapchain->present();
                device->endFrame();

                uint64_t* pData = reinterpret_cast<uint64_t*>(timestampBuffer->map());
                if (pData)
                {
                    uint64_t startTick = pData[0];
                    uint64_t endTick = pData[1];

                    if (endTick > startTick)
                    {
                        double gpuMs = double(endTick - startTick) * 1000.0;
                        sampleElement->setGpuTime(gpuMs);
                    }
                    timestampBuffer->unmap();
                }
            }
        }

        ImPlot::DestroyContext();
        imguiLayer->terminate();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
