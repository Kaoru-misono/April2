#include <doctest/doctest.h>
#include <core/profile/profile-exporter.hpp>

using namespace april::core;

TEST_CASE("ProfileExporter Skeleton")
{
    // Just verify we can call the function (even if it does nothing)
    // This ensures linkage and basic API existence.
    std::vector<ProfileEvent> events;
    ProfileExporter::exportToFile("test_skeleton.json", events);
    
    // If we reached here, it didn't crash and linked correctly.
    CHECK(true);
}
