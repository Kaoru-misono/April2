#include "profiler.hpp"

namespace april::core {

    // --- CpuTimer ---

    CpuTimer::CpuTimer()
    {
        reset();
    }

    auto CpuTimer::reset() -> void
    {
        m_startTime = std::chrono::high_resolution_clock::now();
    }

    auto CpuTimer::getMilliseconds() const -> double
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration<double, std::milli>(now - m_startTime);
        return ms.count();
    }

    auto CpuTimer::getMicroseconds() const -> double
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration<double, std::micro>(now - m_startTime);
        return us.count();
    }

    // --- Snapshot ---

    auto Snapshot::appendToString(std::string& stats, bool full) const -> void
    {
        // Placeholder implementation
        (void)stats;
        (void)full;
    }

    // --- Profiler ---

    auto Profiler::get() -> Profiler&
    {
        static auto instance = Profiler{};
        return instance;
    }

    auto Profiler::init() -> void
    {
        auto lock = std::lock_guard<std::mutex>{m_mutex};
        m_threadData.clear();
    }

    auto Profiler::shutdown() -> void
    {
        auto lock = std::lock_guard<std::mutex>{m_mutex};
        m_threadData.clear();
    }

    // Thread-local pointer to the data for the current thread.
    // This allows fast access without locking the global map every time.
    // Defined inside the function to have access to private ThreadData type.
    auto Profiler::getThreadData() -> ThreadData&
    {
        static thread_local ThreadData* tls_threadData = nullptr;

        if (tls_threadData)
        {
            return *tls_threadData;
        }

        // Slow path: First time this thread is profiling.
        auto lock = std::lock_guard<std::mutex>{m_mutex};
        auto id = std::this_thread::get_id();
        // Insert and get reference. unordered_map references are stable.
        tls_threadData = &m_threadData[id];
        return *tls_threadData;
    }

    auto Profiler::beginEvent(char const* name, char const* file, uint32_t line) -> void
    {
        auto& data = getThreadData();
        
        auto now = std::chrono::high_resolution_clock::now();
        auto startCpuTime = std::chrono::duration<double, std::micro>(now.time_since_epoch()).count();

        auto event = ProfilerEvent{
            .name = name ? name : "",
            .file = file ? file : "",
            .line = line,
            .threadId = std::this_thread::get_id(),
            .startCpuTime = startCpuTime,
            .endCpuTime = 0.0,
            .gpuDuration = 0.0
        };

        data.events.push_back(event);
        data.openEventIndices.push_back(data.events.size() - 1);
    }

    auto Profiler::endEvent() -> void
    {
        auto& data = getThreadData();
        if (data.openEventIndices.empty())
        {
            return;
        }

        auto index = data.openEventIndices.back();
        data.openEventIndices.pop_back();

        auto now = std::chrono::high_resolution_clock::now();
        data.events[index].endCpuTime = std::chrono::duration<double, std::micro>(now.time_since_epoch()).count();
    }

    auto Profiler::addGpuEvent(char const* name, double duration) -> void
    {
        auto& data = getThreadData();
        
        // GPU events are currently added as "completed" events with 0 CPU duration 
        // but with valid GPU duration.
        // In the future, we might want to unify them better.
        auto event = ProfilerEvent{
            .name = name ? name : "",
            .file = "",
            .line = 0,
            .threadId = std::this_thread::get_id(),
            .startCpuTime = 0.0,
            .endCpuTime = 0.0,
            .gpuDuration = duration
        };

        data.events.push_back(event);
    }

    auto Profiler::getThreadEventCount(std::thread::id threadId) const -> size_t
    {
        auto lock = std::lock_guard<std::mutex>{m_mutex};
        auto it = m_threadData.find(threadId);
        if (it != m_threadData.end())
        {
            return it->second.events.size();
        }
        return 0;
    }

    auto Profiler::getSnapshots(std::vector<Snapshot>& frameSnapshots, std::vector<Snapshot>& asyncSnapshots) const -> void
    {
        // Placeholder implementation for UI compatibility.
        // In Phase 3, this will be populated with aggregated data from m_threadData.
        frameSnapshots.clear();
        asyncSnapshots.clear();
    }

    // --- ScopedProfileEvent ---

    ScopedProfileEvent::ScopedProfileEvent(char const* name, std::source_location loc)
    {
        Profiler::get().beginEvent(name, loc.file_name(), loc.line());
    }

    ScopedProfileEvent::~ScopedProfileEvent()
    {
        Profiler::get().endEvent();
    }

} // namespace april::core
