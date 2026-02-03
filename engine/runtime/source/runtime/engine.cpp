#include "engine.hpp"

#include <core/error/assert.hpp>
#include <core/input/input.hpp>
#include <core/log/logger.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>

namespace april
{
    Engine* Engine::s_instance = nullptr;

    Engine::Engine(EngineConfig const& config, EngineHooks hooks)
        : m_config(config)
        , m_hooks(std::move(hooks))
    {
        AP_ASSERT(s_instance == nullptr, "Only one Engine instance allowed.");
        s_instance = this;
    }

    Engine::~Engine()
    {
        if (m_running)
        {
            stop();
        }
        shutdown();
        s_instance = nullptr;
    }

    auto Engine::get() -> Engine&
    {
        AP_ASSERT(s_instance != nullptr, "Engine instance not created.");
        return *s_instance;
    }

    auto Engine::run() -> int
    {
        try
        {
            init();

            using Clock = std::chrono::steady_clock;
            auto lastTime = Clock::now();

            while (m_running)
            {
                auto now = Clock::now();
                std::chrono::duration<float> delta = now - lastTime;
                lastTime = now;

                Input::beginFrame();
                m_window->onEvent();

                renderFrame(delta.count());
            }

            shutdown();
            return 0;
        }
        catch (std::exception const& e)
        {
            std::cerr << "Engine exception: " << e.what() << std::endl;
            return 1;
        }
        catch (...)
        {
            std::cerr << "Engine unknown exception" << std::endl;
            return 1;
        }
    }

    auto Engine::renderFrame(float delta) -> void
    {
        if (m_swapchainDirty)
        {
            auto fbWidth = m_window->getFramebufferWidth();
            auto fbHeight = m_window->getFramebufferHeight();
            if (fbWidth == 0 || fbHeight == 0)
            {
                return;
            }
            m_swapchain->resize(fbWidth, fbHeight);
            ensureOffscreenTarget(fbWidth, fbHeight);
            m_swapchainDirty = false;
        }

        auto backBuffer = m_swapchain->acquireNextImage();
        if (!backBuffer)
        {
            m_swapchainDirty = true;
            return;
        }

        if (m_hooks.onUpdate)
        {
            m_hooks.onUpdate(delta);
        }

        auto targetTexture = m_offscreen ? m_offscreen : backBuffer;
        auto targetRtv = targetTexture->getRTV();
        auto targetSrv = targetTexture->getSRV();

        m_context->resourceBarrier(targetTexture.get(), graphics::Resource::State::RenderTarget);

        if (m_hooks.onRender)
        {
            m_hooks.onRender(m_context, targetRtv.get());
        }
        else
        {
            m_context->clearRtv(targetRtv.get(), m_config.clearColor);
        }

        bool uiBegan = false;
        if (m_imguiLayer)
        {
            m_imguiLayer->beginFrame();
            uiBegan = true;

            if (m_hooks.onUI)
            {
                m_hooks.onUI();
            }
        }

        if (m_renderer)
        {
            m_renderer->render(m_context, m_config.clearColor);
        }

        if (m_renderer && m_config.compositeSceneToOutput && !m_hooks.onRender)
        {
            auto sceneSrv = m_renderer->getSceneColorSrv();
            if (sceneSrv)
            {
                auto colorTarget = graphics::ColorTarget(
                    targetRtv,
                    graphics::LoadOp::Load,
                    graphics::StoreOp::Store
                );
                auto encoder = m_context->beginRenderPass({colorTarget});
                encoder->blit(sceneSrv, targetRtv);
                encoder->end();
            }
        }

        if (uiBegan)
        {
            m_imguiLayer->endFrame(m_context, targetRtv);
        }

        if (m_offscreen)
        {
            m_context->resourceBarrier(targetTexture.get(), graphics::Resource::State::ShaderResource);
        }

        if (m_offscreen && targetSrv)
        {
            auto colorTarget = graphics::ColorTarget(
                backBuffer->getRTV(),
                graphics::LoadOp::Clear,
                graphics::StoreOp::Store,
                m_config.clearColor
            );
            auto encoder = m_context->beginRenderPass({colorTarget});
            encoder->blit(targetSrv, backBuffer->getRTV());
            encoder->end();
        }

        m_context->resourceBarrier(backBuffer.get(), graphics::Resource::State::Present);
        m_context->submit();
        m_swapchain->present();
        m_device->endFrame();
    }

    auto Engine::stop() -> void
    {
        m_running = false;
    }

    auto Engine::addElement(core::ref<ui::IElement> const& element) -> void
    {
        if (m_imguiLayer)
        {
            m_imguiLayer->addElement(element);
            return;
        }

        m_pendingElements.push_back(element);
    }

    auto Engine::getSceneColorSrv() const -> core::ref<graphics::TextureView>
    {
        if (!m_renderer)
        {
            return nullptr;
        }
        return m_renderer->getSceneColorSrv();
    }

    auto Engine::setSceneViewportSize(uint32_t width, uint32_t height) -> void
    {
        if (!m_renderer)
        {
            return;
        }
        m_renderer->setViewportSize(width, height);
    }

    auto Engine::setSceneViewProjection(float4x4 const& viewProj) -> void
    {
        if (m_renderer)
        {
            m_renderer->setExternalViewProjection(viewProj);
        }
    }

    auto Engine::init() -> void
    {
        if (m_initialized)
        {
            return;
        }

        m_window = Window::create(m_config.window);
        if (!m_window)
        {
            throw std::runtime_error("Failed to create window.");
        }

        m_window->setVSync(m_config.vSync);

        m_window->subscribe<WindowCloseEvent>([this](WindowCloseEvent const&) {
            stop();
        });

        m_window->subscribe<FrameBufferResizeEvent>([this](FrameBufferResizeEvent const& e) {
            if (e.width > 0 && e.height > 0)
            {
                m_swapchain->resize(e.width, e.height);
                ensureOffscreenTarget(e.width, e.height);
                m_swapchainDirty = false;
                renderFrame(0.0f);
            }
        });

        m_device = core::make_ref<graphics::Device>(m_config.device);

        graphics::Swapchain::Desc swapchainDesc{};
        swapchainDesc.format = graphics::ResourceFormat::RGBA8Unorm;
        swapchainDesc.width = m_window->getFramebufferWidth();
        swapchainDesc.height = m_window->getFramebufferHeight();
        swapchainDesc.imageCount = m_device->kInFlightFrameCount;

        m_swapchain = core::make_ref<graphics::Swapchain>(
            m_device,
            swapchainDesc,
            m_window->getNativeWindowHandle()
        );

        ensureOffscreenTarget(swapchainDesc.width, swapchainDesc.height);

        m_context = m_device->getCommandContext();

        // Create asset manager (path relative to executable directory)
        m_assetManager = std::make_unique<asset::AssetManager>("E:/github/April2/content", "E:/github/April2/Cache/DDC");

        m_renderer = core::make_ref<graphics::SceneRenderer>(m_device, m_assetManager.get());
        m_renderer->setViewportSize(
            m_window->getFramebufferWidth(),
            m_window->getFramebufferHeight()
        );
        m_renderer->setUseExternalCamera(m_config.enableUI);

        if (m_config.enableUI)
        {
            auto desc = m_config.imgui;
            desc.device = m_device;
            desc.window = m_window.get();
            desc.vSync = m_config.vSync;
            desc.iniFilename = m_config.imguiIniFilename;

            m_imguiLayer = core::make_ref<ui::ImGuiLayer>();
            m_imguiLayer->init(desc);
            attachPendingElements();
        }

        m_running = true;
        m_initialized = true;

        if (m_hooks.onInit)
        {
            m_hooks.onInit();
        }
    }

    auto Engine::shutdown() -> void
    {
        if (!m_initialized)
        {
            return;
        }

        if (m_hooks.onShutdown)
        {
            m_hooks.onShutdown();
        }

        if (m_imguiLayer)
        {
            m_imguiLayer->terminate();
            m_imguiLayer.reset();
        }

        m_swapchain.reset();
        m_offscreen.reset();
        m_device.reset();
        m_window.reset();

        m_running = false;
        m_initialized = false;
    }

    auto Engine::attachPendingElements() -> void
    {
        if (!m_imguiLayer)
        {
            return;
        }

        for (auto const& element : m_pendingElements)
        {
            m_imguiLayer->addElement(element);
        }
        m_pendingElements.clear();
    }

    auto Engine::ensureOffscreenTarget(uint32_t width, uint32_t height) -> void
    {
        if (!m_device || width == 0 || height == 0)
        {
            return;
        }

        if (m_offscreen && width == m_offscreenWidth && height == m_offscreenHeight)
        {
            return;
        }

        m_offscreenWidth = width;
        m_offscreenHeight = height;
        m_offscreen = m_device->createTexture2D(
            width,
            height,
            graphics::ResourceFormat::RGBA8Unorm,
            1,
            1,
            nullptr,
            graphics::TextureUsage::RenderTarget | graphics::TextureUsage::ShaderResource
        );
        m_offscreen->setName("Engine.OffscreenColor");
    }
}
