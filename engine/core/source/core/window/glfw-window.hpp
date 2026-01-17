#pragma once

#include <core/window/window.hpp>
#include <unordered_map>

struct GLFWwindow;

namespace april
{
    struct GlfwWindow final: Window
    {
        GlfwWindow(WindowDesc const& desc);
        ~GlfwWindow() override;

        auto onEvent() -> void override;

        [[nodiscard]] auto getWidth() const -> unsigned int override { return m_data.width.load(std::memory_order_relaxed); }
        [[nodiscard]] auto getHeight() const -> unsigned int override { return m_data.height.load(std::memory_order_relaxed); }

        [[nodiscard]] auto getFramebufferWidth() const -> unsigned int override { return m_data.fbWidth.load(std::memory_order_relaxed); }
        [[nodiscard]] auto getFramebufferHeight() const -> unsigned int override { return m_data.fbHeight.load(std::memory_order_relaxed); }

        auto setVSync(bool enabled) -> void override;
        [[nodiscard]] auto isVSync() const -> bool override;

        [[nodiscard]] auto getBackendWindow() const -> void* override { return m_glfwWindow; }
        [[nodiscard]] auto getNativeWindowHandle() const -> void* override;

        auto registerCallbackImpl(EventType type, EventCallbackFn callback) -> void override
        {
            m_data.callbacks[type].emplace_back(std::move(callback));
        }

    private:
        auto init(WindowDesc const& desc) -> void;
        auto shutdown() -> void;

    private:
        GLFWwindow* m_glfwWindow{nullptr};

        struct WindowData
        {
            std::string title;
            std::atomic<unsigned int> width{0};
            std::atomic<unsigned int> height{0};
            std::atomic<unsigned int> fbWidth{0};
            std::atomic<unsigned int> fbHeight{0};
            bool vSync{false};

            std::unordered_map<EventType, std::vector<EventCallbackFn>> callbacks;

            auto dispatchEvent(Event& event) -> void;
        };

        WindowData m_data{};
    };
}
