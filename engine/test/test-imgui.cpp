#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <graphics/ui/imgui-layer.hpp>
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/resource-views.hpp>
#include <graphics/rhi/swapchain.hpp>
#include <graphics/rhi/render-target.hpp>
#include <graphics/rhi/rasterizer-state.hpp>
#include <graphics/program/program.hpp>
#include <graphics/program/program-variables.hpp>
#include <core/window/window.hpp>
#include <GLFW/glfw3.h>
#include <iostream>
#include <imgui.h>

using namespace april;
using namespace april::graphics;

const char* tri_vs_code = R"(
struct VSOut {
    float4 pos : SV_Position;
    float4 color : COLOR;
};

VSOut main(uint vertexId : SV_VertexID) {
    VSOut output;
    float2 positions[3] = { float2(0.0, 0.5), float2(0.5, -0.5), float2(-0.5, -0.5) };
    float3 colors[3] = { float3(1.0, 0.0, 0.0), float3(0.0, 1.0, 0.0), float3(0.0, 0.0, 1.0) };

    output.pos = float4(positions[vertexId], 0.0, 1.0);
    output.color = float4(colors[vertexId], 1.0);
    return output;
}
)";

const char* tri_ps_code = R"(
float4 main(float4 pos : SV_Position, float4 color : COLOR) : SV_Target {
    return color;
}
)";

void runImGuiTest(Device::Type deviceType)
{
    REQUIRE(glfwInit());

    WindowDesc windowDesc;
    windowDesc.width = 1280;
    windowDesc.height = 720;
    windowDesc.title = "ImGui Test";
    auto window = Window::create(windowDesc);
    REQUIRE(window);

    Device::Desc deviceDesc;
    deviceDesc.type = deviceType;
    deviceDesc.enableDebugLayer = true;
    
    std::vector<AdapterInfo> gpus = Device::getGPUs(deviceType);
    if (gpus.empty()) {
        std::cout << "Skipping test for " << (deviceType == Device::Type::D3D12 ? "D3D12" : "Vulkan") << " as no compatible GPUs were found." << std::endl;
        window.reset();
        glfwTerminate();
        return;
    }

    auto device = core::make_ref<Device>(deviceDesc);
    REQUIRE(device);

    ProgramDesc triProgDesc;
    triProgDesc.addShaderModule("TriangleVS").addString(tri_vs_code, "TriangleVS.slang");
    triProgDesc.vsEntryPoint("main");
    triProgDesc.addShaderModule("TrianglePS").addString(tri_ps_code, "TrianglePS.slang");
    triProgDesc.psEntryPoint("main");

    auto triProgram = Program::create(device, triProgDesc);
    auto triVars = ProgramVariables::create(device, triProgram.get());

    GraphicsPipelineDesc triPipeDesc;
    triPipeDesc.programKernels = triProgram->getActiveVersion()->getKernels(device.get(), nullptr);
    triPipeDesc.renderTargetCount = 1;
    triPipeDesc.renderTargetFormats[0] = rhi::Format::RGBA8Unorm;
    triPipeDesc.primitiveType = GraphicsPipelineDesc::PrimitiveType::TriangleList;

    RasterizerState::Desc rsDesc;
    rsDesc.setCullMode(RasterizerState::CullMode::None);
    triPipeDesc.rasterizerState = RasterizerState::create(rsDesc);

    auto triPipeline = device->createGraphicsPipeline(triPipeDesc);

    Swapchain::Desc swapchainDesc;
    swapchainDesc.format = ResourceFormat::RGBA8Unorm;
    swapchainDesc.width = window->getFramebufferWidth();
    swapchainDesc.height = window->getFramebufferHeight();
    swapchainDesc.imageCount = 3;
    auto swapchain = core::make_ref<Swapchain>(device, swapchainDesc, window->getNativeWindowHandle());

    auto ctx = device->getCommandContext();
    auto closeWindow = false;
    window->subscribe<WindowCloseEvent>([&] (WindowCloseEvent const& event){
        closeWindow = true;
    });

    bool swapchainDirty = false;
    window->subscribe<FrameBufferResizeEvent>([&](const FrameBufferResizeEvent& e) {
        if (e.width > 0 && e.height > 0) {
            swapchainDirty = true;
        }
    });

    {
        ImGuiLayer imguiLayer(*window, device);

        auto viewportTexture = device->createTexture2D(1280, 720, ResourceFormat::RGBA8Unorm, 1, 1, nullptr, ResourceBindFlags::RenderTarget | ResourceBindFlags::ShaderResource);
        viewportTexture->setName("ViewportTexture");
        auto viewportRTV = viewportTexture->getRTV();
        auto viewportSRV = viewportTexture->getSRV();

        std::cout << "Starting ImGui loop for " << device->getInfo().apiName << ". Close window to proceed." << std::endl;

        while (!closeWindow)
        {
            window->onEvent();

            if (swapchainDirty)
            {
                swapchain->resize(window->getFramebufferWidth(), window->getFramebufferHeight());
                swapchainDirty = false;
            }

            ctx->resourceBarrier(viewportTexture.get(), Resource::State::RenderTarget);
            
            auto triColorTarget = ColorTarget(viewportRTV, LoadOp::Clear, StoreOp::Store, float4(0.1f, 0.2f, 0.4f, 1.0f));
            auto triEncoder = ctx->beginRenderPass({triColorTarget});
            triEncoder->setViewport(0, Viewport::fromSize(1280, 720));
            triEncoder->setScissor(0, {0, 0, 1280, 720});
            triEncoder->bindPipeline(triPipeline.get(), triVars.get());
            triEncoder->draw(3, 0);
            triEncoder->end();
            
            auto backBuffer = swapchain->acquireNextImage();
            if (!backBuffer) break;

            ctx->resourceBarrier(backBuffer.get(), Resource::State::RenderTarget);
            ctx->resourceBarrier(viewportTexture.get(), Resource::State::ShaderResource);

            imguiLayer.setViewportTexture(viewportSRV);
            imguiLayer.begin();
            
            ImGui::Begin("Test Controls");
            ImGui::Text("API: %s", device->getInfo().apiName.c_str());
            
            float xscale, yscale;
            glfwGetWindowContentScale((GLFWwindow*)window->getBackendWindow(), &xscale, &yscale);
            ImGui::Text("DPI Scale: %.2f", xscale);

            ImGui::Separator();
            ImGui::Text("Sharpness Test: %s", "The quick brown fox jumps over the lazy dog.");
            
            static float fontScale = 1.0f;
            if (ImGui::SliderFloat("Font Scale", &fontScale, 0.5f, 3.0f))
            {
                ImGui::GetIO().FontGlobalScale = fontScale;
            }
            
            if (ImGui::Button("Apply (Sharp Rebuild)"))
            {
                imguiLayer.setFontScale(fontScale);
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset"))
            {
                fontScale = 1.0f;
                imguiLayer.setFontScale(1.0f);
                ImGui::GetIO().FontGlobalScale = 1.0f;
            }

            if (ImGui::Button("Log Info")) { AP_INFO("Test info log from button"); }
            if (ImGui::Button("Log Error")) { AP_ERROR("Test error log from button"); }

            ImGui::End();

            auto rtv = backBuffer->getRTV();
            auto uiColorTarget = ColorTarget(rtv, LoadOp::Clear, StoreOp::Store, float4(0.0f, 0.0f, 0.0f, 1.0f));
            auto uiClearEncoder = ctx->beginRenderPass({uiColorTarget});
            uiClearEncoder->end();

            imguiLayer.end(ctx, rtv.get());

            ctx->resourceBarrier(backBuffer.get(), Resource::State::Present);
            ctx->submit();
            swapchain->present();
            device->endFrame();

            if (getenv("CI")) break;
        }
        
        std::cout << "Successfully tested ImGui on " << device->getInfo().apiName << std::endl;
    }

    window.reset();
    glfwTerminate();
}

TEST_SUITE("ImGui Visual Validation")
{
    TEST_CASE("Vulkan Backend") { runImGuiTest(Device::Type::Vulkan); }
    TEST_CASE("D3D12 Backend") { runImGuiTest(Device::Type::D3D12); }
}