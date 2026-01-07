#include "glfw-window.hpp"
#include "core/log/logger.hpp"

#include <GLFW/glfw3.h>
#ifdef _WIN32
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <GLFW/glfw3native.h>
#endif
#include <format>
#include <iostream>

namespace april
{
    // --- Static Helpers ---

    static auto glfwErrorCallback(int error, char const* description) -> void
    {
        std::cerr << std::format("GLFW Error ({0}): {1}\n", error, description);
    }

    // --- GlfwWindow Implementation ---

    GlfwWindow::GlfwWindow(WindowDesc const& desc)
    {
        init(desc);
    }

    GlfwWindow::~GlfwWindow()
    {
        shutdown();
    }

    auto GlfwWindow::getNativeWindowHandle() const -> void*
    {
        #ifdef _WIN32
            return glfwGetWin32Window(m_glfwWindow);
        #else
            return nullptr;
        #endif
    }

    auto GlfwWindow::init(WindowDesc const& desc) -> void
    {
        m_data.title = desc.title;
        m_data.width = desc.width;
        m_data.height = desc.height;

        AP_INFO("Creating Window: {0} ({1}, {2})", m_data.title, m_data.width.load(), m_data.height.load());

        if (static bool isGlfwInitialized = false; !isGlfwInitialized)
        {
            int const success = glfwInit();
            if (success == GLFW_FALSE)
            {
                AP_CRITICAL("Could not initialize GLFW!");
                return;
            }
            glfwSetErrorCallback(glfwErrorCallback);
            isGlfwInitialized = true;
        }

        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

        m_glfwWindow = glfwCreateWindow(
            static_cast<int>(m_data.width),
            static_cast<int>(m_data.height),
            m_data.title.c_str(),
            nullptr,
            nullptr
        );

        if (m_glfwWindow == nullptr)
        {
            AP_CRITICAL("Failed to create GLFW window!");

            return;
        }

        glfwSetWindowUserPointer(m_glfwWindow, &m_data);

        setVSync(true);

        // --- GLFW Callbacks ---

        // 1. Window Resize
        glfwSetWindowSizeCallback(m_glfwWindow, [](GLFWwindow* window, int width, int height) {
            auto* dataPtr = static_cast<WindowData*>(glfwGetWindowUserPointer(window));

            dataPtr->width = static_cast<unsigned int>(width);
            dataPtr->height = static_cast<unsigned int>(height);

            auto event = WindowResizeEvent{dataPtr->width, dataPtr->height};
            dataPtr->dispatchEvent(event);
        });

        // 2. Window Close
        glfwSetWindowCloseCallback(m_glfwWindow, [](GLFWwindow* window) {
            auto* dataPtr = static_cast<WindowData*>(glfwGetWindowUserPointer(window));

            auto event = WindowCloseEvent{};
            dataPtr->dispatchEvent(event);
        });

        // 3. Key Callback
        // glfwSetKeyCallback(...)
    }

    auto GlfwWindow::shutdown() -> void
    {
        if (m_glfwWindow != nullptr)
        {
            glfwDestroyWindow(m_glfwWindow);
            m_glfwWindow = nullptr;
        }

        glfwTerminate();
    }

    auto GlfwWindow::WindowData::dispatchEvent(Event& e) -> void
    {
        if (auto it = callbacks.find(e.getType()); it != callbacks.end())
        {
            for (auto& func : it->second)
            {
                func(e);
                if (e.handled) break;
            }
        }
    }

    auto GlfwWindow::onUpdate() -> void
    {
        glfwPollEvents();
    }

    auto GlfwWindow::setVSync(bool enabled) -> void
    {
        m_data.vSync = enabled;
    }

    auto GlfwWindow::isVSync() const -> bool
    {
        return m_data.vSync;
    }

    auto Window::create(WindowDesc const& desc) -> std::unique_ptr<Window>
    {
        switch (desc.type)
        {
            case EWindowType::glfw: return std::make_unique<GlfwWindow>(desc);
            default: return nullptr;
        }
    }
} // namespace april
