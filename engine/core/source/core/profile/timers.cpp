#include "timers.hpp"

#include <core/log/logger.hpp>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <realtimeapiset.h>
#elif defined(__unix__)
#include <time.h>
#else
#include <chrono>
#endif

namespace april::core
{

    auto PerformanceTimer::now() const -> PerformanceTimer::TimeValue
    {
    #if defined(_WIN32)
        static auto frequency = int64_t{0};
        if (frequency == 0) {
            auto freq = LARGE_INTEGER{};
            QueryPerformanceFrequency(&freq);
            frequency = freq.QuadPart;
        }
        auto count = LARGE_INTEGER{};
        QueryPerformanceCounter(&count);
        // Convert to 100ns ticks
        // count * 10,000,000 / frequency
        return {.ticks_100ns = count.QuadPart * 10000000 / frequency};
    #elif defined(__unix__)
    #ifdef __APPLE__
        constexpr auto clockID = CLOCK_UPTIME_RAW;
    #else
        constexpr auto clockID = CLOCK_MONOTONIC;
    #endif
        auto tv = timespec{};
        clock_gettime(clockID, &tv);
        return {.seconds = static_cast<int64_t>(tv.tv_sec), .nanoseconds = static_cast<int64_t>(tv.tv_nsec)};
    #else
        auto time = std::chrono::steady_clock::now().time_since_epoch();
        using ns100 = std::chrono::duration<int64_t, std::ratio<1, 10000000>>;
        return {.ticks_100ns = std::chrono::duration_cast<ns100>(time).count()};
    #endif
    }

    auto PerformanceTimer::reset() -> void
    {
        m_start = now();
    }

    auto PerformanceTimer::getSeconds() const -> double
    {
    #ifdef __unix__
        auto const t = now();
        auto const delta = 1e-9 * static_cast<double>(t.nanoseconds - m_start.nanoseconds)
                            + static_cast<double>(t.seconds - m_start.seconds);
        return delta >= 0 ? delta : 0.;
    #else
        auto const delta = now().ticks_100ns - m_start.ticks_100ns;
        return delta >= 0 ? static_cast<double>(delta) * 1e-7 : 0.;
    #endif
    }

    ScopedTimer::ScopedTimer(std::string const& str) { init(str); }

    auto ScopedTimer::init(std::string const& str) -> void
    {
        if(s_openNewline) {
            AP_INFO("\n");
        }
        m_manualIndent = !str.empty() && (str[0] == ' ' || str[0] == '-' || str[0] == '|');
        if(s_nesting > 0 && !m_manualIndent) {
            AP_INFO("{}", indent());
        }
        AP_INFO("{}", str);
        s_openNewline = str.empty() || str.back() != '\n';
        ++s_nesting;
    }

    ScopedTimer::~ScopedTimer()
    {
        --s_nesting;
        if(!s_openNewline && !m_manualIndent) {
            AP_INFO("{}|", indent());
        } else {
            AP_INFO(" ");
        }
        AP_INFO("-> {:.3f} ms\n", m_timer.getMilliseconds());
        s_openNewline = false;
    }

    auto ScopedTimer::indent() -> std::string
    {
        auto result = std::string(static_cast<size_t>(s_nesting * 2), ' ');
        for(int i = 0; i < s_nesting * 2; i += 2) result[i] = '|';
        return result;
    }

} // namespace april::core
