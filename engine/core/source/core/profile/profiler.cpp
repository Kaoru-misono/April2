#include "profiler.hpp"

#include <sstream>
#include <algorithm>
#include <fstream>

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
        m_frameCount = 0;
    }

    auto Profiler::shutdown() -> void
    {
        auto lock = std::lock_guard<std::mutex>{m_mutex};
        m_threadData.clear();
    }

    auto Profiler::beginFrame() -> void
    {
        // For now, assume beginFrame is called from main thread.
        // It doesn't strictly need to do anything globally yet,
        // but it could signal threads to rotate their buffers.
    }

    auto Profiler::endFrame() -> void
    {
        auto lock = std::lock_guard<std::mutex>{m_mutex};
        for (auto& [id, data] : m_threadData)
        {
            if (data.history.size() < ThreadData::kMaxHistoryFrames)
            {
                data.history.push_back(std::move(data.currentFrameEvents));
            }
            else
            {
                data.history[data.historyWriteIndex] = std::move(data.currentFrameEvents);
                data.historyWriteIndex = (data.historyWriteIndex + 1) % ThreadData::kMaxHistoryFrames;
            }
            data.currentFrameEvents.clear();
            data.openEventIndices.clear();
        }
        m_frameCount++;
    }

    // Helper to access thread-local storage securely or look it up
    auto Profiler::getThreadData() -> ThreadData&
    {
        // Safe implementation: always lock and look up.
        // Caching pointers to map elements is unsafe if the map is cleared (e.g. in init/shutdown).
        auto lock = std::lock_guard<std::mutex>{m_mutex};
        auto id = std::this_thread::get_id();
        return m_threadData[id];
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
            .gpuDuration = 0.0,
            .level = static_cast<uint32_t>(data.openEventIndices.size())
        };

        data.currentFrameEvents.push_back(event);
        data.openEventIndices.push_back(data.currentFrameEvents.size() - 1);
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
        data.currentFrameEvents[index].endCpuTime = std::chrono::duration<double, std::micro>(now.time_since_epoch()).count();
    }

    auto Profiler::addGpuEvent(char const* name, double duration, uint32_t level) -> void
    {
        auto& data = getThreadData();

        // GPU events are currently added as "completed" events with 0 CPU duration
        // but with valid GPU duration.
        auto event = ProfilerEvent{
            .name = name ? name : "",
            .file = "",
            .line = 0,
            .threadId = std::this_thread::get_id(),
            .startCpuTime = 0.0,
            .endCpuTime = 0.0,
            .gpuDuration = duration,
            .level = level
        };

        data.currentFrameEvents.push_back(event);
    }

    auto Profiler::getThreadEventCount(std::thread::id threadId) const -> size_t
    {
        auto lock = std::lock_guard<std::mutex>{m_mutex};
        auto it = m_threadData.find(threadId);
        if (it != m_threadData.end())
        {
            return it->second.currentFrameEvents.size();
        }
        return 0;
    }

    auto Profiler::getSnapshots(std::vector<Snapshot>& frameSnapshots, std::vector<Snapshot>& asyncSnapshots) const -> void
    {
        auto lock = std::lock_guard<std::mutex>{m_mutex};

        frameSnapshots.clear();
        asyncSnapshots.clear();

        for (auto const& [threadId, data] : m_threadData)
        {
            if (data.history.empty()) continue;

            auto snapshot = Snapshot{};
            snapshot.name = "Thread";

            auto const& lastFrame = data.history.back();

            for (size_t i = 0; i < lastFrame.size(); ++i)
            {
                auto const& event = lastFrame[i];
                auto info = TimerInfo{};

                double totalCpu = 0.0;
                double totalGpu = 0.0;
                info.cpu.absMinValue = 1e30;
                info.gpu.absMinValue = 1e30;
                uint32_t actualCount = 0;

                uint32_t frameCounter = 0;
                for (auto const& frame : data.history)
                {
                    if (i < frame.size())
                    {
                        auto const& hEvent = frame[i];
                        double cpuDur = hEvent.endCpuTime - hEvent.startCpuTime;
                        double gpuDur = hEvent.gpuDuration;

                        totalCpu += cpuDur;
                        totalGpu += gpuDur;

                        info.cpu.absMinValue = std::min(info.cpu.absMinValue, cpuDur);
                        info.cpu.absMaxValue = std::max(info.cpu.absMaxValue, cpuDur);
                        info.gpu.absMinValue = std::min(info.gpu.absMinValue, gpuDur);
                        info.gpu.absMaxValue = std::max(info.gpu.absMaxValue, gpuDur);

                        info.cpu.last = cpuDur;
                        info.gpu.last = gpuDur;

                        if (frameCounter < TimerStats::kMaxLastFrames)
                        {
                            info.cpu.times[frameCounter] = cpuDur;
                            info.gpu.times[frameCounter] = gpuDur;
                        }

                        actualCount++;
                        frameCounter++;
                    }
                }

                if (actualCount > 0)
                {
                    info.cpu.average = totalCpu / actualCount;
                    info.gpu.average = totalGpu / actualCount;
                    info.numAveraged = actualCount;
                    info.level = event.level;
                    info.cpu.index = actualCount; // Points to next or count
                    info.gpu.index = actualCount;
                }
                else
                {
                    info.cpu.absMinValue = 0.0;
                    info.gpu.absMinValue = 0.0;
                }

                snapshot.timerNames.push_back(event.name);
                snapshot.timerInfos.push_back(info);
            }

            frameSnapshots.push_back(std::move(snapshot));
        }
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

    auto Profiler::serializeToJson(std::string const& filePath) -> void
    {
        auto lock = std::lock_guard<std::mutex>{m_mutex};

        auto outFile = std::ofstream{filePath};
        if (!outFile.is_open()) return;

        outFile << "[\n";
        bool first = true;

        for (auto const& [threadId, data] : m_threadData)
        {
            for (auto const& frame : data.history)
            {
                for (auto const& event : frame)
                {
                    if (!first) outFile << ",\n";
                    first = false;

                    // CPU Event
                    outFile << "  { \"name\": \"" << event.name << "\", "
                            << "\"cat\": \"PERF\", "
                            << "\"ph\": \"X\", "
                            << "\"ts\": " << event.startCpuTime << ", "
                            << "\"dur\": " << (event.endCpuTime - event.startCpuTime) << ", "
                            << "\"pid\": 1, "
                            << "\"tid\": " << threadId << " }";

                    // If it has GPU duration, add it as a sub-event or metadata
                    if (event.gpuDuration > 0)
                    {
                        outFile << ",\n";
                        outFile << "  { \"name\": \"" << event.name << " (GPU)\", "
                                << "\"cat\": \"GPU\", "
                                << "\"ph\": \"X\", "
                                << "\"ts\": " << event.startCpuTime << ", "
                                << "\"dur\": " << event.gpuDuration << ", "
                                << "\"pid\": 1, "
                                << "\"tid\": " << threadId << ", "
                                << "\"args\": { \"type\": \"gpu_resolved\" } }";
                    }
                }
            }
        }

        outFile << "\n]\n";
        outFile.close();
    }

} // namespace april::core
