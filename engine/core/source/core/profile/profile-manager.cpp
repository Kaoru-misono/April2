#include "profile-manager.hpp"
#include "profiler.hpp"
#include <algorithm>

namespace april::core
{
    ProfileManager::ProfileManager()
    {
        // Pre-register GPU Queue
        m_threadNames[0xFFFFFFFF] = "GPU Queue";
    }

    auto ProfileManager::registerThreadName(uint32_t tid, std::string const& name) -> void
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_threadNames[tid] = name;
    }

    auto ProfileManager::getThreadNames() const -> std::map<uint32_t, std::string> const&
    {
        return m_threadNames;
    }

    auto ProfileManager::get() -> ProfileManager&
    {
        static ProfileManager instance;
        return instance;
    }

    auto ProfileManager::registerBuffer(ProfileBuffer* pBuffer) -> void
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        // Only add if not already present
        if (std::find(m_buffers.begin(), m_buffers.end(), pBuffer) == m_buffers.end())
        {
            m_buffers.push_back(pBuffer);
        }
    }

    auto ProfileManager::unregisterBuffer(ProfileBuffer* pBuffer) -> void
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_buffers.erase(std::remove(m_buffers.begin(), m_buffers.end(), pBuffer), m_buffers.end());
    }

    auto ProfileManager::flush() -> std::vector<ProfileEvent>
    {
        std::vector<ProfileEvent> allEvents;

        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto* pBuffer : m_buffers)
        {
            auto const* events = pBuffer->getEvents();
            size_t count = pBuffer->getCount();

            if (count > 0)
            {
                allEvents.insert(allEvents.end(), events, events + count);
                pBuffer->reset();
            }
        }

        // Aggregate GPU events if a provider is registered
        if (auto* pGpuProfiler = Profiler::get().getGpuProfiler())
        {
            auto gpuEvents = pGpuProfiler->collectEvents();
            if (!gpuEvents.empty())
            {
                allEvents.insert(allEvents.end(), gpuEvents.begin(), gpuEvents.end());
            }
        }

        // Sort events by timestamp to ensure correct temporal order across threads and GPU
        std::sort(allEvents.begin(), allEvents.end(), [](ProfileEvent const& a, ProfileEvent const& b) {
            if (a.timestamp != b.timestamp) return a.timestamp < b.timestamp;
            return a.type < b.type; // Consistent ordering for simultaneous events
        });

        return allEvents;
    }
}
