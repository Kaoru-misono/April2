#pragma once

#include "event.hpp"
#include "../tools/exclusive.hpp"

#include <core/math/math.hpp>
#include <string>
#include <functional>
#include <memory>

namespace april
{
    enum struct EWindowType
    {
        none,
        windows,
        glfw,
        sdl,
    };

    struct WindowDesc
    {
        std::string title{"April"};
        EWindowType type{EWindowType::glfw};
        unsigned int width{1280};
        unsigned int height{720};
    };

    struct Window: ThreadExclusive<Window>
    {
        using EventCallbackFn = std::function<void(Event&)>;

        virtual ~Window() = default;

        virtual auto onEvent() -> void = 0;

        virtual auto getWidth() const -> unsigned int = 0;
        virtual auto getHeight() const -> unsigned int = 0;

        virtual auto getFramebufferWidth() const -> unsigned int = 0;
        virtual auto getFramebufferHeight() const -> unsigned int = 0;

        virtual auto getWindowContentScale() const -> float2 = 0;

        virtual auto setVSync(bool enabled) -> void = 0;
        virtual auto isVSync() const -> bool = 0;

        virtual auto getBackendWindow() const -> void* = 0;
        virtual auto getNativeWindowHandle() const -> void* = 0;

        static auto create(WindowDesc const& desc = WindowDesc{}) -> std::unique_ptr<Window>;

        template <typename T, typename CallbackFn>
        auto subscribe(CallbackFn&& callback) -> void
        requires std::is_base_of_v<Event, T>
        {
            auto type = T::Type();

            auto wrapper = [func = std::forward<CallbackFn>(callback)](Event& e) {
                if (e.getType() == T::Type())
                {
                    func(static_cast<T&>(e));
                }
            };

            registerCallbackImpl(type, wrapper);
        }

    protected:
        virtual auto registerCallbackImpl(EventType type, EventCallbackFn callback) -> void = 0;
    };
}
