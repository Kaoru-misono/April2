#include "glfw-window.hpp"
#include <core/input/input.hpp>
#include <core/log/logger.hpp>
#include <core/math/type.hpp>

#include <GLFW/glfw3.h>
#ifdef _WIN32
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <GLFW/glfw3native.h>
#endif
#include <format>
#include <iostream>

namespace april
{
    namespace
    {
        auto mapKey(int key) -> Key
        {
            switch (key)
            {
            case GLFW_KEY_SPACE: return Key::Space;
            case GLFW_KEY_APOSTROPHE: return Key::Apostrophe;
            case GLFW_KEY_COMMA: return Key::Comma;
            case GLFW_KEY_MINUS: return Key::Minus;
            case GLFW_KEY_PERIOD: return Key::Period;
            case GLFW_KEY_SLASH: return Key::Slash;
            case GLFW_KEY_0: return Key::D0;
            case GLFW_KEY_1: return Key::D1;
            case GLFW_KEY_2: return Key::D2;
            case GLFW_KEY_3: return Key::D3;
            case GLFW_KEY_4: return Key::D4;
            case GLFW_KEY_5: return Key::D5;
            case GLFW_KEY_6: return Key::D6;
            case GLFW_KEY_7: return Key::D7;
            case GLFW_KEY_8: return Key::D8;
            case GLFW_KEY_9: return Key::D9;
            case GLFW_KEY_SEMICOLON: return Key::Semicolon;
            case GLFW_KEY_EQUAL: return Key::Equal;
            case GLFW_KEY_A: return Key::A;
            case GLFW_KEY_B: return Key::B;
            case GLFW_KEY_C: return Key::C;
            case GLFW_KEY_D: return Key::D;
            case GLFW_KEY_E: return Key::E;
            case GLFW_KEY_F: return Key::F;
            case GLFW_KEY_G: return Key::G;
            case GLFW_KEY_H: return Key::H;
            case GLFW_KEY_I: return Key::I;
            case GLFW_KEY_J: return Key::J;
            case GLFW_KEY_K: return Key::K;
            case GLFW_KEY_L: return Key::L;
            case GLFW_KEY_M: return Key::M;
            case GLFW_KEY_N: return Key::N;
            case GLFW_KEY_O: return Key::O;
            case GLFW_KEY_P: return Key::P;
            case GLFW_KEY_Q: return Key::Q;
            case GLFW_KEY_R: return Key::R;
            case GLFW_KEY_S: return Key::S;
            case GLFW_KEY_T: return Key::T;
            case GLFW_KEY_U: return Key::U;
            case GLFW_KEY_V: return Key::V;
            case GLFW_KEY_W: return Key::W;
            case GLFW_KEY_X: return Key::X;
            case GLFW_KEY_Y: return Key::Y;
            case GLFW_KEY_Z: return Key::Z;
            case GLFW_KEY_LEFT_BRACKET: return Key::LeftBracket;
            case GLFW_KEY_BACKSLASH: return Key::Backslash;
            case GLFW_KEY_RIGHT_BRACKET: return Key::RightBracket;
            case GLFW_KEY_GRAVE_ACCENT: return Key::GraveAccent;
            case GLFW_KEY_ESCAPE: return Key::Escape;
            case GLFW_KEY_ENTER: return Key::Enter;
            case GLFW_KEY_TAB: return Key::Tab;
            case GLFW_KEY_BACKSPACE: return Key::Backspace;
            case GLFW_KEY_INSERT: return Key::Insert;
            case GLFW_KEY_DELETE: return Key::Delete;
            case GLFW_KEY_RIGHT: return Key::Right;
            case GLFW_KEY_LEFT: return Key::Left;
            case GLFW_KEY_DOWN: return Key::Down;
            case GLFW_KEY_UP: return Key::Up;
            case GLFW_KEY_PAGE_UP: return Key::PageUp;
            case GLFW_KEY_PAGE_DOWN: return Key::PageDown;
            case GLFW_KEY_HOME: return Key::Home;
            case GLFW_KEY_END: return Key::End;
            case GLFW_KEY_CAPS_LOCK: return Key::CapsLock;
            case GLFW_KEY_SCROLL_LOCK: return Key::ScrollLock;
            case GLFW_KEY_NUM_LOCK: return Key::NumLock;
            case GLFW_KEY_PRINT_SCREEN: return Key::PrintScreen;
            case GLFW_KEY_PAUSE: return Key::Pause;
            case GLFW_KEY_F1: return Key::F1;
            case GLFW_KEY_F2: return Key::F2;
            case GLFW_KEY_F3: return Key::F3;
            case GLFW_KEY_F4: return Key::F4;
            case GLFW_KEY_F5: return Key::F5;
            case GLFW_KEY_F6: return Key::F6;
            case GLFW_KEY_F7: return Key::F7;
            case GLFW_KEY_F8: return Key::F8;
            case GLFW_KEY_F9: return Key::F9;
            case GLFW_KEY_F10: return Key::F10;
            case GLFW_KEY_F11: return Key::F11;
            case GLFW_KEY_F12: return Key::F12;
            case GLFW_KEY_LEFT_SHIFT: return Key::LeftShift;
            case GLFW_KEY_LEFT_CONTROL: return Key::LeftControl;
            case GLFW_KEY_LEFT_ALT: return Key::LeftAlt;
            case GLFW_KEY_LEFT_SUPER: return Key::LeftSuper;
            case GLFW_KEY_RIGHT_SHIFT: return Key::RightShift;
            case GLFW_KEY_RIGHT_CONTROL: return Key::RightControl;
            case GLFW_KEY_RIGHT_ALT: return Key::RightAlt;
            case GLFW_KEY_RIGHT_SUPER: return Key::RightSuper;
            case GLFW_KEY_MENU: return Key::Menu;
            default: return Key::Unknown;
            }
        }

        auto mapMouseButton(int button) -> MouseButton
        {
            switch (button)
            {
            case GLFW_MOUSE_BUTTON_LEFT: return MouseButton::Left;
            case GLFW_MOUSE_BUTTON_RIGHT: return MouseButton::Right;
            case GLFW_MOUSE_BUTTON_MIDDLE: return MouseButton::Middle;
            case GLFW_MOUSE_BUTTON_4: return MouseButton::Button4;
            case GLFW_MOUSE_BUTTON_5: return MouseButton::Button5;
            case GLFW_MOUSE_BUTTON_6: return MouseButton::Button6;
            case GLFW_MOUSE_BUTTON_7: return MouseButton::Button7;
            case GLFW_MOUSE_BUTTON_8: return MouseButton::Button8;
            default: return MouseButton::Unknown;
            }
        }
    }

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

    auto GlfwWindow::getWindowContentScale() const -> float2
    {
        auto scale = float2{};
        glfwGetWindowContentScale(m_glfwWindow, &scale.x, &scale.y);

        return scale;
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

        {
            int fWidth, fHeight;
            glfwGetFramebufferSize(m_glfwWindow, &fWidth, &fHeight);
            m_data.fbWidth = static_cast<unsigned int>(fWidth);
            m_data.fbHeight = static_cast<unsigned int>(fHeight);
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

        // 2. Framebuffer Resize
        glfwSetFramebufferSizeCallback(m_glfwWindow, [](GLFWwindow* window, int width, int height) {
            auto* dataPtr = static_cast<WindowData*>(glfwGetWindowUserPointer(window));

            dataPtr->fbWidth = static_cast<unsigned int>(width);
            dataPtr->fbHeight = static_cast<unsigned int>(height);

            auto event = FrameBufferResizeEvent{dataPtr->fbWidth, dataPtr->fbHeight};
            dataPtr->dispatchEvent(event);
        });

        // 3. Window Close
        glfwSetWindowCloseCallback(m_glfwWindow, [](GLFWwindow* window) {
            auto* dataPtr = static_cast<WindowData*>(glfwGetWindowUserPointer(window));

            auto event = WindowCloseEvent{};
            dataPtr->dispatchEvent(event);
        });

        // 4. Window Focus
        glfwSetWindowFocusCallback(m_glfwWindow, [](GLFWwindow* window, int focused) {
            auto* dataPtr = static_cast<WindowData*>(glfwGetWindowUserPointer(window));

            Input::setWindowFocused(focused == GLFW_TRUE);
            if (focused == GLFW_TRUE)
            {
                auto event = WindowFocusEvent{};
                dataPtr->dispatchEvent(event);
            }
            else
            {
                auto event = WindowLostFocusEvent{};
                dataPtr->dispatchEvent(event);
            }
        });

        // 5. Key Callback
        glfwSetKeyCallback(m_glfwWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            auto* dataPtr = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
            auto const mappedKey = mapKey(key);

            if (action == GLFW_PRESS)
            {
                Input::setKeyDown(mappedKey, true);
                auto event = KeyPressedEvent{key, scancode, mods};
                dataPtr->dispatchEvent(event);
            }
            else if (action == GLFW_RELEASE)
            {
                Input::setKeyDown(mappedKey, false);
                auto event = KeyReleasedEvent{key, scancode, mods};
                dataPtr->dispatchEvent(event);
            }
            else if (action == GLFW_REPEAT)
            {
                Input::setKeyDown(mappedKey, true);
                auto event = KeyRepeatedEvent{key, scancode, mods};
                dataPtr->dispatchEvent(event);
            }
        });

        // 6. Char Callback
        glfwSetCharCallback(m_glfwWindow, [](GLFWwindow* window, unsigned int codepoint) {
            auto* dataPtr = static_cast<WindowData*>(glfwGetWindowUserPointer(window));

            auto event = CharInputEvent{static_cast<uint32_t>(codepoint)};
            dataPtr->dispatchEvent(event);
        });

        // 7. Mouse Button Callback
        glfwSetMouseButtonCallback(m_glfwWindow, [](GLFWwindow* window, int button, int action, int mods) {
            auto* dataPtr = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
            auto const mappedButton = mapMouseButton(button);

            if (action == GLFW_PRESS)
            {
                Input::setMouseButtonDown(mappedButton, true);
                auto event = MouseButtonPressedEvent{button, mods};
                dataPtr->dispatchEvent(event);
            }
            else if (action == GLFW_RELEASE)
            {
                Input::setMouseButtonDown(mappedButton, false);
                auto event = MouseButtonReleasedEvent{button, mods};
                dataPtr->dispatchEvent(event);
            }
        });

        // 8. Mouse Position Callback
        glfwSetCursorPosCallback(m_glfwWindow, [](GLFWwindow* window, double xPos, double yPos) {
            auto* dataPtr = static_cast<WindowData*>(glfwGetWindowUserPointer(window));

            auto position = float2{static_cast<float>(xPos), static_cast<float>(yPos)};
            Input::setMousePosition(position);

            auto event = MouseMovedEvent{position.x, position.y};
            dataPtr->dispatchEvent(event);
        });

        // 9. Mouse Scroll Callback
        glfwSetScrollCallback(m_glfwWindow, [](GLFWwindow* window, double xOffset, double yOffset) {
            auto* dataPtr = static_cast<WindowData*>(glfwGetWindowUserPointer(window));

            auto delta = float2{static_cast<float>(xOffset), static_cast<float>(yOffset)};
            Input::addMouseWheel(delta);

            auto event = MouseScrolledEvent{delta.x, delta.y};
            dataPtr->dispatchEvent(event);
        });
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

    auto GlfwWindow::onEvent() -> void
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
