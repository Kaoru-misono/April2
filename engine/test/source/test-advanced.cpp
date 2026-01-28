#include <doctest/doctest.h>
#include <graphics/program/program.hpp>
#include <graphics/program/program-manager.hpp>
#include <graphics/rhi/render-device.hpp>

using namespace april::graphics;

TEST_SUITE("Advanced")
{
    TEST_CASE("Ray Tracing Hit Group Reflection")
    {
        // Setup Device
        Device::Desc deviceDesc;
        deviceDesc.type = Device::Type::Default;
        auto device = april::core::make_ref<Device>(deviceDesc);
        REQUIRE(device);

        // Ray Tracing Shaders
        char const* shaderSource = R"(
            struct RayPayload { float4 color; };
            struct BuiltInTriangleIntersectionAttributes { float2 hitCoords; };

            RaytracingAccelerationStructure gScene;

            [shader("raygen")]
            void rayGen() {
                RayPayload payload;
                RayDesc ray;
                ray.Origin = float3(0,0,0);
                ray.Direction = float3(0,0,1);
                ray.TMin = 0.0;
                ray.TMax = 1000.0;
                TraceRay(gScene, 0, 0xFF, 0, 0, 0, ray, payload);
            }

            [shader("miss")]
            void miss(inout RayPayload payload) {
                payload.color = float4(0, 0, 0, 1);
            }

            [shader("closesthit")]
            void closestHit(inout RayPayload payload, BuiltInTriangleIntersectionAttributes attr) {
                payload.color = float4(1, 1, 1, 1);
            }

            [shader("anyhit")]
            void anyHit(inout RayPayload payload, BuiltInTriangleIntersectionAttributes attr) {
                IgnoreHit();
            }
        )";

        ProgramDesc progDesc;
        progDesc.addShaderModule("RT").addString(shaderSource, "RT.slang");
        progDesc.addRayGen("rayGen");
        progDesc.addMiss("miss");
        progDesc.addHitGroup("closestHit", "anyHit");
        progDesc.setMaxTraceRecursionDepth(1);
        progDesc.setMaxPayloadSize(16);

        auto program = Program::create(device, progDesc);
        REQUIRE(program);

        auto version = program->getActiveVersion();
        REQUIRE(version);

        auto reflector = version->getReflector();
        REQUIRE(reflector);

        // Verify group count
        CHECK(program->getEntryPointGroupCount() == 3); // RayGen group, Miss group, Hit group
    }

    TEST_CASE("Global Manager State Propagation")
    {
        // Setup Device
        Device::Desc deviceDesc;
        deviceDesc.type = Device::Type::Default;
        auto device = april::core::make_ref<Device>(deviceDesc);
        REQUIRE(device);

        auto progManager = device->getProgramManager();

        char const* shaderSource = R"(
            struct VSOut { float4 pos : SV_Position; };
            VSOut main() {
                VSOut output;
                #ifdef GLOBAL_DEBUG
                    output.pos = float4(1, 0, 0, 1);
                #else
                    output.pos = float4(0, 1, 0, 1);
                #endif
                return output;
            }
        )";

        ProgramDesc progDesc;
        progDesc.addShaderModule("GlobalVS").addString(shaderSource, "GlobalVS.slang");
        progDesc.vsEntryPoint("main");

        auto program = Program::create(device, progDesc);
        auto version1 = program->getActiveVersion();
        REQUIRE(version1);

        // Add Global Define
        progManager->addGlobalDefines({{"GLOBAL_DEBUG", "1"}});

        // The program should have been reset and re-linked
        auto version2 = program->getActiveVersion();
        REQUIRE(version2);
        CHECK(version1 != version2);

        // Remove Global Define
        progManager->removeGlobalDefines({{"GLOBAL_DEBUG", "1"}});
        auto version3 = program->getActiveVersion();
        REQUIRE(version3);
        CHECK(version2 != version3);
    }

    TEST_CASE("Deeply Nested Parameter Blocks")
    {
        // Setup Device
        Device::Desc deviceDesc;
        deviceDesc.type = Device::Type::Default;
        auto device = april::core::make_ref<Device>(deviceDesc);
        REQUIRE(device);

        char const* shaderSource = R"(
            struct Inner {
                Texture2D tex;
                float4 color;
            };

            struct Outer {
                Inner inner;
                SamplerState samp;
            };

            ParameterBlock<Outer> gData;

            struct VSOut { float4 pos : SV_Position; };
            VSOut main() {
                VSOut output;
                output.pos = gData.inner.tex.Sample(gData.samp, float2(0,0)) * gData.inner.color;
                return output;
            }
        )";

        ProgramDesc progDesc;
        progDesc.addShaderModule("Nested").addString(shaderSource, "Nested.slang");
        progDesc.vsEntryPoint("main");

        auto program = Program::create(device, progDesc);
        auto reflector = program->getReflector();
        REQUIRE(reflector);

        // Find global block
        auto pBlock = reflector->getParameterBlock("gData");
        REQUIRE(pBlock);

        // Verify nested structure
        auto pOuterType = pBlock->getElementType()->asStructType();
        REQUIRE(pOuterType);

        auto pInnerVar = pOuterType->getMember("inner");
        REQUIRE(pInnerVar);

        auto pInnerType = pInnerVar->getType()->asStructType();
        REQUIRE(pInnerType);

        auto pTexVar = pInnerType->getMember("tex");
        REQUIRE(pTexVar);
        CHECK(pTexVar->getType()->asResourceType()->getType() == ReflectionResourceType::Type::Texture);
    }

    TEST_CASE("Multiple Interface Implementations")
    {
        // Setup Device
        Device::Desc deviceDesc;
        deviceDesc.type = Device::Type::Default;
        auto device = april::core::make_ref<Device>(deviceDesc);
        REQUIRE(device);

        char const* shaderSource = R"(
            interface ILight { float3 getIntensity(); };

            struct PointLight : ILight { float3 getIntensity() { return float3(1,0,0); } };
            struct DirectionalLight : ILight { float3 getIntensity() { return float3(0,1,0); } };

            ParameterBlock<ILight> gLight;

            struct VSOut { float4 pos : SV_Position; };
            VSOut main() {
                VSOut output;
                output.pos = float4(gLight.getIntensity(), 1.0);
                return output;
            }
        )";

        ProgramDesc progDesc;
        progDesc.addShaderModule("Interface").addString(shaderSource, "Interface.slang");
        progDesc.vsEntryPoint("main");

        auto program = Program::create(device, progDesc);
        REQUIRE(program);

        // Specialize with PointLight
        program->addTypeConformance("PointLight", "ILight", 0);
        auto version1 = program->getActiveVersion();
        REQUIRE(version1);

        // Specialize with DirectionalLight
        program->removeTypeConformance("PointLight", "ILight");
        program->addTypeConformance("DirectionalLight", "ILight", 1);
        auto version2 = program->getActiveVersion();
        REQUIRE(version2);

        CHECK(version1 != version2);
    }
}
