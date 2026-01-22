#include <doctest/doctest.h>
#include <graphics/program/program.hpp>
#include <graphics/program/program-variables.hpp>
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/texture.hpp>
#include <graphics/rhi/buffer.hpp>
#include <graphics/rhi/sampler.hpp>

using namespace april::graphics;

TEST_SUITE("ResourceBinding")
{
    TEST_CASE("Variable Binding")
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
        progDesc.addShaderModule("BindVS").addString(shaderSource, "BindVS.slang");
        progDesc.vsEntryPoint("main");

        auto program = Program::create(device, progDesc);
        REQUIRE(program);

        auto vars = ProgramVariables::create(device, program.get());
        REQUIRE(vars);

        auto root = vars->getRootVariable();
        REQUIRE(root.isValid());

        // Test Uniform Binding
        root["gCB"]["a"] = 1.5f;
        root["gCB"]["b"] = 42;

        // Test Texture Binding
        auto texture = device->createTexture2D(128, 128, ResourceFormat::RGBA8Unorm, 1, 1, nullptr, TextureUsage::ShaderResource);
        REQUIRE(texture);
        root["gTex"] = texture;

        // Test Sampler Binding
        auto sampler = device->createSampler(Sampler::Desc());
        REQUIRE(sampler);
        root["gSampler"] = sampler;

        // Verify retrieval
        CHECK(root["gTex"].getTexture() == texture);
        CHECK(root["gSampler"].getSampler() == sampler);
    }
}
