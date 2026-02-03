#pragma once

#include <core/math/type.hpp>

#include <array>
#include <cstddef>

namespace april
{
    class Input final
    {
    public:
        static constexpr size_t kMaxKeys = 512;
        static constexpr size_t kMaxMouseButtons = 8;

        static auto beginFrame() -> void;

        static auto setKeyDown(int key, bool down) -> void;
        static auto setMouseButtonDown(int button, bool down) -> void;
        static auto setMousePosition(float2 const& position) -> void;
        static auto addMouseWheel(float2 const& delta) -> void;
        static auto setWindowFocused(bool focused) -> void;
        static auto setUiCapture(bool mouseCaptured, bool keyboardCaptured) -> void;

        [[nodiscard]] static auto isKeyDown(int key) -> bool;
        [[nodiscard]] static auto wasKeyPressed(int key) -> bool;
        [[nodiscard]] static auto wasKeyReleased(int key) -> bool;

        [[nodiscard]] static auto isMouseDown(int button) -> bool;
        [[nodiscard]] static auto wasMousePressed(int button) -> bool;
        [[nodiscard]] static auto wasMouseReleased(int button) -> bool;

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
