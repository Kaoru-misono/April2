#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <core/window/window.hpp>
#include <graphics/rhi/render-device.hpp>
#include "imgui-backend.hpp"
#include <GLFW/glfw3.h>

using namespace april;
using namespace april::graphics;
using namespace april::editor;

TEST_CASE("ImGui Font Texture Creation") {
    // 1. Initialize Windowing
    if (!glfwInit()) {
        FAIL("Failed to initialize GLFW");
    }

    WindowDesc windowDesc;
    windowDesc.title = "Test Window";
    windowDesc.width = 100;
    windowDesc.height = 100;
    auto window = Window::create(windowDesc);

    // 2. Initialize Device
    Device::Desc deviceDesc;
    deviceDesc.type = Device::Type::Vulkan;
    auto device = core::make_ref<Device>(deviceDesc);

    // 3. Initialize ImGui Backend
    ImGuiBackendDesc backendDesc;
    backendDesc.device = device;
    backendDesc.window = window.get();

    auto imguiBackend = core::make_ref<ImGuiBackend>();
    imguiBackend->init(backendDesc);

    // 4. Verify Font Texture
    // This is expected to FAIL initially because we haven't implemented the creation logic yet.
    auto fontTexture = imguiBackend->getFontTexture();

    CHECK(fontTexture != nullptr);
    if (fontTexture) {
        CHECK(fontTexture->getWidth() > 0);
        CHECK(fontTexture->getHeight() > 0);
    }

    imguiBackend->terminate();
    glfwTerminate();
}
