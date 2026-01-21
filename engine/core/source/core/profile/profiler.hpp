#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <source_location>
#include <unordered_map>

namespace april::core {

    struct ProfilerEvent
    {
        std::string name;
        std::string file;
        uint32_t line;
        std::thread::id threadId;
        double startCpuTime;
        double endCpuTime;
    };

    class CpuTimer
    {
    public:
        CpuTimer();
        auto reset() -> void;
        auto getMilliseconds() const -> double;
        auto getMicroseconds() const -> double;

    private:
        std::chrono::high_resolution_clock::time_point m_startTime;
    };

    class Profiler
    {
    public:
        static auto get() -> Profiler&;

        auto init() -> void;
        auto shutdown() -> void;

        // Core profiling API
        auto beginEvent(char const* name, char const* file, uint32_t line) -> void;
        auto endEvent() -> void;

        // Diagnostics / Query
        auto getThreadEventCount(std::thread::id threadId) const -> size_t;

    private:
        Profiler() = default;
        ~Profiler() = default;
        
        Profiler(Profiler const&) = delete;
        auto operator=(Profiler const&) -> Profiler& = delete;

        struct ThreadData
        {
            std::vector<ProfilerEvent> events;
            std::vector<size_t> openEventIndices; // Stack of indices into 'events'
        };

        mutable std::mutex m_mutex;
        std::unordered_map<std::thread::id, ThreadData> m_threadData;

        // Helper to access thread-local storage securely or look it up
        auto getThreadData() -> ThreadData&;
    };

    struct ScopedProfileEvent
    {
        ScopedProfileEvent(char const* name, std::source_location loc = std::source_location::current());
        ~ScopedProfileEvent();
    };

} // namespace april::core

#define AP_PROFILE_SCOPE(name) april::core::ScopedProfileEvent AP_UNIQUE_VAR_NAME(profile_scope_)(name)
#define AP_UNIQUE_VAR_NAME_CONCAT_IMPL(base, line) base ## line
#define AP_UNIQUE_VAR_NAME_CONCAT(base, line) AP_UNIQUE_VAR_NAME_CONCAT_IMPL(base, line)
#define AP_UNIQUE_VAR_NAME(base) AP_UNIQUE_VAR_NAME_CONCAT(base, __LINE__)
