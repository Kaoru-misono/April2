#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <graphics/profile/gpu-profiler.hpp>
#include <graphics/rhi/command-context.hpp>

using namespace april::graphics;

TEST_CASE("ScopedGpuProfileEvent Null Context") {
    // This should not crash
    {
        AP_PROFILE_GPU_SCOPE(nullptr, "NullScope");
    }
    CHECK(true);
}
