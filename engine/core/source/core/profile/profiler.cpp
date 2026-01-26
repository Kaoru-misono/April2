#include "profiler.hpp"
#include "timer.hpp"
#include "profile-manager.hpp"
#include <thread>

namespace april::core
{
    // ProfileBuffer Implementation

    ProfileBuffer::ProfileBuffer()
    {
        m_events.resize(kMaxEvents);
        ProfileManager::get().registerBuffer(this);
    }

    ProfileBuffer::~ProfileBuffer()
    {
        ProfileManager::get().unregisterBuffer(this);
    }

    auto ProfileBuffer::record(const char* name, ProfileEventType type) -> ProfileEvent*
    {
        size_t index = m_writeIndex.fetch_add(1, std::memory_order_relaxed);
        if (index >= kMaxEvents)
        {
            return nullptr;
        }

        ProfileEvent& event = m_events[index];
        event.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(Timer::now().time_since_epoch()).count();
        event.name = name;
        event.threadId = static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
        event.type = type;

        m_commitIndex.store(index + 1, std::memory_order_release);

        return &event;
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

    auto Profiler::startEvent(const char* name) -> void
    {
        getThreadBuffer().record(name, ProfileEventType::Begin);
    }

    auto Profiler::endEvent(const char* name) -> void
    {
        getThreadBuffer().record(name, ProfileEventType::End);
    }
}
