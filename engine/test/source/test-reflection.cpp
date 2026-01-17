#include <doctest/doctest.h>
#include <graphics/program/program.hpp>
#include <graphics/program/program-reflection.hpp>
#include <graphics/rhi/render-device.hpp>

using namespace april::graphics;

TEST_SUITE("Reflection")
{
    TEST_CASE("Basic Reflection")
    {
        Device::Desc deviceDesc;
        deviceDesc.type = Device::Type::Default;
        auto device = april::core::make_ref<Device>(deviceDesc);
        REQUIRE(device);

        const char* shaderSource = R"(
            struct MyStruct {
                float a;
                int b;
            };
            ConstantBuffer<MyStruct> gCB;
            Texture2D gTex;
            SamplerState gSampler;

            struct VSOut {
                float4 pos : SV_Position;
            };
            VSOut main(uint vertexId : SV_VertexID) {
                VSOut output;
                output.pos = float4(gCB.a, float(gCB.b), 0.0, 1.0);
                return output;
            }
        )";

        ProgramDesc progDesc;
        progDesc.addShaderModule("ReflectVS").addString(shaderSource, "ReflectVS.slang");
        progDesc.vsEntryPoint("main");

        auto program = Program::create(device, progDesc);
        REQUIRE(program);

        auto reflector = program->getReflector();
        REQUIRE(reflector);

        // Test Type Lookup
        auto myStructType = reflector->findType("MyStruct");
        REQUIRE(myStructType);
        CHECK(myStructType->getKind() == ReflectionType::Kind::Struct);

        auto structType = myStructType->asStructType();
        REQUIRE(structType);
        CHECK(structType->getMemberCount() == 2);
        CHECK(structType->getMember("a") != nullptr);
        CHECK(structType->getMember("b") != nullptr);

        // Test Resource Reflection
        auto texVar = reflector->getResource("gTex");
        REQUIRE(texVar);
        CHECK(texVar->getType()->getKind() == ReflectionType::Kind::Resource);
        CHECK(texVar->getType()->asResourceType()->getType() == ReflectionResourceType::Type::Texture);

        auto samplerVar = reflector->getResource("gSampler");
        REQUIRE(samplerVar);
        CHECK(samplerVar->getType()->asResourceType()->getType() == ReflectionResourceType::Type::Sampler);

        auto cbVar = reflector->getResource("gCB");
        REQUIRE(cbVar);
        CHECK(cbVar->getType()->asResourceType()->getType() == ReflectionResourceType::Type::ConstantBuffer);
    }

    TEST_CASE("Compute Reflection")
    {
        Device::Desc deviceDesc;
        deviceDesc.type = Device::Type::Default;
        auto device = april::core::make_ref<Device>(deviceDesc);
        REQUIRE(device);

        const char* shaderSource = R"(
            [numthreads(16, 8, 1)]
            void main(uint3 threadId : SV_DispatchThreadID) {
            }
        )";

        ProgramDesc progDesc;
        progDesc.addShaderModule("ReflectCS").addString(shaderSource, "ReflectCS.slang");
        progDesc.csEntryPoint("main");

        auto program = Program::create(device, progDesc);
        REQUIRE(program);

        auto reflector = program->getReflector();
        REQUIRE(reflector);

        auto threadGroupSize = reflector->getThreadGroupSize();
        CHECK(threadGroupSize.x == 16);
        CHECK(threadGroupSize.y == 8);
        CHECK(threadGroupSize.z == 1);
    }
}
