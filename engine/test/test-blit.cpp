#include <doctest/doctest.h>
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/render-target.hpp>
#include <graphics/rhi/texture.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

using namespace april;
using namespace april::graphics;

namespace
{
    uint32_t packRgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        return (static_cast<uint32_t>(a) << 24) |
               (static_cast<uint32_t>(b) << 16) |
               (static_cast<uint32_t>(g) << 8)  |
               static_cast<uint32_t>(r);
    }

    void unpackRgba(uint32_t value, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a)
    {
        r = static_cast<uint8_t>(value & 0xFFu);
        g = static_cast<uint8_t>((value >> 8) & 0xFFu);
        b = static_cast<uint8_t>((value >> 16) & 0xFFu);
        a = static_cast<uint8_t>((value >> 24) & 0xFFu);
    }

    void blitToTexture(
        CommandContext* ctx,
        core::ref<TextureView> const& src,
        core::ref<TextureView> const& dst,
        uint4 srcRect,
        uint4 dstRect,
        TextureFilteringMode filter,
        float4 clearColor = float4(0, 0, 0, 1)
    )
    {
        auto dstTexture = dst->getResource()->asTexture();
        ctx->resourceBarrier(dstTexture.get(), Resource::State::RenderTarget);

        ColorTarget colorTarget(dst, LoadOp::Clear, StoreOp::Store, clearColor);
        auto encoder = ctx->beginRenderPass({colorTarget});
        REQUIRE(encoder);
        encoder->blit(src, dst, srcRect, dstRect, filter);
        encoder->end();
        ctx->submit(true);
    }

    uint8_t toByte(float v)
    {
        int value = static_cast<int>(std::lround(v * 255.0f));
        value = std::clamp(value, 0, 255);
        return static_cast<uint8_t>(value);
    }

    std::vector<uint32_t> makeUvGradient(uint32_t width, uint32_t height)
    {
        std::vector<uint32_t> data(width * height);
        for (uint32_t y = 0; y < height; ++y)
        {
            for (uint32_t x = 0; x < width; ++x)
            {
                float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(width);
                float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(height);
                data[y * width + x] = packRgba(toByte(u), toByte(v), 0, 255);
            }
        }
        return data;
    }

    void checkUvGradientPixel(
        uint32_t const* pixels,
        uint32_t width,
        uint32_t height,
        uint32_t x,
        uint32_t y,
        int tolerance = 2
    )
    {
        uint32_t pixel = pixels[y * width + x];
        uint8_t r, g, b, a;
        unpackRgba(pixel, r, g, b, a);

        float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(width);
        float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(height);
        uint8_t expectedR = toByte(u);
        uint8_t expectedG = toByte(v);

        CHECK(std::abs(int(r) - int(expectedR)) <= tolerance);
        CHECK(std::abs(int(g) - int(expectedG)) <= tolerance);
        CHECK(b == 0);
        CHECK(a == 255);
    }
}

TEST_CASE("Point Blit Full Copy")
{
    for (auto deviceType : { Device::Type::D3D12, Device::Type::Vulkan })
    {
        Device::Desc desc;
        desc.type = deviceType;
        auto device = april::core::make_ref<Device>(desc);
        if (!device) continue;

        auto ctx = device->getCommandContext();
        REQUIRE(ctx);

        const uint32_t width = 4;
        const uint32_t height = 4;
        auto srcTexture = device->createTexture2D(width, height, ResourceFormat::RGBA8Unorm, 1, 1, nullptr, TextureUsage::ShaderResource);
        auto dstTexture = device->createTexture2D(width, height, ResourceFormat::RGBA8Unorm, 1, 1, nullptr, TextureUsage::RenderTarget | TextureUsage::ShaderResource);
        REQUIRE(srcTexture);
        REQUIRE(dstTexture);

        std::vector<uint32_t> srcData(width * height);
        for (uint32_t y = 0; y < height; ++y)
        {
            for (uint32_t x = 0; x < width; ++x)
            {
                uint8_t r = static_cast<uint8_t>(x * 40);
                uint8_t g = static_cast<uint8_t>(y * 60);
                uint8_t b = 15;
                srcData[y * width + x] = packRgba(r, g, b, 255);
            }
        }

        ctx->updateTextureData(srcTexture.get(), srcData.data());
        ctx->submit(true);
        ctx->resourceBarrier(srcTexture.get(), Resource::State::ShaderResource);

        blitToTexture(
            ctx,
            srcTexture->getSRV(),
            dstTexture->getRTV(),
            RenderPassEncoder::kMaxRect,
            RenderPassEncoder::kMaxRect,
            TextureFilteringMode::Point
        );

        auto readbackBytes = ctx->readTextureSubresource(dstTexture.get(), 0);
        ctx->submit(true);

        REQUIRE(readbackBytes.size() == width * height * 4);
        uint32_t const* readbackData = reinterpret_cast<uint32_t const*>(readbackBytes.data());
        for (size_t i = 0; i < srcData.size(); ++i)
        {
            CHECK(readbackData[i] == srcData[i]);
        }
    }
}

TEST_CASE("Point Blit Region Copy")
{
    for (auto deviceType : { Device::Type::D3D12, Device::Type::Vulkan })
    {
        Device::Desc desc;
        desc.type = deviceType;
        auto device = april::core::make_ref<Device>(desc);
        if (!device) continue;

        auto ctx = device->getCommandContext();
        REQUIRE(ctx);

        const uint32_t width = 4;
        const uint32_t height = 4;
        auto srcTexture = device->createTexture2D(width, height, ResourceFormat::RGBA8Unorm, 1, 1, nullptr, TextureUsage::ShaderResource);
        auto dstTexture = device->createTexture2D(width, height, ResourceFormat::RGBA8Unorm, 1, 1, nullptr, TextureUsage::RenderTarget | TextureUsage::ShaderResource);
        REQUIRE(srcTexture);
        REQUIRE(dstTexture);

        std::vector<uint32_t> srcData(width * height);
        for (uint32_t y = 0; y < height; ++y)
        {
            for (uint32_t x = 0; x < width; ++x)
            {
                uint8_t r = static_cast<uint8_t>(x * 30 + 10);
                uint8_t g = static_cast<uint8_t>(y * 50 + 20);
                uint8_t b = 200;
                srcData[y * width + x] = packRgba(r, g, b, 255);
            }
        }

        ctx->updateTextureData(srcTexture.get(), srcData.data());
        ctx->submit(true);
        ctx->resourceBarrier(srcTexture.get(), Resource::State::ShaderResource);

        uint4 srcRect = {1, 1, 3, 3};
        uint4 dstRect = {0, 0, 2, 2};

        uint32_t clearValue = packRgba(5, 10, 15, 255);
        float4 clearColor = float4(5.0f / 255.0f, 10.0f / 255.0f, 15.0f / 255.0f, 1.0f);

        blitToTexture(
            ctx,
            srcTexture->getSRV(),
            dstTexture->getRTV(),
            srcRect,
            dstRect,
            TextureFilteringMode::Point,
            clearColor
        );

        auto readbackBytes = ctx->readTextureSubresource(dstTexture.get(), 0);
        ctx->submit(true);

        REQUIRE(readbackBytes.size() == width * height * 4);
        uint32_t const* readbackData = reinterpret_cast<uint32_t const*>(readbackBytes.data());

        for (uint32_t y = 0; y < height; ++y)
        {
            for (uint32_t x = 0; x < width; ++x)
            {
                uint32_t expected = clearValue;
                if (x < 2 && y < 2)
                {
                    uint32_t srcX = x + srcRect.x;
                    uint32_t srcY = y + srcRect.y;
                    expected = srcData[srcY * width + srcX];
                }
                CHECK(readbackData[y * width + x] == expected);
            }
        }
    }
}

TEST_CASE("Linear Blit Downscale To Single Pixel")
{
    for (auto deviceType : { Device::Type::D3D12, Device::Type::Vulkan })
    {
        Device::Desc desc;
        desc.type = deviceType;
        auto device = april::core::make_ref<Device>(desc);
        if (!device) continue;

        auto ctx = device->getCommandContext();
        REQUIRE(ctx);

        const uint32_t srcWidth = 2;
        const uint32_t srcHeight = 2;
        auto srcTexture = device->createTexture2D(srcWidth, srcHeight, ResourceFormat::RGBA8Unorm, 1, 1, nullptr, TextureUsage::ShaderResource);
        auto dstTexture = device->createTexture2D(1, 1, ResourceFormat::RGBA8Unorm, 1, 1, nullptr, TextureUsage::RenderTarget | TextureUsage::ShaderResource);
        REQUIRE(srcTexture);
        REQUIRE(dstTexture);

        std::vector<uint32_t> srcData = {
            packRgba(255, 0, 0, 255),
            packRgba(0, 255, 0, 255),
            packRgba(0, 0, 255, 255),
            packRgba(255, 255, 255, 255)
        };

        ctx->updateTextureData(srcTexture.get(), srcData.data());
        ctx->submit(true);
        ctx->resourceBarrier(srcTexture.get(), Resource::State::ShaderResource);

        blitToTexture(
            ctx,
            srcTexture->getSRV(),
            dstTexture->getRTV(),
            RenderPassEncoder::kMaxRect,
            RenderPassEncoder::kMaxRect,
            TextureFilteringMode::Linear
        );

        auto readbackBytes = ctx->readTextureSubresource(dstTexture.get(), 0);
        ctx->submit(true);

        REQUIRE(readbackBytes.size() == 4);
        uint32_t pixel = *reinterpret_cast<uint32_t const*>(readbackBytes.data());

        uint8_t r, g, b, a;
        unpackRgba(pixel, r, g, b, a);

        uint8_t expectedR = static_cast<uint8_t>((255 + 0 + 0 + 255) / 4);
        uint8_t expectedG = static_cast<uint8_t>((0 + 255 + 0 + 255) / 4);
        uint8_t expectedB = static_cast<uint8_t>((0 + 0 + 255 + 255) / 4);
        uint8_t expectedA = 255;

        CHECK(std::abs(int(r) - int(expectedR)) <= 1);
        CHECK(std::abs(int(g) - int(expectedG)) <= 1);
        CHECK(std::abs(int(b) - int(expectedB)) <= 1);
        CHECK(std::abs(int(a) - int(expectedA)) <= 1);
    }
}

TEST_CASE("Linear Blit Various Sizes")
{
    struct SizePair
    {
        uint32_t srcW;
        uint32_t srcH;
        uint32_t dstW;
        uint32_t dstH;
    };

    const std::vector<SizePair> cases = {
        {7, 5, 3, 3},
        {7, 5, 14, 10},
        {13, 9, 5, 7},
    };

    for (auto deviceType : { Device::Type::D3D12, Device::Type::Vulkan })
    {
        Device::Desc desc;
        desc.type = deviceType;
        auto device = april::core::make_ref<Device>(desc);
        if (!device) continue;

        auto ctx = device->getCommandContext();
        REQUIRE(ctx);

        for (auto const& testCase : cases)
        {
            auto srcTexture = device->createTexture2D(
                testCase.srcW,
                testCase.srcH,
                ResourceFormat::RGBA8Unorm,
                1,
                1,
                nullptr,
                TextureUsage::ShaderResource
            );
            auto dstTexture = device->createTexture2D(
                testCase.dstW,
                testCase.dstH,
                ResourceFormat::RGBA8Unorm,
                1,
                1,
                nullptr,
                TextureUsage::RenderTarget | TextureUsage::ShaderResource
            );
            REQUIRE(srcTexture);
            REQUIRE(dstTexture);

            auto srcData = makeUvGradient(testCase.srcW, testCase.srcH);
            ctx->updateTextureData(srcTexture.get(), srcData.data());
            ctx->submit(true);
            ctx->resourceBarrier(srcTexture.get(), Resource::State::ShaderResource);

            blitToTexture(
                ctx,
                srcTexture->getSRV(),
                dstTexture->getRTV(),
                RenderPassEncoder::kMaxRect,
                RenderPassEncoder::kMaxRect,
                TextureFilteringMode::Linear
            );

            auto readbackBytes = ctx->readTextureSubresource(dstTexture.get(), 0);
            ctx->submit(true);

            REQUIRE(readbackBytes.size() == testCase.dstW * testCase.dstH * 4);
            uint32_t const* readbackData = reinterpret_cast<uint32_t const*>(readbackBytes.data());

            const int tolerance = 16;
            checkUvGradientPixel(readbackData, testCase.dstW, testCase.dstH, 0, 0, tolerance);
            checkUvGradientPixel(readbackData, testCase.dstW, testCase.dstH, testCase.dstW - 1, 0, tolerance);
            checkUvGradientPixel(readbackData, testCase.dstW, testCase.dstH, 0, testCase.dstH - 1, tolerance);
            checkUvGradientPixel(readbackData, testCase.dstW, testCase.dstH, testCase.dstW - 1, testCase.dstH - 1, tolerance);
            checkUvGradientPixel(readbackData, testCase.dstW, testCase.dstH, testCase.dstW / 2, testCase.dstH / 2, tolerance);

            uint8_t r00, g00, b00, a00;
            unpackRgba(readbackData[0], r00, g00, b00, a00);
            uint8_t r10, g10, b10, a10;
            unpackRgba(readbackData[testCase.dstW - 1], r10, g10, b10, a10);
            uint8_t r01, g01, b01, a01;
            unpackRgba(readbackData[(testCase.dstH - 1) * testCase.dstW], r01, g01, b01, a01);

            CHECK(r10 >= r00);
            CHECK(g01 >= g00);
        }
    }
}
