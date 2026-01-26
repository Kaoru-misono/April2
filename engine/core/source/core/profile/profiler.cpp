#include "profiler.hpp"
#include "timer.hpp"
#include "profile-manager.hpp"
#include <chrono>
#include <thread>
#include <vector>

namespace april::core
{
    inline namespace
    {
        static thread_local uint32_t t_threadId = 0;
    }
    // ProfileBuffer Implementation

    ProfileBuffer::ProfileBuffer()
    {
        m_events.resize(kMaxEvents);
        ProfileManager::get().registerBuffer(this);

        if (t_threadId == 0)
        {
            t_threadId = static_cast<uint32_t>(
                std::hash<std::thread::id>{}(std::this_thread::get_id())
            );
        }
    }

    ProfileBuffer::~ProfileBuffer()
    {
        ProfileManager::get().unregisterBuffer(this);
    }

    auto ProfileBuffer::record(const char* name, double startUs, double durationUs, ProfileEventType type) -> void
    {
        size_t index = m_writeIndex.fetch_add(1, std::memory_order_relaxed);
        if (index >= kMaxEvents)
        {
            return;
        }

        ProfileEvent& event = m_events[index];
        event.timestamp = startUs;
        event.duration = durationUs;
        event.name = name;
        event.threadId = t_threadId;
        event.type = type;

        m_commitIndex.store(index + 1, std::memory_order_release);
    }

    // TLS Instance
    static auto getThreadBuffer() -> ProfileBuffer&
    {
        static thread_local ProfileBuffer s_threadBuffer;
        return s_threadBuffer;
    }

    // Profiler Implementation

    auto Profiler::get() -> Profiler&
    {
        static Profiler instance;
        return instance;
    }

    auto Profiler::recordEvent(const char* name, double startUs, double durationUs, ProfileEventType type) -> void
    {
        getThreadBuffer().record(name, startUs, durationUs, type);
    }
}
