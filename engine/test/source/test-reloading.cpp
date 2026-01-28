#include <doctest/doctest.h>
#include <graphics/program/program.hpp>
#include <graphics/program/program-manager.hpp>
#include <graphics/rhi/render-device.hpp>
#include <fstream>
#include <thread>
#include <chrono>

using namespace april::graphics;

TEST_SUITE("Reloading")
{
    TEST_CASE("Manual Reload")
    {
        // Setup Device
        Device::Desc deviceDesc;
        deviceDesc.type = Device::Type::Default;
        auto device = april::core::make_ref<Device>(deviceDesc);
        REQUIRE(device);

        auto progManager = device->getProgramManager();
        REQUIRE(progManager);

        // Simple Shader
        char const* shaderSource = R"(
            struct VSOut {
                float4 pos : SV_Position;
            };
            VSOut main(uint vertexId : SV_VertexID) {
                VSOut output;
                output.pos = float4(0.0, 0.0, 0.0, 1.0);
                return output;
            }
        )";

        // Create Program
        ProgramDesc progDesc;
        progDesc.addShaderModule("ReloadVS").addString(shaderSource, "ReloadVS.slang");
        progDesc.vsEntryPoint("main");

        auto program = Program::create(device, progDesc);
        REQUIRE(program);

        auto version1 = program->getActiveVersion();
        REQUIRE(version1);

        // Manually trigger reload
        progManager->reloadAllPrograms(true);

        auto version2 = program->getActiveVersion();
        REQUIRE(version2);

        // Since we did force reload, it should have cleared and re-linked, resulting in a new version object
        CHECK(version1 != version2);
    }

    TEST_CASE("Hot Reloading from File")
    {
        // Setup Device
        Device::Desc deviceDesc;
        deviceDesc.type = Device::Type::Default;
        auto device = april::core::make_ref<Device>(deviceDesc);
        REQUIRE(device);

        auto progManager = device->getProgramManager();
        REQUIRE(progManager);

        // Create temporary shader file
        std::filesystem::path shaderPath = std::filesystem::absolute("test-reload.slang");
        {
            std::ofstream ofs(shaderPath);
            ofs << R"(
                struct VSOut {
                    float4 pos : SV_Position;
                };
                VSOut main(uint vertexId : SV_VertexID) {
                    VSOut output;
                    output.pos = float4(0.0, 0.0, 0.0, 1.0);
                    return output;
                }
            )";
        }

        // Create Program
        ProgramDesc progDesc;
        progDesc.addShaderLibrary(shaderPath);
        progDesc.vsEntryPoint("main");

        auto program = Program::create(device, progDesc);
        REQUIRE(program);

        auto version1 = program->getActiveVersion();
        REQUIRE(version1);

        // Wait a bit to ensure timestamp difference
        std::this_thread::sleep_for(std::chrono::milliseconds(1100));

        // Modify shader file
        {
            std::ofstream ofs(shaderPath);
            ofs << R"(
                struct VSOut {
                    float4 pos : SV_Position;
                };
                VSOut main(uint vertexId : SV_VertexID) {
                    VSOut output;
                    output.pos = float4(1.0, 1.0, 1.0, 1.0); // Changed
                    return output;
                }
            )";
        }

        // Trigger reload (automatic detection)
        bool reloaded = progManager->reloadAllPrograms(false);

        // This is expected to FAIL because timestamp logic is commented out in ProgramManager.cpp
        CHECK(reloaded == true);

        auto version2 = program->getActiveVersion();
        REQUIRE(version2);

        CHECK(version1 != version2);

        // Clean up
        std::filesystem::remove(shaderPath);
    }
}
