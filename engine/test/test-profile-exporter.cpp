#include <doctest/doctest.h>
#include <core/profile/profile-exporter.hpp>
#include <core/profile/profile-manager.hpp>
#include <fstream>
#include <string>
#include <vector>

using namespace april::core;

TEST_CASE("ProfileExporter JSON Formatting")
{
    // Setup test data
    std::vector<ProfileEvent> events;
    
    // Test Begin event: 1000000 ns = 1000 us
    events.push_back({
        .timestamp = 1000000,
        .name = "TestBegin",
        .threadId = 1,
        .type = ProfileEventType::Begin
    });
    
    // Test End event: 2000000 ns = 2000 us
    events.push_back({
        .timestamp = 2000000,
        .name = "TestEnd",
        .threadId = 1,
        .type = ProfileEventType::End
    });

    // Test GPU event: 0xFFFFFFFF
    events.push_back({
        .timestamp = 1500000,
        .name = "GpuTask",
        .threadId = 0xFFFFFFFF,
        .type = ProfileEventType::Begin
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
        // Check Begin event mapping and timestamp scaling
        CHECK(content.find("\"name\": \"TestBegin\"") != std::string::npos);
        CHECK(content.find("\"ph\": \"B\"") != std::string::npos);
        CHECK(content.find("\"ts\": 1000") != std::string::npos); // 1000000 ns / 1000
        CHECK(content.find("\"tid\": 1") != std::string::npos);

        // Check End event mapping
        CHECK(content.find("\"name\": \"TestEnd\"") != std::string::npos);
        CHECK(content.find("\"ph\": \"E\"") != std::string::npos);
        CHECK(content.find("\"ts\": 2000") != std::string::npos);
    }
}