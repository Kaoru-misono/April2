#pragma once

#include <string>
#include <vector>
#include <array>
#include <chrono>
#include <thread>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <source_location>

namespace april::core
{
    /**
    * @brief Simple high-resolution timer.
    */
    class CpuTimer
    {
    public:
        CpuTimer()
        {
            reset();
        }

        auto reset() -> void
        {
            m_start = std::chrono::high_resolution_clock::now();
        }

        auto getSeconds() const -> double
        {
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> diff = end - m_start;
            return diff.count();
        }

        auto getMilliseconds() const -> double
        {
            return getSeconds() * 1000.0;
        }

    private:
        std::chrono::high_resolution_clock::time_point m_start;
    };

    /**
    * @brief Represents a single profiling event (a scope execution).
    */
    struct ProfilerEvent
    {
        char const* name{nullptr};
        std::source_location location;
        std::thread::id threadId;
        std::chrono::high_resolution_clock::time_point startTime;
        std::chrono::high_resolution_clock::time_point endTime;

        // Future expansion:
        // std::uint64_t gpuStartTime{0};
        // std::uint64_t gpuEndTime{0};
    };

    /**
    * @brief Statistics for a single timer over a window of frames.
    */
    struct TimerStats
    {
        static constexpr uint32_t kMaxLastFrames = 128;

        double last{0.0};
        double average{0.0};
        double absMinValue{0.0};
        double absMaxValue{0.0};
        uint32_t index{0};
        std::array<double, kMaxLastFrames> times{};
    };

    /**
    * @brief Aggregated information for a specific profiled scope.
    */
    struct TimerInfo
    {
        uint32_t numAveraged{0};
        bool async{false};
        uint32_t level{0};
        TimerStats cpu;
        TimerStats gpu;
    };

    /**
    * @brief A point-in-time capture of aggregated profiling data.
    */
    struct Snapshot
    {
        std::string name;
        std::vector<TimerInfo> timerInfos;
        std::vector<std::string> timerNames;
        std::vector<std::string> timerApiNames;
    };

    /**
    * @brief Central manager for the profiling system.
    *
    * Handles per-thread event storage and aggregation.
    */
    class Profiler
    {
    public:
        static auto get() -> Profiler&;

        auto init() -> void;
        auto shutdown() -> void;

        /**
        * @brief Begins a new CPU event on the current thread.
        * @param name Name of the event (should be a string literal or persistent string).
        * @param loc Source location (automatically captured).
        */
        auto beginEvent(char const* name, std::source_location const& loc = std::source_location::current()) -> void;

        /**
        * @brief Ends the current open CPU event on the current thread.
        */
        auto endEvent() -> void;

        /**
        * @brief Captures aggregated data into snapshots for UI visualization.
        */
        auto getSnapshots(std::vector<Snapshot>& frameSnapshots, std::vector<Snapshot>& singleSnapshots) -> void;

        // TODO: Phase 3 - getEvents() and serialization
        // For testing purposes
        auto getThreadEventCount(std::thread::id id) -> size_t;

    private:
        Profiler() = default;
        ~Profiler() = default;

        Profiler(Profiler const&) = delete;
        Profiler& operator=(Profiler const&) = delete;

        struct ThreadData
        {
            std::vector<ProfilerEvent> events;
            std::vector<ProfilerEvent> openEvents; // Stack for nested events
            std::string threadName;
            // In a real sliding window impl, we'd use a ring buffer or clear 'events' periodically
        };

        std::mutex m_mutex;
        std::unordered_map<std::thread::id, std::shared_ptr<ThreadData>> m_threadData;

        auto getThreadData() -> std::shared_ptr<ThreadData>;
    };

    /**
    * @brief Helper class for RAII-based scoping.
    */
    class ScopedProfile
    {
    public:
        ScopedProfile(char const* name, std::source_location const& loc = std::source_location::current())
        {
            Profiler::get().beginEvent(name, loc);
        }

        ~ScopedProfile()
        {
            Profiler::get().endEvent();
        }

        ScopedProfile(ScopedProfile const&) = delete;
        ScopedProfile& operator=(ScopedProfile const&) = delete;
    };

} // namespace april::core

#define AP_PROFILE_SCOPE(name) ::april::core::ScopedProfile _profile_scope_##__LINE__(name)
#define AP_PROFILE_FUNCTION() AP_PROFILE_SCOPE(__FUNCTION__)
