#include <doctest/doctest.h>
#include <core/profile/profile-exporter.hpp>
#include <core/profile/profile-manager.hpp>
#include <core/profile/profiler.hpp>
#include <fstream>
#include <string>
#include <vector>
#include <thread>

using namespace april::core;

TEST_CASE("ProfileExporter Integration")
{
    // Ensure clean state
    ProfileManager::get().flush();

    // Register current thread
    uint32_t tid = static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    ProfileManager::get().registerThreadName(tid, "Main");

    {
        APRIL_PROFILE_ZONE("IntegrationZone");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    auto events = ProfileManager::get().flush();
    REQUIRE(events.size() >= 1);

    const std::string filename = "test_profile.json";
    ProfileExporter::exportToFile(filename, events);

    std::ifstream ifs(filename);
    REQUIRE(ifs.is_open());
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    CHECK(content.find("\"name\": \"IntegrationZone\"") != std::string::npos);
    CHECK(content.find("\"name\": \"Main\"") != std::string::npos);
}

TEST_CASE("ProfileExporter JSON Formatting")
{
    // Setup test data
    std::vector<ProfileEvent> events;

    // Test Complete event: 1000 us start, 500 us duration
    events.push_back({
        .timestamp = 1000.0,
        .duration = 500.0,
        .name = "TestBegin",
        .threadId = 1,
        .type = ProfileEventType::Complete
    });

    events.push_back({
        .timestamp = 2000.0,
        .duration = 250.0,
        .name = "TestComplete2",
        .threadId = 1,
        .type = ProfileEventType::Complete
    });

    // Test GPU event: 0xFFFFFFFF
    events.push_back({
        .timestamp = 1500.0,
        .duration = 100.0,
        .name = "GpuTask",
        .threadId = 0xFFFFFFFF,
        .type = ProfileEventType::Complete
    });

    const std::string filename = "test_export.json";
    ProfileExporter::exportToFile(filename, events);

    std::ifstream ifs(filename);
    REQUIRE(ifs.is_open());
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    SUBCASE("Metadata events")
    {
        // Check for process_name
        CHECK(content.find("\"name\": \"process_name\"") != std::string::npos);
        CHECK(content.find("\"ph\": \"M\"") != std::string::npos);

        // Check for thread_name metadata for thread 1 (if registered)
        ProfileManager::get().registerThreadName(1, "MainThread");
        ProfileExporter::exportToFile(filename, events);

        std::ifstream ifs2(filename);
        std::string content2((std::istreambuf_iterator<char>(ifs2)), (std::istreambuf_iterator<char>()));

        CHECK(content2.find("\"name\": \"thread_name\"") != std::string::npos);
        CHECK(content2.find("\"tid\": 1") != std::string::npos);
        CHECK(content2.find("\"name\": \"MainThread\"") != std::string::npos);

        // Check for GPU Queue metadata
        CHECK(content2.find("\"tid\": 4294967295") != std::string::npos); // 0xFFFFFFFF
        CHECK(content2.find("\"name\": \"GPU Queue\"") != std::string::npos);
    }

    SUBCASE("Event data mapping")
    {
        // Check Complete event mapping and timestamp
        CHECK(content.find("\"name\": \"TestBegin\"") != std::string::npos);
        CHECK(content.find("\"ph\": \"X\"") != std::string::npos);
        CHECK(content.find("\"ts\": 1000") != std::string::npos);
        CHECK(content.find("\"dur\": 500") != std::string::npos);
        CHECK(content.find("\"tid\": 1") != std::string::npos);

        // Check second complete event mapping
        CHECK(content.find("\"name\": \"TestComplete2\"") != std::string::npos);
        CHECK(content.find("\"ts\": 2000") != std::string::npos);
    }
}
