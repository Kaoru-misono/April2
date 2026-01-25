#include <doctest/doctest.h>
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/profile/gpu-profiler.hpp>
#include <core/profile/profile-manager.hpp>
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
            // No need for clearTexture if we just want to test the recording
        }

        // 3. Process 3 Frames to trigger N+2 readback
        for (int i = 0; i < 3; ++i)
        {
            pDevice->endFrame();
        }

        // 4. Flush and Verify
        auto events = ProfileManager::get().flush();
        
        bool foundGpuBegin = false;
        bool foundGpuEnd = false;

        for (const auto& e : events)
        {
            if (e.name && std::string(e.name) == "TestGpuPass")
            {
                if (e.threadId == 0xFFFFFFFF)
                {
                    if (e.type == ProfileEventType::Begin) foundGpuBegin = true;
                    if (e.type == ProfileEventType::End) foundGpuEnd = true;
                }
            }
        }

        CHECK(foundGpuBegin);
        CHECK(foundGpuEnd);
    }
}