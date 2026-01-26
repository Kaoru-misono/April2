#pragma once

#include "profile-types.hpp"
#include <vector>
#include <mutex>
#include <memory>
#include <map>
#include <string>

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
         * Registers a name for a specific thread ID.
         */
        auto registerThreadName(uint32_t tid, std::string const& name) -> void;

        /**
         * Retrieves the map of registered thread names.
         */
        auto getThreadNames() const -> std::map<uint32_t, std::string> const&;

        /**
         * Collects and clears data from all registered buffers.
         * @return Vector of all collected events.
         */
        auto flush() -> std::vector<ProfileEvent>;

    private:
        ProfileManager();
        ~ProfileManager() = default;

        std::vector<ProfileBuffer*> m_buffers;
        std::map<uint32_t, std::string> m_threadNames;
        std::mutex m_mutex;
    };
}