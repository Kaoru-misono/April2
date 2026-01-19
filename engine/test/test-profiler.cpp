#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <core/profile/profiler.hpp>
#include <core/profile/timers.hpp>
#include <thread>
#include <chrono>

using namespace april::core;

TEST_CASE("PerformanceTimer") {
    PerformanceTimer timer;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    CHECK(timer.getMilliseconds() >= 10.0);
    timer.reset();
    CHECK(timer.getMilliseconds() < 1.0);
}

TEST_CASE("ProfilerManager and Timeline") {
    ProfilerManager manager;
    ProfilerTimeline::CreateInfo info;
    info.name = "TestTimeline";
    ProfilerTimeline* timeline = manager.createTimeline(info);

    CHECK(timeline != nullptr);
    CHECK(timeline->getName() == "TestTimeline");

    // Frame scope
    {
        timeline->frameAdvance(); // Start frame 1
        {
            auto section = timeline->frameSection("TestSection");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        timeline->frameAdvance(); // End frame 1, Start frame 2
    }

    // Async scope
    {
        auto section = timeline->asyncSection("AsyncSection");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Snapshots
    std::vector<ProfilerTimeline::Snapshot> frameSnaps, asyncSnaps;
    manager.getSnapshots(frameSnaps, asyncSnaps);

    CHECK(frameSnaps.size() == 1);
    CHECK(asyncSnaps.size() == 1);
    CHECK(frameSnaps[0].name == "TestTimeline");

    // Clean up
    manager.destroyTimeline(timeline);
}

TEST_CASE("GlobalProfiler and Macros") {
    GlobalProfiler::init("MainThread");
    CHECK(GlobalProfiler::getManager() != nullptr);
    CHECK(GlobalProfiler::getTimeline() != nullptr);

    {
        AP_PROFILE_SCOPE("GlobalScope");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    GlobalProfiler::shutdown();
    CHECK(GlobalProfiler::getManager() == nullptr);
}
