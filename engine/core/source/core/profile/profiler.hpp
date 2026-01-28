#pragma once

#include "profile-types.hpp"
#include "timer.hpp"
#include <chrono>
#include <source_location>

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
         * Registers a GPU profiler provider.
         */
        auto registerGpuProfiler(IGpuProfiler* pGpuProfiler) -> void { m_gpuProfiler = pGpuProfiler; }

        /**
         * Records a complete event with explicit timing.
         */
        auto recordEvent(char const* name, double startUs, double durationUs, ProfileEventType type = ProfileEventType::Complete) -> void;

        /**
         * Returns the registered GPU profiler.
         */
        auto getGpuProfiler() const -> IGpuProfiler* { return m_gpuProfiler; }

    private:
        Profiler() = default;
        ~Profiler() = default;
        Profiler(Profiler const&) = delete;
        Profiler& operator=(Profiler const&) = delete;

        IGpuProfiler* m_gpuProfiler{nullptr};
    };

    /**
     * RAII-style helper for profiling code blocks.
     */
    class ScopedProfileZone
    {
    public:
        ScopedProfileZone(char const* name) : m_name(name), m_start(Timer::now()) {}

        ~ScopedProfileZone()
        {
            auto const end = Timer::now();
            auto const startUs = std::chrono::duration<double, std::micro>(m_start.time_since_epoch()).count();
            auto durationUs = std::chrono::duration<double, std::micro>(end - m_start).count();
            if (durationUs < 0.001)
            {
                durationUs = 0.001;
            }
            Profiler::get().recordEvent(m_name, startUs, durationUs, ProfileEventType::Complete);
        }

    private:
        char const* m_name;
        Timer::TimePoint m_start;
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
