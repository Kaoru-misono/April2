#pragma once

#include <core/math/type.hpp>
#include <core/input/key.hpp>

#include <cstddef>

#include <array>
#include <cstddef>

namespace april
{
    class Input final
    {
    public:
        static constexpr size_t kMaxKeys = static_cast<size_t>(Key::Count);
        static constexpr size_t kMaxMouseButtons = static_cast<size_t>(MouseButton::Count);

        static auto beginFrame() -> void;

        static auto setKeyDown(Key key, bool down) -> void;
        static auto setMouseButtonDown(MouseButton button, bool down) -> void;
        static auto setMousePosition(float2 const& position) -> void;
        static auto addMouseWheel(float2 const& delta) -> void;
        static auto setWindowFocused(bool focused) -> void;
        static auto setUiCapture(bool mouseCaptured, bool keyboardCaptured) -> void;

        [[nodiscard]] static auto isKeyDown(Key key) -> bool;
        [[nodiscard]] static auto wasKeyPressed(Key key) -> bool;
        [[nodiscard]] static auto wasKeyReleased(Key key) -> bool;

        [[nodiscard]] static auto isMouseDown(MouseButton button) -> bool;
        [[nodiscard]] static auto wasMousePressed(MouseButton button) -> bool;
        [[nodiscard]] static auto wasMouseReleased(MouseButton button) -> bool;

        [[nodiscard]] static auto getMousePosition() -> float2;
        [[nodiscard]] static auto getMouseDelta() -> float2;
        [[nodiscard]] static auto getMouseWheelDelta() -> float2;

        [[nodiscard]] static auto isWindowFocused() -> bool;
        [[nodiscard]] static auto isMouseCapturedByUi() -> bool;
        [[nodiscard]] static auto isKeyboardCapturedByUi() -> bool;
        [[nodiscard]] static auto shouldProcessMouse() -> bool;
        [[nodiscard]] static auto shouldProcessKeyboard() -> bool;
    };
}
