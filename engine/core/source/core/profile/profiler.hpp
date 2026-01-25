#pragma once

#include "profile-types.hpp"
#include <string>
#include <source_location>
#include <string_view>

namespace april::core
{
    /**
     * Interface for GPU profiling data providers.
     * This allows the graphics module to inject GPU events into the core profiler.
     */
    class IGpuProfiler
    {
    public:
        virtual ~IGpuProfiler() = default;

        /**
         * Collects all ready GPU events.
         * @return Vector of GPU-originated profiler events.
         */
        virtual auto collectEvents() -> std::vector<ProfileEvent> = 0;
    };

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

        /**
         * Registers a GPU profiler provider.
         */
        auto registerGpuProfiler(IGpuProfiler* pGpuProfiler) -> void { m_gpuProfiler = pGpuProfiler; }

        /**
         * Returns the registered GPU profiler.
         */
        auto getGpuProfiler() const -> IGpuProfiler* { return m_gpuProfiler; }

    private:
        Profiler() = default;
        ~Profiler() = default;
        Profiler(const Profiler&) = delete;
        Profiler& operator=(const Profiler&) = delete;

        IGpuProfiler* m_gpuProfiler{nullptr};
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