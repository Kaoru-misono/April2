#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <graphics/ui/slang-rhi-imgui-backend.hpp>
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/resource-views.hpp>
#include <core/window/window.hpp>
#include <GLFW/glfw3.h>
#include <imgui.h>

using namespace april;
using namespace april::graphics;

TEST_CASE("SlangRHIImGuiBackend Lifecycle")
{
    REQUIRE(glfwInit());
    
    WindowDesc windowDesc;
    windowDesc.width = 100;
    windowDesc.height = 100;
    windowDesc.title = "Backend Test";
    auto window = Window::create(windowDesc);
    REQUIRE(window);

    Device::Desc deviceDesc;
    // Try Vulkan first
    std::vector<AdapterInfo> gpus = Device::getGPUs(Device::Type::Vulkan);
    if (!gpus.empty()) {
        deviceDesc.type = Device::Type::Vulkan;
    } else {
        gpus = Device::getGPUs(Device::Type::D3D12);
        if (!gpus.empty()) {
            deviceDesc.type = Device::Type::D3D12;
        } else {
            MESSAGE("No compatible GPU found, skipping test");
            window.reset();
            glfwTerminate();
            return;
        }
    }

    auto device = core::make_ref<Device>(deviceDesc);
    REQUIRE(device);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    SlangRHIImGuiBackend backend;
    backend.init(device);

    auto& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(100, 100);

    backend.newFrame();
    
    ImGui::NewFrame();
    ImGui::Begin("Test Window");
    ImGui::Text("Testing SlangRHIImGuiBackend");
    ImGui::End();
    ImGui::Render();

    auto dummyTexture = device->createTexture2D(100, 100, ResourceFormat::RGBA8Unorm, 1, 1, nullptr, ResourceBindFlags::RenderTarget);
    auto rtv = dummyTexture->getRTV();
    auto context = device->getCommandContext();
    
    ColorTargets colorTargets = {
        ColorTarget(core::ref<RenderTargetView>(rtv.get()), LoadOp::Clear, StoreOp::Store)
    };
    
    auto encoder = context->beginRenderPass(colorTargets);
    backend.renderDrawData(ImGui::GetDrawData(), encoder);
    encoder->end();
    
    backend.shutdown();
    ImGui::DestroyContext();
    window.reset();
    glfwTerminate();
}
