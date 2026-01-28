#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <slang-rhi.h>
#include <slang-com-ptr.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <print>

char const* kSlangCode = R"(
struct VSOut {
    float4 pos : SV_Position;
    float4 color : COLOR;
};

[shader("vertex")]
VSOut vertexMain(uint vertexId : SV_VertexID) {
    VSOut output;
    float2 positions[3] = { float2(0.0, 0.5), float2(0.5, -0.5), float2(-0.5, -0.5) };
    float3 colors[3] = { float3(1.0, 0.0, 0.0), float3(0.0, 1.0, 0.0), float3(0.0, 0.0, 1.0) };

    output.pos = float4(positions[vertexId], 0.0, 1.0);
    output.color = float4(colors[vertexId], 1.0);
    return output;
}

[shader("fragment")]
float4 fragmentMain(float4 pos : SV_Position, float4 color : COLOR) : SV_Target {
    return color;
}
)";

auto debugMessageSourceToString(rhi::DebugMessageSource source) -> std::string
{
    switch (source)
    {
    case rhi::DebugMessageSource::Layer:  return "[Layer]";
    case rhi::DebugMessageSource::Driver: return "[Driver]";
    case rhi::DebugMessageSource::Slang:  return "[Slang]";
    default: return "[Unknown]";
    }
}

class GFXDebugCallBack final: public rhi::IDebugCallback
{
    virtual SLANG_NO_THROW auto SLANG_MCALL
    handleMessage(rhi::DebugMessageType type, rhi::DebugMessageSource source, char const* message) -> void override
    {
        if (type == rhi::DebugMessageType::Error)
        {
            std::println("{}: {}", debugMessageSourceToString(source), message);
        }
        else if (type == rhi::DebugMessageType::Warning)
        {
            std::println("{}: {}", debugMessageSourceToString(source), message);
        }
        else
        {
            std::println("{}: {}", debugMessageSourceToString(source), message);
        }
    }
};

int main() {
    std::cout << "Initializing..." << std::endl;
    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Slang-RHI Triangle", nullptr, nullptr);
    if (!window) return 1;

    HWND hwnd = glfwGetWin32Window(window);

    auto callback = GFXDebugCallBack{};

    Slang::ComPtr<rhi::IDevice> device;
    rhi::DeviceDesc deviceDesc = {};
    deviceDesc.deviceType = rhi::DeviceType::D3D12;
    deviceDesc.enableValidation = true;
    deviceDesc.debugCallback = &callback;

    if (SLANG_FAILED(rhi::getRHI()->createDevice(deviceDesc, device.writeRef()))) {
        std::cerr << "Failed to create D3D12 device" << std::endl;
        return 1;
    }

    std::cout << "Created device: " << device->getInfo().apiName << std::endl;

    Slang::ComPtr<rhi::ICommandQueue> queue;
    device->getQueue(rhi::QueueType::Graphics, queue.writeRef());

    Slang::ComPtr<rhi::ISurface> surface;
    rhi::WindowHandle windowHandle = rhi::WindowHandle::fromHwnd(hwnd);
    if (SLANG_FAILED(device->createSurface(windowHandle, surface.writeRef()))) return 1;

    rhi::SurfaceConfig surfaceConfig = {};
    surfaceConfig.format = rhi::Format::RGBA8Unorm;
    surfaceConfig.width = 800;
    surfaceConfig.height = 600;
    surfaceConfig.desiredImageCount = 3;
    surfaceConfig.vsync = true;
    surface->configure(surfaceConfig);

    rhi::InputLayoutDesc inputLayoutDesc = {};
    inputLayoutDesc.inputElementCount = 0;
    inputLayoutDesc.vertexStreamCount = 0;
    Slang::ComPtr<rhi::IInputLayout> inputLayout;
    device->createInputLayout(inputLayoutDesc, inputLayout.writeRef());

    Slang::ComPtr<slang::IGlobalSession> slangSession;
    slang::createGlobalSession(slangSession.writeRef());

    Slang::ComPtr<slang::ISession> session;
    slang::SessionDesc sessionDesc = {};
    slang::TargetDesc targetDesc = {};
    targetDesc.format = SLANG_DXIL;
    if (device->getInfo().deviceType == rhi::DeviceType::Vulkan) targetDesc.format = SLANG_SPIRV;
    targetDesc.profile = slangSession->findProfile("sm_6_0");
    sessionDesc.targets = &targetDesc;
    sessionDesc.targetCount = 1;
    slangSession->createSession(sessionDesc, session.writeRef());

    Slang::ComPtr<slang::IBlob> diagnosticBlob;
    slang::IModule* module = session->loadModuleFromSourceString("triangle", "triangle.slang", kSlangCode, diagnosticBlob.writeRef());
    if (!module) return 1;

    Slang::ComPtr<slang::IEntryPoint> vsEntry, psEntry;
    module->findEntryPointByName("vertexMain", vsEntry.writeRef());
    module->findEntryPointByName("fragmentMain", psEntry.writeRef());

    std::vector<slang::IComponentType*> components = { module, vsEntry, psEntry };
    Slang::ComPtr<slang::IComponentType> linkedProgram;
    session->createCompositeComponentType(components.data(), components.size(), linkedProgram.writeRef());

    rhi::ShaderProgramDesc programDesc = {};
    programDesc.linkingStyle = rhi::LinkingStyle::SingleProgram;
    programDesc.slangGlobalScope = linkedProgram;

    Slang::ComPtr<rhi::IShaderProgram> shaderProgram;
    Slang::ComPtr<ISlangBlob> diagnostics;
    if (SLANG_FAILED(device->createShaderProgram(programDesc, shaderProgram.writeRef(), diagnostics.writeRef()))) {
        if(diagnostics) std::cerr << (char const*)diagnostics->getBufferPointer() << std::endl;
        return 1;
    }

    rhi::ColorTargetDesc colorTargetDesc = {};
    colorTargetDesc.format = rhi::Format::RGBA8Unorm;

    rhi::RenderPipelineDesc pipelineDesc = {};
    pipelineDesc.program = shaderProgram.get();
    pipelineDesc.inputLayout = inputLayout.get();
    pipelineDesc.targetCount = 1;
    pipelineDesc.targets = &colorTargetDesc;
    pipelineDesc.primitiveTopology = rhi::PrimitiveTopology::TriangleList;
    pipelineDesc.depthStencil.format = rhi::Format::Undefined;
    pipelineDesc.rasterizer.cullMode = rhi::CullMode::None;

    std::cout << "Creating Render Pipeline..." << std::endl;
    Slang::ComPtr<rhi::IRenderPipeline> pipeline;
    if (SLANG_FAILED(device->createRenderPipeline(pipelineDesc, pipeline.writeRef()))) {
        std::cerr << "Failed to create render pipeline" << std::endl;
        return 1;
    }

    std::cout << "Starting Loop..." << std::endl;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        Slang::ComPtr<rhi::ITexture> backBuffer;
        surface->acquireNextImage(backBuffer.writeRef());
        if (!backBuffer) continue;

        auto encoder = queue->createCommandEncoder();

        rhi::SubresourceRange subresource = {};
        subresource.layerCount = 1;
        subresource.mipCount = 1;

        encoder->setTextureState(backBuffer, subresource, rhi::ResourceState::RenderTarget);

        rhi::RenderPassColorAttachment colorAttachment = {};
        Slang::ComPtr<rhi::ITextureView> rtv;
        rhi::TextureViewDesc viewDesc = {};
        viewDesc.format = rhi::Format::RGBA8Unorm;
        viewDesc.subresourceRange = subresource;
        device->createTextureView(backBuffer, viewDesc, rtv.writeRef());
        colorAttachment.view = rtv;
        colorAttachment.loadOp = rhi::LoadOp::Clear;
        colorAttachment.storeOp = rhi::StoreOp::Store;
        colorAttachment.clearValue[0] = 0.1f;
        colorAttachment.clearValue[1] = 0.1f;
        colorAttachment.clearValue[2] = 0.1f;
        colorAttachment.clearValue[3] = 1.0f;

        rhi::RenderPassDesc renderPass = {};
        renderPass.colorAttachments = &colorAttachment;
        renderPass.colorAttachmentCount = 1;

        rhi::IRenderPassEncoder* passEncoder = encoder->beginRenderPass(renderPass);

        rhi::Viewport viewport = {};
        viewport.extentX = 800;
        viewport.extentY = 600;
        viewport.maxZ = 1.0f;

        rhi::ScissorRect scissor = {};
        scissor.maxX = 800;
        scissor.maxY = 600;

        rhi::RenderState renderState = {};
        renderState.viewportCount = 1;
        renderState.viewports[0] = viewport;
        renderState.scissorRectCount = 1;
        renderState.scissorRects[0] = scissor;

        passEncoder->setRenderState(renderState);
        passEncoder->bindPipeline(pipeline);
        passEncoder->draw(rhi::DrawArguments{3, 1, 0, 0});
        passEncoder->end();

        encoder->setTextureState(backBuffer, subresource, rhi::ResourceState::Present);

        Slang::ComPtr<rhi::ICommandBuffer> cmdBuf;
        encoder->finish(cmdBuf.writeRef());
        rhi::SubmitDesc submitDesc = {};
        rhi::ICommandBuffer* cmdBufs[] = { cmdBuf };
        submitDesc.commandBuffers = cmdBufs;
        submitDesc.commandBufferCount = 1;
        queue->submit(submitDesc);

        surface->present();
        device->waitForFences(0, nullptr, nullptr, true, 100000);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
