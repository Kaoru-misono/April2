#include "timer.hpp"

namespace april::core
{
    auto Timer::now() -> TimePoint
    {
        return Clock::now();
    }

    auto Timer::calcDuration(TimePoint start, TimePoint end) -> double
    {
        auto delta = end - start;
        return std::chrono::duration<double, std::milli>(delta).count();
    }

    auto Timer::calcDurationMicroseconds(TimePoint start, TimePoint end) -> int64_t
    {
        auto delta = end - start;
        return std::chrono::duration_cast<std::chrono::microseconds>(delta).count();
    }

    auto Timer::calcDurationNanoseconds(TimePoint start, TimePoint end) -> int64_t
    {
        auto delta = end - start;
        return std::chrono::duration_cast<std::chrono::nanoseconds>(delta).count();
    }
}
