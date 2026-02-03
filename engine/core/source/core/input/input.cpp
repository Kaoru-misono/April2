#include "input.hpp"

namespace april
{
    inline namespace
    {
        struct InputState
        {
            std::array<bool, Input::kMaxKeys> keysDown{};
            std::array<bool, Input::kMaxKeys> keysPressed{};
            std::array<bool, Input::kMaxKeys> keysReleased{};

            std::array<bool, Input::kMaxMouseButtons> mouseDown{};
            std::array<bool, Input::kMaxMouseButtons> mousePressed{};
            std::array<bool, Input::kMaxMouseButtons> mouseReleased{};

            float2 mousePosition{0.0f, 0.0f};
            float2 mouseDelta{0.0f, 0.0f};
            float2 mouseWheelDelta{0.0f, 0.0f};
            bool mouseInitialized{false};

            bool windowFocused{true};
            bool uiMouseCaptured{false};
            bool uiKeyboardCaptured{false};
        };

        auto g_state = InputState{};

        [[nodiscard]] auto isValidKey(int key) -> bool
        {
            return key >= 0 && key < static_cast<int>(Input::kMaxKeys);
        }

        [[nodiscard]] auto isValidMouseButton(int button) -> bool
        {
            return button >= 0 && button < static_cast<int>(Input::kMaxMouseButtons);
        }
    }

    auto Input::beginFrame() -> void
    {
        g_state.keysPressed.fill(false);
        g_state.keysReleased.fill(false);
        g_state.mousePressed.fill(false);
        g_state.mouseReleased.fill(false);
        g_state.mouseDelta = float2{0.0f, 0.0f};
        g_state.mouseWheelDelta = float2{0.0f, 0.0f};
    }

    auto Input::setKeyDown(int key, bool down) -> void
    {
        if (!isValidKey(key))
        {
            return;
        }

        auto const index = static_cast<size_t>(key);
        bool const wasDown = g_state.keysDown[index];
        if (down && !wasDown)
        {
            g_state.keysPressed[index] = true;
        }
        if (!down && wasDown)
        {
            g_state.keysReleased[index] = true;
        }
        g_state.keysDown[index] = down;
    }

    auto Input::setMouseButtonDown(int button, bool down) -> void
    {
        if (!isValidMouseButton(button))
        {
            return;
        }

        auto const index = static_cast<size_t>(button);
        bool const wasDown = g_state.mouseDown[index];
        if (down && !wasDown)
        {
            g_state.mousePressed[index] = true;
        }
        if (!down && wasDown)
        {
            g_state.mouseReleased[index] = true;
        }
        g_state.mouseDown[index] = down;
    }

    auto Input::setMousePosition(float2 const& position) -> void
    {
        if (!g_state.mouseInitialized)
        {
            g_state.mousePosition = position;
            g_state.mouseInitialized = true;
            return;
        }

        g_state.mouseDelta += (position - g_state.mousePosition);
        g_state.mousePosition = position;
    }

    auto Input::addMouseWheel(float2 const& delta) -> void
    {
        g_state.mouseWheelDelta += delta;
    }

    auto Input::setWindowFocused(bool focused) -> void
    {
        g_state.windowFocused = focused;
    }

    auto Input::setUiCapture(bool mouseCaptured, bool keyboardCaptured) -> void
    {
        g_state.uiMouseCaptured = mouseCaptured;
        g_state.uiKeyboardCaptured = keyboardCaptured;
    }

    auto Input::isKeyDown(int key) -> bool
    {
        if (!isValidKey(key))
        {
            return false;
        }

        return g_state.keysDown[static_cast<size_t>(key)];
    }

    auto Input::wasKeyPressed(int key) -> bool
    {
        if (!isValidKey(key))
        {
            return false;
        }

        return g_state.keysPressed[static_cast<size_t>(key)];
    }

    auto Input::wasKeyReleased(int key) -> bool
    {
        if (!isValidKey(key))
        {
            return false;
        }

        return g_state.keysReleased[static_cast<size_t>(key)];
    }

    auto Input::isMouseDown(int button) -> bool
    {
        if (!isValidMouseButton(button))
        {
            return false;
        }

        return g_state.mouseDown[static_cast<size_t>(button)];
    }

    auto Input::wasMousePressed(int button) -> bool
    {
        if (!isValidMouseButton(button))
        {
            return false;
        }

        return g_state.mousePressed[static_cast<size_t>(button)];
    }

    auto Input::wasMouseReleased(int button) -> bool
    {
        if (!isValidMouseButton(button))
        {
            return false;
        }

        return g_state.mouseReleased[static_cast<size_t>(button)];
    }

    auto Input::getMousePosition() -> float2
    {
        return g_state.mousePosition;
    }

    auto Input::getMouseDelta() -> float2
    {
        return g_state.mouseDelta;
    }

    auto Input::getMouseWheelDelta() -> float2
    {
        return g_state.mouseWheelDelta;
    }

    auto Input::isWindowFocused() -> bool
    {
        return g_state.windowFocused;
    }

    auto Input::isMouseCapturedByUi() -> bool
    {
        return g_state.uiMouseCaptured;
    }

    auto Input::isKeyboardCapturedByUi() -> bool
    {
        return g_state.uiKeyboardCaptured;
    }

    auto Input::shouldProcessMouse() -> bool
    {
        return !g_state.uiMouseCaptured;
    }

    auto Input::shouldProcessKeyboard() -> bool
    {
        return !g_state.uiKeyboardCaptured;
    }
}
