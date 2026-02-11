#include "texture-analyzer.hpp"

#include "rhi/buffer.hpp"
#include "rhi/command-context.hpp"

#include <core/log/logger.hpp>

#include <array>
#include <cmath>

namespace april::graphics
{
    namespace
    {
        auto computeRangeBits(float const value) -> uint32_t
        {
            auto bits = uint32_t{0};
            if (std::isnan(value))
            {
                bits |= static_cast<uint32_t>(TextureAnalyzer::Result::RangeFlags::NaN);
            }
            if (std::isinf(value))
            {
                bits |= static_cast<uint32_t>(TextureAnalyzer::Result::RangeFlags::Inf);
            }
            if (value > 0.0f)
            {
                bits |= static_cast<uint32_t>(TextureAnalyzer::Result::RangeFlags::Pos);
            }
            if (value < 0.0f)
            {
                bits |= static_cast<uint32_t>(TextureAnalyzer::Result::RangeFlags::Neg);
            }
            return bits;
        }

        auto packChannelRanges(float4 const& minValue, float4 const& maxValue) -> uint32_t
        {
            auto mask = uint32_t{0};
            auto channelRanges = std::array<uint32_t, 4>{
                computeRangeBits(minValue.x) | computeRangeBits(maxValue.x),
                computeRangeBits(minValue.y) | computeRangeBits(maxValue.y),
                computeRangeBits(minValue.z) | computeRangeBits(maxValue.z),
                computeRangeBits(minValue.w) | computeRangeBits(maxValue.w),
            };

            for (auto i = 0; i < 4; ++i)
            {
                mask |= (channelRanges[i] & 0xf) << (4 + 4 * i);
            }
            return mask;
        }
    }

    auto TextureAnalyzer::analyze(core::ref<Texture> const& p_texture) const -> Result
    {
        auto result = Result{};
        result.minValue = float4(0.0f);
        result.maxValue = float4(1.0f);
        if (!p_texture)
        {
            result.mask = static_cast<uint32_t>(TextureChannelFlags::RGBA);
            return result;
        }

        if (p_texture->getWidth() != 1 || p_texture->getHeight() != 1 || p_texture->getDepth() != 1)
        {
            result.mask = static_cast<uint32_t>(TextureChannelFlags::RGBA);
            return result;
        }

        auto const format = p_texture->getFormat();
        if (format == ResourceFormat::RGBA8Unorm || format == ResourceFormat::RGBA8UnormSrgb)
        {
            auto pixel = std::array<uint8_t, 4>{};
            p_texture->getSubresourceBlob(0, pixel.data(), pixel.size());
            result.value = float4(
                static_cast<float>(pixel[0]) / 255.0f,
                static_cast<float>(pixel[1]) / 255.0f,
                static_cast<float>(pixel[2]) / 255.0f,
                static_cast<float>(pixel[3]) / 255.0f
            );
        }
        else if (format == ResourceFormat::BGRA8Unorm || format == ResourceFormat::BGRA8UnormSrgb)
        {
            auto pixel = std::array<uint8_t, 4>{};
            p_texture->getSubresourceBlob(0, pixel.data(), pixel.size());
            result.value = float4(
                static_cast<float>(pixel[2]) / 255.0f,
                static_cast<float>(pixel[1]) / 255.0f,
                static_cast<float>(pixel[0]) / 255.0f,
                static_cast<float>(pixel[3]) / 255.0f
            );
        }
        else if (format == ResourceFormat::RGBA32Float)
        {
            auto pixel = std::array<float, 4>{};
            p_texture->getSubresourceBlob(0, pixel.data(), sizeof(pixel));
            result.value = float4(pixel[0], pixel[1], pixel[2], pixel[3]);
        }
        else
        {
            result.mask = static_cast<uint32_t>(TextureChannelFlags::RGBA);
            return result;
        }

        result.minValue = result.value;
        result.maxValue = result.value;
        result.mask = packChannelRanges(result.minValue, result.maxValue);
        return result;
    }

    auto TextureAnalyzer::analyze(std::vector<core::ref<Texture>> const& textures) const -> std::vector<Result>
    {
        auto results = std::vector<Result>{};
        results.reserve(textures.size());
        for (auto const& p_texture : textures)
        {
            results.push_back(analyze(p_texture));
        }
        return results;
    }

    auto TextureAnalyzer::analyze(CommandContext* p_context, std::vector<core::ref<Texture>> const& textures, core::ref<Buffer> const& p_results) const -> void
    {
        (void)p_context;
        AP_ASSERT(p_results);

        auto const results = analyze(textures);
        auto const requiredSize = results.size() * sizeof(Result);
        AP_ASSERT(p_results->getSize() >= requiredSize);
        if (requiredSize > 0)
        {
            p_results->setBlob(results.data(), 0, requiredSize);
        }
    }
}
