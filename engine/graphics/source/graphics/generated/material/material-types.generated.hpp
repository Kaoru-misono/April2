// AUTO-GENERATED from material-types.slang - DO NOT EDIT
#pragma once

#include <core/math/type.hpp>
#include <core/tools/enum-flags.hpp>

namespace april::graphics::inline generated
{
    using namespace april;

    enum class LobeType : uint32_t
    {
        None = 0x00,
        DiffuseReflection = 0x01,
        SpecularReflection = 0x02,
        DeltaReflection = 0x04,
        DiffuseTransmission = 0x10,
        SpecularTransmission = 0x20,
        DeltaTransmission = 0x40,
        Diffuse = 0x11,
        Specular = 0x22,
        Delta = 0x44,
        NonDelta = 0x33,
        Reflection = 0x0f,
        Transmission = 0xf0,
        NonDeltaReflection = 0x03,
        NonDeltaTransmission = 0x30,
        All = 0xff,
    };
    AP_ENUM_CLASS_OPERATORS(LobeType);

    enum class AlphaMode : uint32_t
    {
        Opaque = 0,
        Mask = 1,
        Blend = 2,
    };

    enum class MaterialType : uint32_t
    {
        Unknown = 0,
        Standard = 1,
    };

    enum class MaterialFlags : uint32_t
    {
        None = 0x00,
        DoubleSided = 0x01,
        AlphaTested = 0x02,
        Emissive = 0x04,
        ThinSurface = 0x08,
        HasTextures = 0x10,
        SpecularWorkflow = 0x20,
    };
    AP_ENUM_CLASS_OPERATORS(MaterialFlags);

    enum class TextureSlot : uint32_t
    {
        BaseColor = 0,
        MetallicRoughness = 1,
        Normal = 2,
        Occlusion = 3,
        Emissive = 4,
        Count = 5,
    };

} // namespace april::graphics::inline generated
