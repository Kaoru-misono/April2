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
        unsigned int width{};
        unsigned int height{};

        WindowResizeEvent(unsigned int w, unsigned int h): width(w), height(h) {}

        DEFINE_EVENT(WindowResize);

        auto toString() const -> std::string
        {
            return std::format("WindowResize: {}, {}", width, height);
        }
    };
} // namespace april
