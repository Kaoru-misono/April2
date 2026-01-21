#include "profiler.hpp"
#include <sstream>

namespace april::core
{

    auto Profiler::get() -> Profiler&
    {
        static Profiler instance;
        return instance;
    }

    auto Profiler::init() -> void
    {
        // Initialize if needed
    }

    auto Profiler::shutdown() -> void
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_threadData.clear();
    }

    auto Profiler::getThreadData() -> std::shared_ptr<ThreadData>
    {
        // Use thread_local to cache the pointer to avoid lock on every access
        thread_local std::shared_ptr<ThreadData> currentThreadData = nullptr;

        if (currentThreadData)
        {
            return currentThreadData;
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        auto id = std::this_thread::get_id();
        auto it = m_threadData.find(id);
        if (it != m_threadData.end())
        {
            currentThreadData = it->second;
        }
        else
        {
            currentThreadData = std::make_shared<ThreadData>();
            std::stringstream ss;
            ss << id;
            currentThreadData->threadName = ss.str();
            m_threadData[id] = currentThreadData;
        }
        return currentThreadData;
    }

    auto Profiler::beginEvent(char const* name, std::source_location const& loc) -> void
    {
        auto data = getThreadData();

        ProfilerEvent event;
        event.name = name;
        event.location = loc;
        event.threadId = std::this_thread::get_id();
        event.startTime = std::chrono::high_resolution_clock::now();

        data->openEvents.push_back(event);
    }

    auto Profiler::endEvent() -> void
    {
        auto data = getThreadData();
        if (data->openEvents.empty())
        {
            return; // Error: mismatched begin/end
        }

        auto& event = data->openEvents.back();
        event.endTime = std::chrono::high_resolution_clock::now();

        data->events.push_back(event);
        data->openEvents.pop_back();
    }

        auto Profiler::getThreadEventCount(std::thread::id id) -> size_t

        {

            std::lock_guard<std::mutex> lock(m_mutex);

            auto it = m_threadData.find(id);

            if (it != m_threadData.end())

            {

                return it->second->events.size();

            }

            return 0;

        }

    

        auto Profiler::getSnapshots(std::vector<Snapshot>& frameSnapshots, std::vector<Snapshot>& singleSnapshots) -> void

        {

            std::lock_guard<std::mutex> lock(m_mutex);

            // STUB: For now, just clear them to avoid using stale data

            frameSnapshots.clear();

            singleSnapshots.clear();

        }

    

    } // namespace april::core
