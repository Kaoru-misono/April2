// Shared basic material payload for codegen.

import material_types;
import material_data;
import texture_handle;

// @export-cpp
struct BasicMaterialData
{
    uint flags = 0;
    float emissiveFactor = 1.0f;

    float16_t4 baseColor = float16_t4(1.0h);
    float16_t4 specular = float16_t4(0.0h);

    float3 emissive = float3(0.0f);
    float16_t specularTransmission = 0.0h;
    float16_t3 transmission = float16_t3(1.0h);
    float16_t diffuseTransmission = 0.0h;

    float16_t3 volumeScattering = float16_t3(0.0h);
    float16_t _pad0 = 0.0h;
    float16_t3 volumeAbsorption = float16_t3(0.0h);
    float16_t volumeAnisotropy = 0.0h;

    float displacementScale = 0.0f;
    float displacementOffset = 0.0f;

    TextureHandle texBaseColor;
    TextureHandle texSpecular;
    TextureHandle texEmissive;
    TextureHandle texNormalMap;
    TextureHandle texTransmission;
    TextureHandle texDisplacementMap;

    static const uint kShadingModelBits = 1;
    static const uint kNormalMapTypeBits = 2;
    static const uint kEntryPointVolumePropertiesBits = 1;

    static const uint kShadingModelOffset = 0;
    static const uint kNormalMapTypeOffset = kShadingModelOffset + kShadingModelBits;
    static const uint kEntryPointVolumePropertiesOffset = kNormalMapTypeOffset + kNormalMapTypeBits;
    static const uint kMinSamplerIDOffset = kEntryPointVolumePropertiesOffset + kEntryPointVolumePropertiesBits;
    static const uint kMaxSamplerIDOffset = kMinSamplerIDOffset + MaterialHeader::kSamplerIDBits;

    [mutating]
    void setShadingModel(ShadingModel model) { flags = PACK_BITS(kShadingModelBits, kShadingModelOffset, flags, uint(model)); }
    ShadingModel getShadingModel() { return ShadingModel(EXTRACT_BITS(kShadingModelBits, kShadingModelOffset, flags)); }

    [mutating]
    void setNormalMapType(NormalMapType type) { flags = PACK_BITS(kNormalMapTypeBits, kNormalMapTypeOffset, flags, uint(type)); }
    NormalMapType getNormalMapType() { return NormalMapType(EXTRACT_BITS(kNormalMapTypeBits, kNormalMapTypeOffset, flags)); }

    [mutating]
    void setHasEntryPointVolumeProperties(bool poe) { flags = PACK_BITS(kEntryPointVolumePropertiesBits, kEntryPointVolumePropertiesOffset, flags, poe ? 1u : 0u); }
    bool getHasEntryPointVolumeProperties() { return EXTRACT_BITS(kEntryPointVolumePropertiesBits, kEntryPointVolumePropertiesOffset, flags) != 0; }

    [mutating]
    void setDisplacementMinSamplerID(uint samplerID) { flags = PACK_BITS(MaterialHeader::kSamplerIDBits, kMinSamplerIDOffset, flags, samplerID); }
    [mutating]
    void setDisplacementMaxSamplerID(uint samplerID) { flags = PACK_BITS(MaterialHeader::kSamplerIDBits, kMaxSamplerIDOffset, flags, samplerID); }
    uint getDisplacementMinSamplerID() { return EXTRACT_BITS(MaterialHeader::kSamplerIDBits, kMinSamplerIDOffset, flags); }
    uint getDisplacementMaxSamplerID() { return EXTRACT_BITS(MaterialHeader::kSamplerIDBits, kMaxSamplerIDOffset, flags); }
};
