#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <source_location>
#include <map>
#include <array>

namespace april::core {

    struct ProfilerEvent
    {
        std::string name;
        std::string file;
        uint32_t line;
        std::thread::id threadId;
        double startCpuTime;
        double endCpuTime;
        double gpuDuration; // In microseconds
        uint32_t level = 0;
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

    // Forward declarations for UI compatibility
    struct TimerStats
    {
        static constexpr uint32_t kMaxLastFrames = 128;

        double last = 0.0;
        double average = 0.0;
        double absMinValue = 0.0;
        double absMaxValue = 0.0;
        uint32_t index = 0;
        std::array<double, kMaxLastFrames> times = {};
    };

    struct TimerInfo
    {
        uint32_t numAveraged = 0;
        bool accumulated = false;
        bool async = false;
        uint32_t level = 0;

        TimerStats cpu;
        TimerStats gpu;
    };

    class Snapshot
    {
    public:
        std::string name;
        size_t id = 0;

        std::vector<TimerInfo> timerInfos;
        std::vector<std::string> timerNames;
        std::vector<std::string> timerApiNames;

        auto appendToString(std::string& stats, bool full) const -> void;
    };

    class Profiler
    {
    public:
        static auto get() -> Profiler&;

        auto init() -> void;
        auto shutdown() -> void;

        // Frame management
        auto beginFrame() -> void;
        auto endFrame() -> void;

        // Core profiling API
        auto beginEvent(char const* name, char const* file, uint32_t line) -> void;
        auto endEvent() -> void;

        /**
         * Adds a GPU event that was resolved from the graphics module.
         * Since GPU events are resolved asynchronously, they are added after they complete.
         */
        auto addGpuEvent(char const* name, double duration, uint32_t level = 0) -> void;

        // Diagnostics / Query
        auto getThreadEventCount(std::thread::id threadId) const -> size_t;

        // UI Compatibility
        auto getSnapshots(std::vector<Snapshot>& frameSnapshots, std::vector<Snapshot>& asyncSnapshots) const -> void;

        // Export
        auto serializeToJson(std::string const& filePath) -> void;

    private:
        Profiler() = default;
        ~Profiler() = default;

        Profiler(Profiler const&) = delete;
        auto operator=(Profiler const&) -> Profiler& = delete;

        struct ThreadData
        {
            std::vector<ProfilerEvent> currentFrameEvents;
            std::vector<size_t> openEventIndices; // Stack of indices into 'currentFrameEvents'

            // History for aggregation (sliding window of frames)
            static constexpr size_t kMaxHistoryFrames = 128;
            std::vector<std::vector<ProfilerEvent>> history;
            size_t historyWriteIndex = 0;
        };

        mutable std::mutex m_mutex;
        std::map<std::thread::id, ThreadData> m_threadData;
        uint32_t m_frameCount = 0;

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
#define AP_PROFILE_FUNCTION() AP_PROFILE_SCOPE(std::source_location::current().function_name())
#define AP_UNIQUE_VAR_NAME_CONCAT_IMPL(base, line) base ## line
#define AP_UNIQUE_VAR_NAME_CONCAT(base, line) AP_UNIQUE_VAR_NAME_CONCAT_IMPL(base, line)
#define AP_UNIQUE_VAR_NAME(base) AP_UNIQUE_VAR_NAME_CONCAT(base, __LINE__)