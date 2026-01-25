#pragma once

#include "profile-types.hpp"
#include <vector>
#include <mutex>
#include <memory>

namespace april::core
{
    /**
     * Singleton manager for coordinating data collection across all threads.
     */
    class ProfileManager
    {
    public:
        static auto get() -> ProfileManager&;

        /**
         * Registers a thread-local buffer with the manager.
         */
        auto registerBuffer(ProfileBuffer* pBuffer) -> void;

        /**
         * Unregisters a thread-local buffer.
         */
        auto unregisterBuffer(ProfileBuffer* pBuffer) -> void;

        /**
         * Collects and clears data from all registered buffers.
         * @return Vector of all collected events.
         */
        auto flush() -> std::vector<ProfileEvent>;

    private:
        ProfileManager() = default;
        ~ProfileManager() = default;

        std::vector<ProfileBuffer*> m_buffers;
        std::mutex m_mutex;
    };
}