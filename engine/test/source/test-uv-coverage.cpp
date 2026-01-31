#include <doctest/doctest.h>
#include <graphics/program/program.hpp>
#include <graphics/program/program-variables.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/render-target.hpp>
#include <graphics/rhi/graphics-pipeline.hpp>
#include <graphics/rhi/rasterizer-state.hpp>

using namespace april;
using namespace april::graphics;

namespace
{
    void unpackRgba(uint32_t value, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a)
    {
        r = static_cast<uint8_t>(value & 0xFFu);
        g = static_cast<uint8_t>((value >> 8) & 0xFFu);
        b = static_cast<uint8_t>((value >> 16) & 0xFFu);
        a = static_cast<uint8_t>((value >> 24) & 0xFFu);
    }
}

TEST_SUITE("Full Screen Triangle")
{
    TEST_CASE("UV Coverage")
    {
        for (auto deviceType : { Device::Type::D3D12, Device::Type::Vulkan })
        {
            Device::Desc desc;
            desc.type = deviceType;
            auto device = april::core::make_ref<Device>(desc);
            if (!device) continue;

            auto ctx = device->getCommandContext();
            REQUIRE(ctx);

            char const* shaderSource = R"(
                struct FullScreenVertex
                {
                    float4 position : SV_Position;
                    float2 uv       : TEXCOORD0;
                };

                FullScreenVertex makeFullScreenTriangle(uint vertexID)
                {
                    FullScreenVertex output;
                    float2 uvRaw = float2((vertexID << 1) & 2, vertexID & 2);
                    output.uv = uvRaw * 0.5f;
                    output.position = float4(uvRaw * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
                    return output;
                }

                struct VertexOutput
                {
                    float4 position : SV_Position;
                    float2 uv       : TEXCOORD0;
                };

                [shader("vertex")]
                VertexOutput vsMain(uint vertexID : SV_VertexID)
                {
                    FullScreenVertex fs = makeFullScreenTriangle(vertexID);
                    VertexOutput output;
                    output.position = fs.position;
                    output.uv = fs.uv;
                    return output;
                }

                [shader("fragment")]
                float4 psMain(VertexOutput input) : SV_Target
                {
                    return float4(input.uv, 0.0f, 1.0f);
                }
            )";

            ProgramDesc progDesc;
            progDesc.addShaderModule("UvCoverage").addString(shaderSource, "UvCoverage.slang");
            progDesc.vsEntryPoint("vsMain");
            progDesc.psEntryPoint("psMain");

            auto program = Program::create(device, progDesc);
            REQUIRE(program);
            auto vars = ProgramVariables::create(device, program.get());
            REQUIRE(vars);

            GraphicsPipelineDesc pipeDesc;
            pipeDesc.programKernels = program->getActiveVersion()->getKernels(device.get(), nullptr);
            pipeDesc.renderTargetCount = 1;
            pipeDesc.renderTargetFormats[0] = ResourceFormat::RGBA8Unorm;

            RasterizerState::Desc rsDesc;
            rsDesc.setCullMode(RasterizerState::CullMode::None);
            pipeDesc.rasterizerState = RasterizerState::create(rsDesc);

            auto pipeline = device->createGraphicsPipeline(pipeDesc);
            REQUIRE(pipeline);

            const uint32_t width = 4;
            const uint32_t height = 4;
            auto target = device->createTexture2D(width, height, ResourceFormat::RGBA8Unorm, 1, 1, nullptr, TextureUsage::RenderTarget | TextureUsage::ShaderResource);
            REQUIRE(target);

            ctx->resourceBarrier(target.get(), Resource::State::RenderTarget);
            auto encoder = ctx->beginRenderPass({ColorTarget(target->getRTV(), LoadOp::Clear, StoreOp::Store, float4(0, 0, 0, 1))});
            REQUIRE(encoder);

            Viewport vp = Viewport::fromSize((float)width, (float)height);
            encoder->setViewport(0, vp);
            encoder->setScissor(0, {0, 0, width, height});
            encoder->bindPipeline(pipeline.get(), vars.get());
            encoder->draw(3, 0);
            encoder->end();
            ctx->submit(true);

            auto readbackBytes = ctx->readTextureSubresource(target.get(), 0);
            ctx->submit(true);

            REQUIRE(readbackBytes.size() == width * height * 4);
            uint32_t const* pixels = reinterpret_cast<uint32_t const*>(readbackBytes.data());

            auto left = pixels[0];
            auto right = pixels[width - 1];
            auto top = pixels[0];
            auto bottom = pixels[(height - 1) * width];

            uint8_t lr, lg, lb, la;
            uint8_t rr, rg, rb, ra;
            uint8_t tr, tg, tb, ta;
            uint8_t br, bg, bb, ba;
            unpackRgba(left, lr, lg, lb, la);
            unpackRgba(right, rr, rg, rb, ra);
            unpackRgba(top, tr, tg, tb, ta);
            unpackRgba(bottom, br, bg, bb, ba);

            CHECK(rr > lr);
            CHECK(bg > tg);
            CHECK(la == 255);
            CHECK(ra == 255);
        }
    }
}
