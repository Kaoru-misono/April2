#pragma once

#include "rhi/texture.hpp"

namespace april::graphics
{
    enum class MaterialTextureSemantic
    {
        Emissive,
        Normal,
        Transmission,
    };

    struct MaterialTextureAnalysis
    {
        bool isConstant{false};
        float4 constantValue{0.0f};
    };

    class MaterialTextureAnalyzer
    {
    public:
        auto analyze(core::ref<Texture> const& texture) const -> MaterialTextureAnalysis;
        auto canPrune(core::ref<Texture> const& texture, MaterialTextureSemantic semantic) const -> bool;
    };
}
