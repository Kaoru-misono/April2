#include <doctest/doctest.h>
#include <core/profile/timer.hpp>
#include <core/profile/profiler.hpp>
#include <core/profile/profile-manager.hpp>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>

using namespace april::core;

TEST_SUITE("ProfilerCollection")
{
    TEST_CASE("Multi-threaded Event Collection")
    {
        // Ensure clean state
        ProfileManager::get().flush();

        std::atomic<bool> start{false};
        std::atomic<bool> finish{false};
        auto threadFunc = [&](const char* name) {
            while (!start) std::this_thread::yield();
            {
                APRIL_PROFILE_ZONE(name);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            // Keep thread alive until manager flushes
            while (!finish) std::this_thread::yield();
        };

        std::thread t1(threadFunc, "Thread1");
        std::thread t2(threadFunc, "Thread2");

        start = true;
        
        // Wait a bit for threads to record
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        auto events = ProfileManager::get().flush();
        
        finish = true;
        t1.join();
        t2.join();

        // Each thread creates 2 events (Begin + End)
        CHECK(events.size() == 4);

        bool foundT1Begin = false, foundT1End = false;
        bool foundT2Begin = false, foundT2End = false;

        for (const auto& e : events)
        {
            if (e.name && std::string(e.name) == "Thread1")
            {
                if (e.type == ProfileEventType::Begin) foundT1Begin = true;
                if (e.type == ProfileEventType::End) foundT1End = true;
            }
            if (e.name && std::string(e.name) == "Thread2")
            {
                if (e.type == ProfileEventType::Begin) foundT2Begin = true;
                if (e.type == ProfileEventType::End) foundT2End = true;
            }
        }

        CHECK(foundT1Begin);
        CHECK(foundT1End);
        CHECK(foundT2Begin);
        CHECK(foundT2End);

        // Verify temporal ordering (sorting by timestamp)
        if (!events.empty())
        {
            for (size_t i = 1; i < events.size(); ++i)
            {
                CHECK(events[i].timestamp >= events[i-1].timestamp);
            }
        }
    }

    TEST_CASE("Performance Benchmark")
    {
        const int kIterations = 100000;
        
        auto start = Timer::now();
        for (int i = 0; i < kIterations; ++i)
        {
            APRIL_PROFILE_ZONE("Benchmark");
        }
        auto end = Timer::now();

        double totalMs = Timer::calcDuration(start, end);
        double avgNs = (totalMs * 1000000.0) / (kIterations * 2.0); // 2 events per iteration

        MESSAGE("Average overhead per event: ", avgNs, " ns");
        
        // Overhead should be very low (typically < 100ns on modern hardware)
        CHECK(avgNs < 500.0); // Loose check for CI/environment variability

        // Cleanup
        ProfileManager::get().flush();
    }
}

TEST_SUITE("ProfilerFoundation")
{
    TEST_CASE("Timer Basic Functionality")
    {
        auto start = Timer::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        auto end = Timer::now();

        double durationMs = Timer::calcDuration(start, end);

        // Sleep is not precise, but it should be at least 10ms
        CHECK(durationMs >= 10.0);
        CHECK(durationMs < 100.0); // Should definitely be less than 100ms
    }

    TEST_CASE("Timer Conversion Methods")
    {
        auto start = Timer::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        auto end = Timer::now();

        auto durationNs = Timer::calcDurationNanoseconds(start, end);
        auto durationUs = Timer::calcDurationMicroseconds(start, end);
        auto durationMs = Timer::calcDuration(start, end);

        CHECK(durationNs >= 5000000);
        CHECK(durationUs >= 5000);
        // Consistency check
        // We use double comparison for all to avoid integer truncation issues in the check itself
        auto durationNsDouble = static_cast<double>(durationNs);
        auto durationUsDouble = static_cast<double>(durationUs);

        CHECK(durationMs >= 5.0);
        CHECK(doctest::Approx(durationMs * 1000.0).epsilon(0.01) == durationUsDouble);
        CHECK(doctest::Approx(durationUsDouble * 1000.0).epsilon(0.01) == durationNsDouble);
    }

    TEST_CASE("ScopedProfileZone and Macro")
    {
        // For now, this is a no-op in terms of data recording,
        // but we verify it compiles and runs without crashing.
        {
            APRIL_PROFILE_ZONE("TestZone");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        {
            APRIL_PROFILE_ZONE(); // Automatic name
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}