#pragma once
#include <string>
#include <string_view>
#include <format>
#include <cstdint>

namespace april
{
    enum struct EventType
    {
        None = 0,
        WindowClose,
        WindowResize,
        FrameBufferResize,
        WindowFocus,
        WindowLostFocus,
        KeyPressed,
        KeyReleased,
        KeyRepeated,
        MouseButtonPressed,
        MouseButtonReleased,
        MouseMoved,
        MouseScrolled,
        CharInput,
    };

    #define DEFINE_EVENT(type) \
        static auto Type() -> EventType { return EventType::type; } \
        virtual auto getType() const -> EventType override { return Type(); } \
        virtual auto getName() const -> std::string_view override { return #type; } \
        static_assert(true)

    struct Event
    {
        virtual ~Event() = default;

        [[nodiscard]] virtual auto getType() const -> EventType = 0;

        [[nodiscard]] virtual auto getName() const -> std::string_view = 0;

        bool handled{false};
    };

    struct WindowCloseEvent final: Event
    {
        DEFINE_EVENT(WindowClose);
    };

    struct WindowResizeEvent final: Event
    {
        uint32_t width{};
        uint32_t height{};

        WindowResizeEvent(uint32_t pWidth, uint32_t pHeight)
            : width(pWidth), height(pHeight)
        {}

        DEFINE_EVENT(WindowResize);

        auto toString() const -> std::string
        {
            return std::format("WindowResize: {}, {})", width, height);
        }
    };

    struct FrameBufferResizeEvent final: Event
    {
        uint32_t width{};
        uint32_t height{};

        FrameBufferResizeEvent(uint32_t pWidth, uint32_t pHeight)
            : width(pWidth), height(pHeight)
        {}

        DEFINE_EVENT(FrameBufferResize);

        auto toString() const -> std::string
        {
            return std::format("Framebuffer: {}, {}", width, height);
        }
    };

    struct WindowFocusEvent final: Event
    {
        DEFINE_EVENT(WindowFocus);
    };

    struct WindowLostFocusEvent final: Event
    {
        DEFINE_EVENT(WindowLostFocus);
    };

    struct KeyPressedEvent final: Event
    {
        int key{};
        int scancode{};
        int mods{};

        KeyPressedEvent(int pKey, int pScancode, int pMods)
            : key(pKey), scancode(pScancode), mods(pMods)
        {}

        DEFINE_EVENT(KeyPressed);
    };

    struct KeyReleasedEvent final: Event
    {
        int key{};
        int scancode{};
        int mods{};

        KeyReleasedEvent(int pKey, int pScancode, int pMods)
            : key(pKey), scancode(pScancode), mods(pMods)
        {}

        DEFINE_EVENT(KeyReleased);
    };

    struct KeyRepeatedEvent final: Event
    {
        int key{};
        int scancode{};
        int mods{};

        KeyRepeatedEvent(int pKey, int pScancode, int pMods)
            : key(pKey), scancode(pScancode), mods(pMods)
        {}

        DEFINE_EVENT(KeyRepeated);
    };

    struct CharInputEvent final: Event
    {
        uint32_t codepoint{};

        explicit CharInputEvent(uint32_t pCodepoint)
            : codepoint(pCodepoint)
        {}

        DEFINE_EVENT(CharInput);
    };

    struct MouseMovedEvent final: Event
    {
        float x{};
        float y{};

        MouseMovedEvent(float pX, float pY)
            : x(pX), y(pY)
        {}

        DEFINE_EVENT(MouseMoved);
    };

    struct MouseScrolledEvent final: Event
    {
        float xOffset{};
        float yOffset{};

        MouseScrolledEvent(float pXOffset, float pYOffset)
            : xOffset(pXOffset), yOffset(pYOffset)
        {}

        DEFINE_EVENT(MouseScrolled);
    };

    struct MouseButtonPressedEvent final: Event
    {
        int button{};
        int mods{};

        MouseButtonPressedEvent(int pButton, int pMods)
            : button(pButton), mods(pMods)
        {}

        DEFINE_EVENT(MouseButtonPressed);
    };

    struct MouseButtonReleasedEvent final: Event
    {
        int button{};
        int mods{};

        MouseButtonReleasedEvent(int pButton, int pMods)
            : button(pButton), mods(pMods)
        {}

        DEFINE_EVENT(MouseButtonReleased);
    };
} // namespace april
