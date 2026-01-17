#pragma once
#include <string>
#include <format>

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
        // KeyPressed, MouseMoved...
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
} // namespace april
