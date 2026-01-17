#include <core/window/window.hpp>
#include <core/log/logger.hpp>
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/swapchain.hpp>
#include <graphics/rhi/graphics-pipeline.hpp>
#include <graphics/rhi/render-target.hpp>
#include <graphics/rhi/rasterizer-state.hpp>
#include <graphics/program/program.hpp>
#include <graphics/program/program-variables.hpp>

#include <iostream>

using namespace april;
using namespace april::graphics;

const char* vs_code = R"(
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

const char* ps_code = R"(
float4 main(float4 pos : SV_Position, float4 color : COLOR) : SV_Target {
    return color;
}
)";

int main()
{
    try {
        // 1. Initialize Window
        WindowDesc windowDesc;
        windowDesc.title = "April Triangle Test";
        windowDesc.width = 1280;
        windowDesc.height = 720;
        auto window = Window::create(windowDesc);
        if (!window)
        {
            std::cerr << "Failed to create window" << std::endl;
            return -1;
        }

        // 2. Initialize Device
        Device::Desc deviceDesc;
        deviceDesc.enableDebugLayer = true;
        deviceDesc.type = Device::Type::Vulkan;
        auto device = core::make_ref<Device>(deviceDesc);

        // 3. Create Swapchain
        Swapchain::Desc swapchainDesc;
        swapchainDesc.format = ResourceFormat::RGBA8Unorm;
        swapchainDesc.width = window->getFramebufferWidth();
        swapchainDesc.height = window->getFramebufferHeight();
        swapchainDesc.imageCount = 3;
        auto swapchain = core::make_ref<Swapchain>(device, swapchainDesc, window->getNativeWindowHandle());

        // 4. Load Shaders and Create Program
        ProgramDesc progDesc;
        progDesc.addShaderModule("TriangleVS").addString(vs_code, "TriangleVS.slang");
        progDesc.vsEntryPoint("main");
        progDesc.addShaderModule("TrianglePS").addString(ps_code, "TrianglePS.slang");
        progDesc.psEntryPoint("main");

        auto program = Program::create(device, progDesc);
        auto vars = ProgramVariables::create(device, program.get());

        // 5. Create Graphics Pipeline
        GraphicsPipelineDesc pipelineDesc;
        pipelineDesc.programKernels = program->getActiveVersion()->getKernels(device.get(), nullptr);
        pipelineDesc.renderTargetCount = 1;
        pipelineDesc.renderTargetFormats[0] = rhi::Format::RGBA8Unorm;
        pipelineDesc.primitiveType = GraphicsPipelineDesc::PrimitiveType::TriangleList;

        RasterizerState::Desc rsDesc;
        rsDesc.setCullMode(RasterizerState::CullMode::None);
        pipelineDesc.rasterizerState = RasterizerState::create(rsDesc);

        auto pipeline = device->createGraphicsPipeline(pipelineDesc);

        auto ctx = device->getCommandContext();

        auto closeWindow = false;
        window->subscribe<WindowCloseEvent>([&] (WindowCloseEvent const& event){
            closeWindow = true;
        });

        bool swapchainDirty = false;
        window->subscribe<FrameBufferResizeEvent>([&](const FrameBufferResizeEvent& e) {
            if (e.width > 0 && e.height > 0)
            {
                swapchainDirty = true;
            }
        });

        // 6. Main Loop
        // For this test, we render 100 frames then exit.
        while (!closeWindow)
        {
            window->onEvent();

            // Acquire next image from swapchain
            if (swapchainDirty)
            {
                swapchain->resize(window->getFramebufferWidth(), window->getFramebufferHeight());
                swapchainDirty = false;
            }
            auto backBuffer = swapchain->acquireNextImage();
            if (!backBuffer) break;

            // Transition to RenderTarget
            ctx->resourceBarrier(backBuffer.get(), Resource::State::RenderTarget);

            // Create ColorTarget
            auto colorTarget = ColorTarget(
                backBuffer->getRTV(),
                LoadOp::Clear,
                StoreOp::Store,
                float4(0.5f, 0.1f, 0.1f, 1.0f)
            );

            // Begin Render Pass
            auto encoder = ctx->beginRenderPass({colorTarget});

            Viewport vp;
            vp.width = (float)window->getFramebufferWidth();
            vp.height = (float)window->getFramebufferHeight();
            encoder->setViewport(0, vp);
            encoder->setScissor(0, {0, 0, (uint32_t) vp.width, (uint32_t) vp.height});

            // Bind and draw
            encoder->bindPipeline(pipeline.get(), vars.get());
            encoder->draw(3, 0);
            encoder->end();

            // Transition backbuffer to Present state
            ctx->resourceBarrier(backBuffer.get(), Resource::State::Present);

            // Submit work before presenting
            ctx->submit();

            // Present the frame
            swapchain->present();

            // Advance frame count and release deferred resources
            device->endFrame();
        }

        std::cout << "Test Triangle completed successfully" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception" << std::endl;
        return 1;
    }
}
