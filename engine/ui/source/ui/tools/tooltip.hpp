#pragma once

namespace april::ui
{
    class Tooltip
    {
    public:
        static auto hover(const char* description, bool questionMark = false, float timerThreshold = 0.5f) -> void;
        static auto property(const char* propertyName, const char* description) -> void;
    };

} // namespace april::ui