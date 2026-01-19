#pragma once

#include <string>
#include <cstdint>
#include <format>

namespace april::core
{
    class PerformanceTimer
    {
    public:
        PerformanceTimer() { reset(); }
        auto reset() -> void;
        auto getSeconds() const -> double;
        auto getMilliseconds() const -> double { return getSeconds() * 1e3; }
        auto getMicroseconds() const -> double { return getSeconds() * 1e6; }

    private:
        struct TimeValue
        {
    #ifdef __unix__
            int64_t seconds{};
            int64_t nanoseconds{};
    #else
            int64_t ticks_100ns{};
    #endif
        };
        TimeValue m_start{};
        auto now() const -> TimeValue;
    };

    class ScopedTimer
    {
    public:
        ScopedTimer(std::string const& str);
        template<typename... Args>
        ScopedTimer(std::format_string<Args...> fmt, Args&&... args) : ScopedTimer(std::format(fmt, std::forward<Args>(args)...)) {}
        ~ScopedTimer();

    private:
        auto init(std::string const& str) -> void;
        static auto indent() -> std::string;

        PerformanceTimer m_timer;
        bool m_manualIndent = false;
        static inline thread_local int s_nesting = 0;
        static inline thread_local bool s_openNewline = false;
    };

#define SCOPED_TIMER(name) auto scopedTimer##__LINE__ = april::core::ScopedTimer(name)

} // namespace april::core
