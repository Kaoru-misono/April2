#pragma once

namespace april::ui
{
    class Tooltip
    {
    public:
        static auto hover(char const* description, bool questionMark = false, float timerThreshold = 0.5f) -> void;
        static auto property(char const* propertyName, char const* description) -> void;
    };

} // namespace april::ui
