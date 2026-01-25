#pragma once

#include "profile-types.hpp"
#include <string>
#include <source_location>
#include <string_view>

namespace april::core
{
    /**
     * Foundational Profiler class.
     * Manages CPU and (future) GPU profiling events.
     */
    class Profiler
    {
    public:
        static auto get() -> Profiler&;

        /**
         * Marks the beginning of a profiling event.
         * @param name The name of the event.
         */
        auto startEvent(const char* name) -> void;

        /**
         * Marks the end of a profiling event.
         * @param name The name of the event.
         */
        auto endEvent(const char* name) -> void;

    private:
        Profiler() = default;
        ~Profiler() = default;
        Profiler(const Profiler&) = delete;
        Profiler& operator=(const Profiler&) = delete;
    };

    /**
     * RAII-style helper for profiling code blocks.
     */
    class ScopedProfileZone
    {
    public:
        ScopedProfileZone(const char* name) : m_name(name)
        {
            Profiler::get().startEvent(m_name);
        }

        ~ScopedProfileZone()
        {
            Profiler::get().endEvent(m_name);
        }

    private:
        const char* m_name;
    };
}

#define APRIL_PROFILE_ZONE_CONCAT_INNER(a, b) a##b
#define APRIL_PROFILE_ZONE_CONCAT(a, b) APRIL_PROFILE_ZONE_CONCAT_INNER(a, b)

/**
 * APRIL_PROFILE_ZONE() captures the current function name.
 * APRIL_PROFILE_ZONE("CustomName") uses the provided custom name.
 */
#define APRIL_PROFILE_ZONE(...) \
    april::core::ScopedProfileZone APRIL_PROFILE_ZONE_CONCAT(april_unique_zone_, __LINE__)( \
        [&]() { \
            if constexpr (std::string_view(#__VA_ARGS__).empty()) return std::source_location::current().function_name(); \
            else return __VA_ARGS__; \
        }() \
    )