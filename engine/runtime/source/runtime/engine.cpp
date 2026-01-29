#include "engine.hpp"

#include <core/error/assert.hpp>
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
                m_window->onEvent();

                if (m_swapchainDirty)
                {
                    m_swapchain->resize(m_window->getFramebufferWidth(), m_window->getFramebufferHeight());
                    m_swapchainDirty = false;
                }

                auto backBuffer = m_swapchain->acquireNextImage();
                if (!backBuffer)
                {
                    break;
                }

                auto now = Clock::now();
                std::chrono::duration<float> delta = now - lastTime;
                lastTime = now;

                if (m_hooks.onUpdate)
                {
                    m_hooks.onUpdate(delta.count());
                }

                m_context->resourceBarrier(backBuffer.get(), graphics::Resource::State::RenderTarget);

                if (m_hooks.onRender)
                {
                    m_hooks.onRender(m_context, backBuffer->getRTV().get());
                }
                else
                {
                    m_context->clearRtv(backBuffer->getRTV().get(), m_config.clearColor);
                }

                if (m_imguiLayer)
                {
                    m_imguiLayer->beginFrame();

                    if (m_hooks.onUI)
                    {
                        m_hooks.onUI();
                    }

                    m_imguiLayer->endFrame(m_context, backBuffer->getRTV());
                }

                m_context->resourceBarrier(backBuffer.get(), graphics::Resource::State::Present);
                m_context->submit();
                m_swapchain->present();
                m_device->endFrame();
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
                m_swapchainDirty = true;
            }
        });

        m_device = core::make_ref<graphics::Device>(m_config.device);

        graphics::Swapchain::Desc swapchainDesc{};
        swapchainDesc.format = graphics::ResourceFormat::RGBA8Unorm;
        swapchainDesc.width = m_window->getFramebufferWidth();
        swapchainDesc.height = m_window->getFramebufferHeight();
        swapchainDesc.imageCount = 3;

        m_swapchain = core::make_ref<graphics::Swapchain>(
            m_device,
            swapchainDesc,
            m_window->getNativeWindowHandle()
        );

        m_context = m_device->getCommandContext();

        if (m_config.enableUI)
        {
            auto desc = m_config.imgui;
            desc.device = m_device;
            desc.window = m_window.get();
            desc.vSync = m_config.vSync;

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
}
