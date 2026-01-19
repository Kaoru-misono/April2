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

namespace april::core {

PerformanceTimer::TimeValue PerformanceTimer::now() const {
#if defined(_WIN32)
    static int64_t frequency = 0;
    if (frequency == 0) {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        frequency = freq.QuadPart;
    }
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    // Convert to 100ns ticks
    // count * 10,000,000 / frequency
    return {.ticks_100ns = count.QuadPart * 10000000 / frequency};
#elif defined(__unix__)
#ifdef __APPLE__
    constexpr clockid_t clockID = CLOCK_UPTIME_RAW;
#else
    constexpr clockid_t clockID = CLOCK_MONOTONIC;
#endif
    timespec tv{};
    clock_gettime(clockID, &tv);
    return {.seconds = static_cast<int64_t>(tv.tv_sec), .nanoseconds = static_cast<int64_t>(tv.tv_nsec)};
#else
    auto time = std::chrono::steady_clock::now().time_since_epoch();
    using ns100 = std::chrono::duration<int64_t, std::ratio<1, 10000000>>;
    return {.ticks_100ns = std::chrono::duration_cast<ns100>(time).count()};
#endif
}

void PerformanceTimer::reset() { m_start = now(); }

double PerformanceTimer::getSeconds() const {
#ifdef __unix__
    const TimeValue t = now();
    const double delta = 1e-9 * static_cast<double>(t.nanoseconds - m_start.nanoseconds)
                         + static_cast<double>(t.seconds - m_start.seconds);
    return delta >= 0 ? delta : 0.;
#else
    const int64_t delta = now().ticks_100ns - m_start.ticks_100ns;
    return delta >= 0 ? static_cast<double>(delta) * 1e-7 : 0.;
#endif
}

ScopedTimer::ScopedTimer(const std::string& str) { init_(str); }

void ScopedTimer::init_(const std::string& str) {
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

ScopedTimer::~ScopedTimer() {
    --s_nesting;
    if(!s_openNewline && !m_manualIndent) {
        AP_INFO("{}\
|", indent());
    } else {
        AP_INFO(" ");
    }
    AP_INFO("-> {:.3f} ms\n", m_timer.getMilliseconds());
    s_openNewline = false;
}

std::string ScopedTimer::indent() {
    std::string result(static_cast<size_t>(s_nesting * 2), ' ');
    for(int i = 0; i < s_nesting * 2; i += 2) result[i] = '|';
    return result;
}

} // namespace april::core
