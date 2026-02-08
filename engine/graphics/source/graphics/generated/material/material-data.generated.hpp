// AUTO-GENERATED from material-data.slang - DO NOT EDIT
#pragma once

#include <core/math/type.hpp>
#include <core/tools/enum-flags.hpp>

namespace april::graphics::inline generated
{
    using namespace april;

    struct alignas(16) MaterialHeader
    {
        uint32_t materialType;
        uint32_t flags;
        uint32_t alphaMode;
        uint32_t activeLobes;
    };
    static_assert(sizeof(MaterialHeader) == 16, "Size mismatch for MaterialHeader");

    struct alignas(16) StandardMaterialData
    {
        MaterialHeader header;
        float4 baseColor;
        float metallic;
        float roughness;
        float ior;
        float specularTransmission;
        float3 emissive;
        float diffuseTransmission;
        float3 transmission;
        float alphaCutoff;
        float normalScale;
        float occlusionStrength;
        float2 _padding;
    };
    static_assert(sizeof(StandardMaterialData) == 96, "Size mismatch for StandardMaterialData");

} // namespace april::graphics::inline generated
