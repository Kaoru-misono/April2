#include <doctest/doctest.h>
#include <core/profile/profile-manager.hpp>

using namespace april::core;

TEST_CASE("ProfileManager Thread Registry")
{
    auto& manager = ProfileManager::get();

    // Test registering a normal thread
    uint32_t tid = 12345;
    manager.registerThreadName(tid, "WorkerThread");

    // We need a way to verify this. 
    // Assuming a getThreadName method or getThreadNames
    auto const& names = manager.getThreadNames();
    
    CHECK(names.count(tid) == 1);
    CHECK(names.at(tid) == "WorkerThread");

    // Test GPU Queue pre-registration
    // 0xFFFFFFFF should be "GPU Queue"
    CHECK(names.count(0xFFFFFFFF) == 1);
    CHECK(names.at(0xFFFFFFFF) == "GPU Queue");
}
