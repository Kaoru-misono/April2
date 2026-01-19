#include <core/window/window.hpp>
#include <core/log/logger.hpp>
#include <core/math/math.hpp>
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/swapchain.hpp>
#include <graphics/rhi/texture.hpp>
#include <graphics/rhi/resource-views.hpp>
#include "ui/imgui-layer.hpp"
#include "ui/element.hpp"
#include "ui/element/element-logger.hpp"

#include <imgui.h>
#include <imgui_internal.h>
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
        auto device = pLayer->getFontTexture()->getDevice(); // Quick way to get device

        // Create a 1x1 viewport texture
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

    void onResize(CommandContext*, float2 const&) override
    {
        // Handle resize if needed
    }

    void onUIRender() override
    {
        ImGui::Begin("Settings");
        ImGui::Checkbox("Animated Viewport", &m_animate);
        ImGui::TextDisabled("%d FPS / %.3fms", static_cast<int>(ImGui::GetIO().Framerate), 1000.F / ImGui::GetIO().Framerate);
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

    void onUIMenu() override
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit", "Ctrl+Q"))
            {
                // How to close window from element?
                // In this simple example we'll just use a flag or assume GLFW.
            }
            ImGui::EndMenu();
        }
    }

    void onFileDrop(std::filesystem::path const&) override {}

private:
    ImGuiLayer* m_pLayer{nullptr};
    core::ref<Texture> m_viewportTexture;
    bool m_animate{true};
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

        // 4. Initialize ImGuiLayer
        ImGuiLayerDesc layerDesc;
        layerDesc.device = device;
        layerDesc.window = window.get();

        auto imguiLayer = core::make_ref<ImGuiLayer>();
        imguiLayer->init(layerDesc);

        // 5. Add Sample Element
        auto sampleElement = core::make_ref<SampleElement>();
        imguiLayer->addElement(sampleElement);

        auto elementLogger = core::make_ref<ElementLogger>(true);
        imguiLayer->addElement(elementLogger);

        auto ctx = device->getCommandContext();
        bool closeWindow = false;
        window->subscribe<WindowCloseEvent>([&](WindowCloseEvent const&) { closeWindow = true; });

        // 6. Main Loop
        AP_INFO("Starting main loop");
        int frame = 0;
        while (!closeWindow)
        {
            window->onEvent();

            if (frame++ % 60 == 0) {
                AP_INFO("Frame {}", frame);
                if (frame % 120 == 0) AP_WARN("Warning at frame {}", frame);
                if (frame % 300 == 0) AP_ERROR("Error at frame {}", frame);
            }

            auto fw = window->getFramebufferWidth();
            auto fh = window->getFramebufferHeight();

            if (fw > 0 && fh > 0)
            {
                if (swapchain->getDesc().width != fw ||
                    swapchain->getDesc().height != fh)
                {
                    AP_INFO("Resizing swapchain to {}x{}", fw, fh);
                    swapchain->resize(fw, fh);
                }

                auto backBuffer = swapchain->acquireNextImage();
                if (!backBuffer) continue;

                imguiLayer->beginFrame();
                imguiLayer->endFrame(ctx, backBuffer->getRTV());

                ctx->resourceBarrier(backBuffer.get(), Resource::State::Present);
                ctx->submit();
                swapchain->present();
                device->endFrame();
            }
        }

        imguiLayer->terminate();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
