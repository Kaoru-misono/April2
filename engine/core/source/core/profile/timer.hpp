#pragma once

#include <chrono>
#include <cstdint>

namespace april::core
{
    /**
     * High-precision timer utility using std::chrono.
     */
    class Timer
    {
    public:
        using Clock = std::chrono::high_resolution_clock;
        using TimePoint = Clock::time_point;

        Timer() = default; // TODO:
        ~Timer() = default; // TODO:

        /**
         * Returns the current time point.
         */
        static auto now() -> TimePoint;

        /**
         * Calculates duration between two time points in milliseconds.
         */
        static auto calcDuration(TimePoint start, TimePoint end) -> double;

        /**
         * Calculates duration between two time points in microseconds.
         */
        static auto calcDurationMicroseconds(TimePoint start, TimePoint end) -> int64_t;

        /**
         * Calculates duration between two time points in nanoseconds.
         */
        static auto calcDurationNanoseconds(TimePoint start, TimePoint end) -> int64_t;

    private:
        // TODO:
    };
}