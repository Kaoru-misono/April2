#include "input.hpp"
#include <core/input/input.hpp>

#include <array>

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

        [[nodiscard]] auto isValidKey(Key key) -> bool
        {
            return key != Key::Unknown;
        }

        [[nodiscard]] auto isValidMouseButton(MouseButton button) -> bool
        {
            return button != MouseButton::Unknown;
        }

        [[nodiscard]] auto toIndex(Key key) -> size_t
        {
            return static_cast<size_t>(key);
        }

        [[nodiscard]] auto toIndex(MouseButton button) -> size_t
        {
            return static_cast<size_t>(button);
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

    auto Input::setKeyDown(Key key, bool down) -> void
    {
        if (!isValidKey(key))
        {
            return;
        }

        auto const index = toIndex(key);
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

    auto Input::setMouseButtonDown(MouseButton button, bool down) -> void
    {
        if (!isValidMouseButton(button))
        {
            return;
        }

        auto const index = toIndex(button);
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

    auto Input::isKeyDown(Key key) -> bool
    {
        if (!isValidKey(key))
        {
            return false;
        }

        return g_state.keysDown[toIndex(key)];
    }

    auto Input::wasKeyPressed(Key key) -> bool
    {
        if (!isValidKey(key))
        {
            return false;
        }

        return g_state.keysPressed[toIndex(key)];
    }

    auto Input::wasKeyReleased(Key key) -> bool
    {
        if (!isValidKey(key))
        {
            return false;
        }

        return g_state.keysReleased[toIndex(key)];
    }

    auto Input::isMouseDown(MouseButton button) -> bool
    {
        if (!isValidMouseButton(button))
        {
            return false;
        }

        return g_state.mouseDown[toIndex(button)];
    }

    auto Input::wasMousePressed(MouseButton button) -> bool
    {
        if (!isValidMouseButton(button))
        {
            return false;
        }

        return g_state.mousePressed[toIndex(button)];
    }

    auto Input::wasMouseReleased(MouseButton button) -> bool
    {
        if (!isValidMouseButton(button))
        {
            return false;
        }

        return g_state.mouseReleased[toIndex(button)];
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
