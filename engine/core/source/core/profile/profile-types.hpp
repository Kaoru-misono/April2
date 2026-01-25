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
        Begin,
        End
    };

    /**
     * Cache-optimized profiler event structure.
     * Members:
     * - uint64_t timestamp (8 bytes)
     * - const char* name (8 bytes)
     * - uint32_t threadId (4 bytes)
     * - ProfileEventType type (1 byte)
     * - padding (11 bytes)
     * Total: 32 bytes
     */
    struct alignas(32) ProfileEvent
    {
        uint64_t timestamp;
        const char* name;
        uint32_t threadId;
        ProfileEventType type;
        uint8_t padding[11]; 
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
         * @return Pointer to the recorded event, or nullptr if buffer is full.
         */
        auto record(const char* name, ProfileEventType type) -> ProfileEvent*;

        /**
         * Resets the buffer for the next frame.
         */
        auto reset() -> void { m_writeIndex.store(0, std::memory_order_relaxed); }

        auto getEvents() const -> const ProfileEvent* { return m_events.data(); }
        auto getCount() const -> size_t { return m_writeIndex.load(std::memory_order_relaxed); }

    private:
        std::vector<ProfileEvent> m_events;
        std::atomic<size_t> m_writeIndex{0};
    };
}