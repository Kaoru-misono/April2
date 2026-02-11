#include "material-texture-analyzer.hpp"

#include <array>
#include <cmath>

namespace april::graphics
{
    auto MaterialTextureAnalyzer::analyze(core::ref<Texture> const& texture) const -> MaterialTextureAnalysis
    {
        auto result = MaterialTextureAnalysis{};
        if (!texture)
        {
            return result;
        }

        if (texture->getWidth() != 1 || texture->getHeight() != 1 || texture->getDepth() != 1)
        {
            return result;
        }

        auto const format = texture->getFormat();
        if (format == ResourceFormat::RGBA8Unorm || format == ResourceFormat::RGBA8UnormSrgb)
        {
            std::array<uint8_t, 4> pixel{};
            texture->getSubresourceBlob(0, pixel.data(), pixel.size());
            result.isConstant = true;
            result.constantValue = float4(
                static_cast<float>(pixel[0]) / 255.0f,
                static_cast<float>(pixel[1]) / 255.0f,
                static_cast<float>(pixel[2]) / 255.0f,
                static_cast<float>(pixel[3]) / 255.0f
            );
            return result;
        }

        if (format == ResourceFormat::BGRA8Unorm || format == ResourceFormat::BGRA8UnormSrgb)
        {
            std::array<uint8_t, 4> pixel{};
            texture->getSubresourceBlob(0, pixel.data(), pixel.size());
            result.isConstant = true;
            result.constantValue = float4(
                static_cast<float>(pixel[2]) / 255.0f,
                static_cast<float>(pixel[1]) / 255.0f,
                static_cast<float>(pixel[0]) / 255.0f,
                static_cast<float>(pixel[3]) / 255.0f
            );
            return result;
        }

        if (format == ResourceFormat::RGBA32Float)
        {
            std::array<float, 4> pixel{};
            texture->getSubresourceBlob(0, pixel.data(), sizeof(pixel));
            result.isConstant = true;
            result.constantValue = float4(pixel[0], pixel[1], pixel[2], pixel[3]);
            return result;
        }

        return result;
    }

    auto MaterialTextureAnalyzer::canPrune(core::ref<Texture> const& texture, MaterialTextureSemantic semantic) const -> bool
    {
        auto const analysis = analyze(texture);
        if (!analysis.isConstant)
        {
            return false;
        }

        auto const epsilon = 1e-3f;
        switch (semantic)
        {
        case MaterialTextureSemantic::Emissive:
        {
            auto const emissive = float3(analysis.constantValue.x, analysis.constantValue.y, analysis.constantValue.z);
            return length(emissive) <= epsilon;
        }
        case MaterialTextureSemantic::Normal:
            return std::abs(analysis.constantValue.x - 0.5f) < epsilon &&
                   std::abs(analysis.constantValue.y - 0.5f) < epsilon &&
                   std::abs(analysis.constantValue.z - 1.0f) < epsilon;
        case MaterialTextureSemantic::Transmission:
        {
            auto const transmission = float3(analysis.constantValue.x, analysis.constantValue.y, analysis.constantValue.z);
            return length(transmission) <= epsilon;
        }
        default:
            return false;
        }
    }
}
