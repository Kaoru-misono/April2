#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <ranges> // C++20

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <slang.h>
#include <slang-com-ptr.h>

// --- Constants ---
constexpr uint32_t WIDTH                = 1280;
constexpr uint32_t HEIGHT               = 720;
constexpr int      MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<char const *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

const std::vector<const char *> requiredDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

// --- Shader Source ---
char const *const kShaderSource = R"(
struct VSOut {
    float4 pos : SV_Position;
    float4 color : COLOR;
};

[shader("vertex")]
VSOut vertexMain(uint vertexId : SV_VertexID) {
    VSOut output;
    static const float2 positions[3] = {
        float2(0.0, 0.5),
        float2(0.5, -0.5),
        float2(-0.5, -0.5)
    };
    static const float3 colors[3] = {
        float3(1.0, 0.0, 0.0),
        float3(0.0, 1.0, 0.0),
        float3(0.0, 0.0, 1.0)
    };

    output.pos = float4(positions[vertexId], 0.0, 1.0);
    output.color = float4(colors[vertexId], 1.0);
    return output;
}

[shader("fragment")]
float4 fragmentMain(float4 pos : SV_Position, float4 color : COLOR) : SV_Target {
    return color;
}
)";

// --- Slang Compiler Helper ---
struct SlangShaderCompiler
{
    explicit SlangShaderCompiler(std::string const &source)
    {
        if (SLANG_FAILED(slang::createGlobalSession(m_globalSession.writeRef())))
            throw std::runtime_error("failed to create Slang global session");

        slang::TargetDesc targetDesc{};
        targetDesc.format  = SLANG_SPIRV;
        targetDesc.profile = m_globalSession->findProfile("sm_6_0");
        if (!targetDesc.profile) throw std::runtime_error("Slang profile sm_6_0 not found");

        slang::SessionDesc sessionDesc{};
        sessionDesc.targets     = &targetDesc;
        sessionDesc.targetCount = 1;

        if (SLANG_FAILED(m_globalSession->createSession(sessionDesc, m_session.writeRef())))
            throw std::runtime_error("failed to create Slang session");

        Slang::ComPtr<slang::IBlob> diagnostics;
        auto module = m_session->loadModuleFromSourceString("shader", "shader.slang", source.c_str(), diagnostics.writeRef());

        if (!module)
            throw std::runtime_error(std::string("failed to compile Slang module: ") + blobToString(diagnostics));

        m_module = module;
    }

    auto compileEntryPoint(char const *name) -> std::vector<uint32_t>
    {
        Slang::ComPtr<slang::IEntryPoint> entryPoint;
        if (SLANG_FAILED(m_module->findEntryPointByName(name, entryPoint.writeRef())))
            throw std::runtime_error(std::string("entry point not found: ") + name);

        slang::IComponentType* components[] = { m_module, entryPoint };
        Slang::ComPtr<slang::IComponentType> composedProgram;
        Slang::ComPtr<slang::IBlob> diagnostics;

        if (SLANG_FAILED(m_session->createCompositeComponentType(components, 2, composedProgram.writeRef(), diagnostics.writeRef())))
            throw std::runtime_error(std::string("failed to compose program: ") + blobToString(diagnostics));

        Slang::ComPtr<ISlangBlob> code;
        if (SLANG_FAILED(composedProgram->getEntryPointCode(0, 0, code.writeRef(), diagnostics.writeRef())))
             throw std::runtime_error(std::string("failed to generate SPIR-V for: ") + name + " - " + blobToString(diagnostics));

        auto const byteCount = code->getBufferSize();
        if (byteCount % sizeof(uint32_t) != 0) throw std::runtime_error("SPIR-V blob size error");

        auto const *const data = reinterpret_cast<uint8_t const *>(code->getBufferPointer());
        std::vector<uint32_t> result(byteCount / sizeof(uint32_t));
        std::memcpy(result.data(), data, byteCount);
        return result;
    }

private:
    static auto blobToString(Slang::ComPtr<slang::IBlob> const &blob) -> std::string {
        if (!blob) return {};
        return std::string((char const *)blob->getBufferPointer(), (char const *)blob->getBufferPointer() + blob->getBufferSize());
    }
    Slang::ComPtr<slang::IGlobalSession> m_globalSession;
    Slang::ComPtr<slang::ISession>       m_session;
    Slang::ComPtr<slang::IModule>        m_module;
};

// --- Main Application Class ---
class SwapchainResizeTest
{
public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window = nullptr;
    vk::raii::Context                    context;
    vk::raii::Instance                   instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT     debugMessenger = nullptr;
    vk::raii::SurfaceKHR                 surface = nullptr;
    vk::raii::PhysicalDevice             physicalDevice = nullptr;
    vk::raii::Device                     device = nullptr;

    uint32_t                             queueIndex = ~0u;

    vk::raii::Queue                      queue = nullptr;
    vk::raii::SwapchainKHR               swapChain = nullptr;
    std::vector<vk::Image>               swapChainImages;
    vk::SurfaceFormatKHR                 swapChainSurfaceFormat;
    vk::Extent2D                         swapChainExtent;
    std::vector<vk::raii::ImageView>     swapChainImageViews;
    vk::raii::PipelineLayout             pipelineLayout = nullptr;
    vk::raii::Pipeline                   graphicsPipeline = nullptr;
    vk::raii::CommandPool                commandPool = nullptr;
    std::vector<vk::raii::CommandBuffer> commandBuffers;
    std::vector<vk::raii::Semaphore>     presentCompleteSemaphores;
    std::vector<vk::raii::Semaphore>     renderFinishedSemaphores;
    std::vector<vk::raii::Fence>         inFlightFences;
    uint32_t                             frameIndex = 0;
    bool                                 framebufferResized = false;

    static void framebufferResizeCallback(GLFWwindow *window, int width, int height)
    {
        auto app = reinterpret_cast<SwapchainResizeTest *>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;

        if (width > 0 && height > 0) {
            app->drawFrame();
        }
    }

    static void windowRefreshCallback(GLFWwindow* window)
    {
        auto app = reinterpret_cast<SwapchainResizeTest*>(glfwGetWindowUserPointer(window));
        app->drawFrame();
    }

    void initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Slang Modern C++", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);

        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
        glfwSetWindowRefreshCallback(window, windowRefreshCallback);
    }

    void initVulkan()
    {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createGraphicsPipeline();
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();
    }

    void createInstance()
    {
        constexpr vk::ApplicationInfo appInfo{
            .pApplicationName   = "Slang Vulkan Test",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName        = "No Engine",
            .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion         = vk::ApiVersion13
        };

        std::vector<char const *> layers;
        if (enableValidationLayers) layers = validationLayers;

        auto layerProperties = context.enumerateInstanceLayerProperties();
        for (auto const &layer : layers) {
            if (std::ranges::none_of(layerProperties, [&layer](auto const &lp) { return strcmp(lp.layerName, layer) == 0; }))
                throw std::runtime_error("Required layer not supported: " + std::string(layer));
        }

        auto extensions = getRequiredExtensions();

        vk::InstanceCreateInfo createInfo{
            .pApplicationInfo        = &appInfo,
            .enabledLayerCount       = static_cast<uint32_t>(layers.size()),
            .ppEnabledLayerNames     = layers.data(),
            .enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data()
        };
        instance = vk::raii::Instance(context, createInfo);
    }

    void setupDebugMessenger()
    {
        if (!enableValidationLayers) return;
        vk::DebugUtilsMessengerCreateInfoEXT createInfo{
            .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            .messageType     = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            .pfnUserCallback = &debugCallback
        };
        debugMessenger = instance.createDebugUtilsMessengerEXT(createInfo);
    }

    void createSurface()
    {
        VkSurfaceKHR rawSurface;
        if (glfwCreateWindowSurface(*instance, window, nullptr, &rawSurface) != VK_SUCCESS)
            throw std::runtime_error("failed to create window surface");
        surface = vk::raii::SurfaceKHR(instance, rawSurface);
    }

    void pickPhysicalDevice()
    {
        auto devices = instance.enumeratePhysicalDevices();
        auto it = std::ranges::find_if(devices, [&](auto const& dev) {
            if (dev.getProperties().apiVersion < VK_API_VERSION_1_3) return false;

            auto qProps = dev.getQueueFamilyProperties();
            bool hasGraphics = std::ranges::any_of(qProps, [](auto const& q) { return !!(q.queueFlags & vk::QueueFlagBits::eGraphics); });
            if (!hasGraphics) return false;

            auto exts = dev.enumerateDeviceExtensionProperties();
            for (const char* req : requiredDeviceExtensions) {
                if (std::ranges::none_of(exts, [req](auto const& e) { return strcmp(e.extensionName, req) == 0; })) return false;
            }
            return true;
        });

        if (it == devices.end()) throw std::runtime_error("failed to find suitable GPU");
        physicalDevice = *it;
    }

    void createLogicalDevice()
    {
        auto queueProps = physicalDevice.getQueueFamilyProperties();
        for (uint32_t i = 0; i < queueProps.size(); ++i) {
            if ((queueProps[i].queueFlags & vk::QueueFlagBits::eGraphics) && physicalDevice.getSurfaceSupportKHR(i, *surface)) {
                queueIndex = i;
                break;
            }
        }
        if (queueIndex == ~0u) throw std::runtime_error("no suitable queue family");

        float priority = 1.0f;
        vk::DeviceQueueCreateInfo queueCreateInfo{
            .queueFamilyIndex = queueIndex,
            .queueCount       = 1,
            .pQueuePriorities = &priority
        };

        vk::StructureChain<vk::PhysicalDeviceFeatures2,
                           vk::PhysicalDeviceVulkan11Features,
                           vk::PhysicalDeviceVulkan13Features,
                           vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
            featuresChain{
                {},
                {.shaderDrawParameters = true},
                {.synchronization2 = true, .dynamicRendering = true},
                {.extendedDynamicState = true}
            };

        vk::DeviceCreateInfo deviceCreateInfo{
            .pNext                   = &featuresChain.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount    = 1,
            .pQueueCreateInfos       = &queueCreateInfo,
            .enabledExtensionCount   = static_cast<uint32_t>(requiredDeviceExtensions.size()),
            .ppEnabledExtensionNames = requiredDeviceExtensions.data()
        };

        if (enableValidationLayers) {
            deviceCreateInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
            deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
        }

        device = vk::raii::Device(physicalDevice, deviceCreateInfo);
        queue  = vk::raii::Queue(device, queueIndex, 0);
    }

    void createSwapChain()
    {
        auto caps = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
        auto formats = physicalDevice.getSurfaceFormatsKHR(*surface);
        auto modes = physicalDevice.getSurfacePresentModesKHR(*surface);

        swapChainExtent = chooseSwapExtent(caps);
        swapChainSurfaceFormat = chooseSwapSurfaceFormat(formats);
        auto presentMode = chooseSwapPresentMode(modes);

        uint32_t imageCount = std::max(3u, caps.minImageCount);
        if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) imageCount = caps.maxImageCount;

        vk::SwapchainCreateInfoKHR createInfo{
            .surface          = *surface,
            .minImageCount    = imageCount,
            .imageFormat      = swapChainSurfaceFormat.format,
            .imageColorSpace  = swapChainSurfaceFormat.colorSpace,
            .imageExtent      = swapChainExtent,
            .imageArrayLayers = 1,
            .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode = vk::SharingMode::eExclusive,
            .preTransform     = caps.currentTransform,
            .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode      = presentMode,
            .clipped          = true
        };

        swapChain = vk::raii::SwapchainKHR(device, createInfo);
        swapChainImages = swapChain.getImages();
    }

    void createImageViews()
    {
        swapChainImageViews.clear();
        for (auto const& image : swapChainImages) {
            vk::ImageViewCreateInfo createInfo{
                .image            = image,
                .viewType         = vk::ImageViewType::e2D,
                .format           = swapChainSurfaceFormat.format,
                .subresourceRange = { .aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .layerCount = 1 }
            };
            swapChainImageViews.emplace_back(device, createInfo);
        }
    }

    void createGraphicsPipeline()
    {
        SlangShaderCompiler compiler(kShaderSource);
        auto vertCode = compiler.compileEntryPoint("vertexMain");
        auto fragCode = compiler.compileEntryPoint("fragmentMain");

        vk::raii::ShaderModule vertModule = createShaderModule(vertCode);
        vk::raii::ShaderModule fragModule = createShaderModule(fragCode);

        std::array<vk::PipelineShaderStageCreateInfo, 2> stages = {
            vk::PipelineShaderStageCreateInfo{ .stage = vk::ShaderStageFlagBits::eVertex,   .module = *vertModule, .pName = "main" },
            vk::PipelineShaderStageCreateInfo{ .stage = vk::ShaderStageFlagBits::eFragment, .module = *fragModule, .pName = "main" }
        };

        vk::PipelineVertexInputStateCreateInfo vertexInput{};
        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ .topology = vk::PrimitiveTopology::eTriangleList };
        vk::PipelineViewportStateCreateInfo viewportState{ .viewportCount = 1, .scissorCount = 1 };

        vk::PipelineRasterizationStateCreateInfo rasterizer{
            .polygonMode = vk::PolygonMode::eFill,
            .cullMode    = vk::CullModeFlagBits::eBack,
            .frontFace   = vk::FrontFace::eCounterClockwise,
            .lineWidth   = 1.0f
        };

        vk::PipelineMultisampleStateCreateInfo multisampling{ .rasterizationSamples = vk::SampleCountFlagBits::e1 };

        vk::PipelineColorBlendAttachmentState blendAttachment{
            .blendEnable    = vk::False,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
        };
        vk::PipelineColorBlendStateCreateInfo colorBlending{ .attachmentCount = 1, .pAttachments = &blendAttachment };

        std::array<vk::DynamicState, 2> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
        vk::PipelineDynamicStateCreateInfo dynamicState{
            .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
            .pDynamicStates    = dynamicStates.data()
        };

        vk::PipelineLayoutCreateInfo layoutInfo{};
        pipelineLayout = vk::raii::PipelineLayout(device, layoutInfo);

        vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineChain{
            {
                .stageCount          = static_cast<uint32_t>(stages.size()),
                .pStages             = stages.data(),
                .pVertexInputState   = &vertexInput,
                .pInputAssemblyState = &inputAssembly,
                .pViewportState      = &viewportState,
                .pRasterizationState = &rasterizer,
                .pMultisampleState   = &multisampling,
                .pColorBlendState    = &colorBlending,
                .pDynamicState       = &dynamicState,
                .layout              = *pipelineLayout
            },
            {
                .colorAttachmentCount    = 1,
                .pColorAttachmentFormats = &swapChainSurfaceFormat.format
            }
        };

        graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineChain.get<vk::GraphicsPipelineCreateInfo>());
    }

    void createCommandPool()
    {
        vk::CommandPoolCreateInfo poolInfo{
            .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = queueIndex
        };
        commandPool = vk::raii::CommandPool(device, poolInfo);
    }

    void createCommandBuffers()
    {
        vk::CommandBufferAllocateInfo allocInfo{
            .commandPool        = *commandPool,
            .level              = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = MAX_FRAMES_IN_FLIGHT
        };
        commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
    }

    void createSyncObjects()
    {
        presentCompleteSemaphores.clear();
        renderFinishedSemaphores.clear();
        inFlightFences.clear();

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            presentCompleteSemaphores.emplace_back(device, vk::SemaphoreCreateInfo{});
            inFlightFences.emplace_back(device, vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled });
        }
        for (size_t i = 0; i < swapChainImages.size(); ++i) {
            renderFinishedSemaphores.emplace_back(device, vk::SemaphoreCreateInfo{});
        }
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }
        device.waitIdle();
    }

    void drawFrame()
    {
        if (device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX) != vk::Result::eSuccess)
            throw std::runtime_error("wait for fences failed");

        uint32_t imageIndex;
        vk::Result result;

        while (true)
        {
            try {
                auto pair = swapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);
                result = pair.first;
                imageIndex = pair.second;
            } catch (const vk::SystemError& err) {
                result = static_cast<vk::Result>(err.code().value());
            }

            if (result == vk::Result::eErrorOutOfDateKHR) {
                recreateSwapChain();
                continue;
            } else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
                throw std::runtime_error("failed to acquire image");
            }
            break;
        }

        device.resetFences(*inFlightFences[frameIndex]);
        commandBuffers[frameIndex].reset();
        recordCommandBuffer(imageIndex);

        vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo submitInfo{
            .waitSemaphoreCount   = 1,
            .pWaitSemaphores      = &*presentCompleteSemaphores[frameIndex],
            .pWaitDstStageMask    = &waitStage,
            .commandBufferCount   = 1,
            .pCommandBuffers      = &*commandBuffers[frameIndex],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores    = &*renderFinishedSemaphores[imageIndex]
        };
        queue.submit(submitInfo, *inFlightFences[frameIndex]);

        vk::PresentInfoKHR presentInfo{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores    = &*renderFinishedSemaphores[imageIndex],
            .swapchainCount     = 1,
            .pSwapchains        = &*swapChain,
            .pImageIndices      = &imageIndex
        };

        try {
            result = queue.presentKHR(presentInfo);
        } catch (const vk::SystemError& err) {
             result = static_cast<vk::Result>(err.code().value());
        }

        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
        }
        frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void recordCommandBuffer(uint32_t imageIndex)
    {
        auto& cmd = commandBuffers[frameIndex];
        cmd.begin({ .flags = {} });

        transition_image_layout(cmd, imageIndex,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
            {}, vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eColorAttachmentOutput);

        vk::ClearValue clearValue{};
        clearValue.color = vk::ClearColorValue{ std::array{0.0f, 0.0f, 0.0f, 1.0f} };

        vk::RenderingAttachmentInfo attachment{
            .imageView   = *swapChainImageViews[imageIndex],
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp      = vk::AttachmentLoadOp::eClear,
            .storeOp     = vk::AttachmentStoreOp::eStore,
            .clearValue  = clearValue
        };

        vk::RenderingInfo renderingInfo{
            .renderArea           = { {0, 0}, swapChainExtent },
            .layerCount           = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments    = &attachment
        };

        cmd.beginRendering(renderingInfo);
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);

        vk::Viewport vp{ 0.0f, 0.0f, (float)swapChainExtent.width, (float)swapChainExtent.height, 0.0f, 1.0f };
        cmd.setViewport(0, vp);
        vk::Rect2D scissor{ {0, 0}, swapChainExtent };
        cmd.setScissor(0, scissor);

        cmd.draw(3, 1, 0, 0);
        cmd.endRendering();

        transition_image_layout(cmd, imageIndex,
            vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite, {},
            vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eBottomOfPipe);

        cmd.end();
    }

    void transition_image_layout(vk::raii::CommandBuffer& cmd, uint32_t imageIndex,
        vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
        vk::AccessFlags2 srcAccess, vk::AccessFlags2 dstAccess,
        vk::PipelineStageFlags2 srcStage, vk::PipelineStageFlags2 dstStage)
    {
        vk::ImageMemoryBarrier2 barrier{
            .srcStageMask        = srcStage,
            .srcAccessMask       = srcAccess,
            .dstStageMask        = dstStage,
            .dstAccessMask       = dstAccess,
            .oldLayout           = oldLayout,
            .newLayout           = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = swapChainImages[imageIndex],
            .subresourceRange    = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
        };
        vk::DependencyInfo depInfo{ .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier };
        cmd.pipelineBarrier2(depInfo);
    }

    vk::raii::ShaderModule createShaderModule(const std::vector<uint32_t>& code)
    {
        vk::ShaderModuleCreateInfo info{
            .codeSize = code.size() * sizeof(uint32_t),
            .pCode    = code.data()
        };
        return vk::raii::ShaderModule(device, info);
    }

    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats) {
        auto it = std::ranges::find_if(formats, [](auto& f) {
            return f.format == vk::Format::eB8G8R8A8Srgb && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        });
        return it != formats.end() ? *it : formats[0];
    }

    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& modes) {
        auto it = std::ranges::find(modes, vk::PresentModeKHR::eMailbox);
        return it != modes.end() ? *it : vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& caps) {
        if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) return caps.currentExtent;
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        return {
            std::clamp<uint32_t>(w, caps.minImageExtent.width, caps.maxImageExtent.width),
            std::clamp<uint32_t>(h, caps.minImageExtent.height, caps.maxImageExtent.height)
        };
    }

    std::vector<const char*> getRequiredExtensions() {
        uint32_t count = 0;
        const char** glfwExts = glfwGetRequiredInstanceExtensions(&count);
        std::vector<const char*> exts(glfwExts, glfwExts + count);
        if (enableValidationLayers) exts.push_back(vk::EXTDebugUtilsExtensionName);
        return exts;
    }

    void recreateSwapChain() {
        int w = 0, h = 0;
        glfwGetFramebufferSize(window, &w, &h);
        while (w == 0 || h == 0) {
            glfwGetFramebufferSize(window, &w, &h);
            glfwWaitEvents();
        }
        device.waitIdle();
        swapChainImageViews.clear();
        swapChain = nullptr;
        createSwapChain();
        createImageViews();
    }

    void cleanup() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void*)
    {
        if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
            std::cerr << "Validation: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }
};

int main()
{
    try {
        SwapchainResizeTest app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
