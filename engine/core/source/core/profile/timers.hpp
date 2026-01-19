#pragma once
#include <string>
#include <cstdint>
#include <format>

namespace april::core {

class PerformanceTimer {
public:
    PerformanceTimer() { reset(); }
    void reset();
    double getSeconds() const;
    double getMilliseconds() const { return getSeconds() * 1e3; }
    double getMicroseconds() const { return getSeconds() * 1e6; }

private:
    struct TimeValue {
#ifdef __unix__
        int64_t seconds{};
        int64_t nanoseconds{};
#else
        int64_t ticks_100ns{};
#endif
    };
    TimeValue m_start{};
    TimeValue now() const;
};

class ScopedTimer {
public:
    ScopedTimer(const std::string& str);
    template<typename... Args>
    ScopedTimer(std::format_string<Args...> fmt, Args&&... args) : ScopedTimer(std::format(fmt, std::forward<Args>(args)...)) {}
    ~ScopedTimer();

private:
    void init_(const std::string& str);
    static std::string indent();

    PerformanceTimer m_timer;
    bool m_manualIndent = false;
    static inline thread_local int s_nesting = 0;
    static inline thread_local bool s_openNewline = false;
};

#define SCOPED_TIMER(name) auto scopedTimer##__LINE__ = april::core::ScopedTimer(name)

} // namespace april::core
