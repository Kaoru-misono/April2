#include <doctest/doctest.h>
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/profile/gpu-profiler.hpp>
#include <core/profile/profile-manager.hpp>
#include <core/profile/timer.hpp>
#include <core/foundation/object.hpp>
#include <thread>
#include <chrono>

using namespace april::core;
using namespace april::graphics;

TEST_SUITE("GpuProfiler")
{
    TEST_CASE("GPU Event Recording and Readback")
    {
        // 1. Setup Device
        Device::Desc desc;
        desc.enableDebugLayer = true;
        auto pDevice = april::core::make_ref<april::graphics::Device>(desc);
        auto* pCtx = pDevice->getCommandContext();

        // 2. Instrument GPU Zone
        {
            APRIL_GPU_ZONE(pCtx, "TestGpuPass");
        }

        // 3. Process 3 Frames to trigger N+2 readback
        for (int i = 0; i < 3; ++i)
        {
            pDevice->endFrame();
        }

        // 4. Flush and Verify
        auto events = ProfileManager::get().flush();
        
        bool foundGpuComplete = false;

        for (const auto& e : events)
        {
            if (e.name && std::string(e.name) == "TestGpuPass")
            {
                if (e.threadId == 0xFFFFFFFF)
                {
                    if (e.type == ProfileEventType::Complete) foundGpuComplete = true;
                }
            }
        }

        CHECK(foundGpuComplete);
    }

    TEST_CASE("GPU Event Timing Alignment")
    {
        // 1. Setup Device
        Device::Desc desc;
        desc.enableDebugLayer = true;
        auto pDevice = april::core::make_ref<april::graphics::Device>(desc);
        auto* pCtx = pDevice->getCommandContext();

        // 2. Instrument GPU Zone
        {
            APRIL_GPU_ZONE(pCtx, "AlignedZone");
        }

        // 3. Capture CPU time around the frame that contains the zone
        auto cpuStartNs = std::chrono::duration_cast<std::chrono::nanoseconds>(Timer::now().time_since_epoch()).count();
        pDevice->endFrame();
        auto cpuEndNs = std::chrono::duration_cast<std::chrono::nanoseconds>(Timer::now().time_since_epoch()).count();

        // 4. Process more frames to trigger readback
        for (int i = 0; i < 2; ++i) pDevice->endFrame();

        // 5. Flush and Verify
        auto events = ProfileManager::get().flush();
        
        bool found = false;
        for (const auto& e : events)
        {
            if (e.name && std::string(e.name) == "AlignedZone")
            {
                found = true;
                // According to our calibration, GPU start is aligned with the midpoint of submit call
                // So it should be reasonably within [cpuStart, cpuEnd]
                // Allow some tolerance for timer precision and rolling average warmup
                const double cpuStartUs = static_cast<double>(cpuStartNs) / 1000.0;
                CHECK(e.timestamp >= cpuStartUs - 50000.0); // 50ms tolerance for startup jitter
            }
        }
        CHECK(found);
    }
}
