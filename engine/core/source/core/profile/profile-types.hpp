#pragma once

#include <cstdint>
#include <vector>
#include <atomic>
#include <string>

namespace april::core
{
    /**
     * Profiler event types.
     */
    enum class ProfileEventType : uint8_t
    {
        Complete,
        Instant
    };

    /**
     * Cache-optimized profiler event structure.
     * Members:
     * - double timestamp (8 bytes)
     * - double duration (8 bytes)
     * - char const* name (8 bytes)
     * - uint32_t threadId (4 bytes)
     * - ProfileEventType type (1 byte)
     * - padding (3 bytes)
     * Total: 32 bytes
     */
    struct alignas(32) ProfileEvent
    {
        double timestamp;
        double duration;
        char const* name;
        uint32_t threadId;
        ProfileEventType type;
        uint8_t padding[3];
    };

    static_assert(sizeof(ProfileEvent) == 32, "ProfileEvent must be exactly 32 bytes");

    /**
     * Thread-local buffer for storing profiler events.
     */
    class ProfileBuffer
    {
    public:
        static constexpr size_t kMaxEvents = 256 * 1024; // Increased capacity for high-frequency profiling

        ProfileBuffer();
        ~ProfileBuffer();

        /**
         * Records a new event in the buffer.
         */
        auto record(char const* name, double startUs, double durationUs, ProfileEventType type = ProfileEventType::Complete) -> void;

        /**
         * Resets the buffer for the next frame.
         */
        auto reset() -> void
        {
            m_writeIndex.store(0, std::memory_order_relaxed);
            m_commitIndex.store(0, std::memory_order_relaxed);
        }

        auto getEvents() const -> ProfileEvent const* { return m_events.data(); }
        auto getCount() const -> size_t { return m_commitIndex.load(std::memory_order_acquire); }

    private:
        std::vector<ProfileEvent> m_events;
        std::atomic<size_t> m_writeIndex{0};
        std::atomic<size_t> m_commitIndex{0};
    };
}
