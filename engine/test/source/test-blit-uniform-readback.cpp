#include <doctest/doctest.h>
#include <graphics/program/program.hpp>
#include <graphics/program/program-variables.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/buffer.hpp>
#include <slang-rhi/shader-cursor.h>

#include <cmath>

using namespace april;
using namespace april::graphics;

TEST_SUITE("Slang Uniform Readback")
{
    TEST_CASE("Blit uvTransform Readback")
    {
        for (auto deviceType : { Device::Type::D3D12, Device::Type::Vulkan })
        {
            Device::Desc deviceDesc;
            deviceDesc.type = deviceType;
            auto device = april::core::make_ref<Device>(deviceDesc);
            if (!device) continue;

            auto ctx = device->getCommandContext();
            REQUIRE(ctx);

            char const* shaderSource = R"(
                uniform float4 uvTransform;
                RWStructuredBuffer<float4> outData;
                [shader("compute")]
                [numthreads(1, 1, 1)]
                void main() {
                    outData[0] = uvTransform;
                }
            )";

            ProgramDesc progDesc;
            progDesc.addShaderModule("BlitUvReadback").addString(shaderSource, "BlitUvReadback.slang");
            progDesc.csEntryPoint("main");

            auto program = Program::create(device, progDesc);
            REQUIRE(program);

            auto vars = ProgramVariables::create(device, program.get());
            REQUIRE(vars);

            auto outputBuffer = device->createBuffer(sizeof(float4), BufferUsage::UnorderedAccess);
            REQUIRE(outputBuffer);

            vars->setBuffer("outData", outputBuffer);

            rhi::ShaderCursor cursor(vars->getShaderObject());
            auto uvCursor = cursor.getPath("uvTransform");
            REQUIRE(uvCursor.isValid());

            float4 expected = float4(0.25f, 0.5f, 0.75f, 1.0f);
            uvCursor.setData(expected);

            ComputePipelineDesc pipeDesc;
            pipeDesc.programKernels = program->getActiveVersion()->getKernels(device.get(), vars.get());
            auto pipeline = device->createComputePipeline(pipeDesc);
            REQUIRE(pipeline);

            auto encoder = ctx->beginComputePass();
            REQUIRE(encoder);
            encoder->bindPipeline(pipeline.get(), vars.get());
            encoder->dispatch(uint3(1, 1, 1));
            encoder->end();
            ctx->submit(true);

            float4 result{};
            ctx->readBuffer(outputBuffer.get(), &result, 0, sizeof(float4));
            ctx->submit(true);

            CHECK(std::abs(result.x - expected.x) < 1e-5f);
            CHECK(std::abs(result.y - expected.y) < 1e-5f);
            CHECK(std::abs(result.z - expected.z) < 1e-5f);
            CHECK(std::abs(result.w - expected.w) < 1e-5f);
        }
    }
}
