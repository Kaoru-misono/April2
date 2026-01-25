#include "profile-manager.hpp"
#include <algorithm>

namespace april::core
{
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
            const auto* events = pBuffer->getEvents();
            size_t count = pBuffer->getCount();
            
            if (count > 0)
            {
                allEvents.insert(allEvents.end(), events, events + count);
                pBuffer->reset();
            }
        }

        // Sort events by timestamp to ensure correct temporal order across threads
        std::sort(allEvents.begin(), allEvents.end(), [](const ProfileEvent& a, const ProfileEvent& b) {
            if (a.timestamp != b.timestamp) return a.timestamp < b.timestamp;
            return a.type < b.type; // Consistent ordering for simultaneous events
        });

        return allEvents;
    }
}