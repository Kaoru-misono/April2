// Shared material header/blob layout for codegen.

import material_types;
import texture_handle;

uint PACK_BITS(uint numBits, uint offset, uint value, uint field)
{
    uint mask = ((1u << numBits) - 1u) << offset;
    return (value & ~mask) | ((field << offset) & mask);
}

uint EXTRACT_BITS(uint numBits, uint offset, uint value)
{
    return (value >> offset) & ((1u << numBits) - 1u);
}

// @export-cpp
struct MaterialHeader
{
    uint4 packedData = uint4(0, 0, 0, 0);

    static const uint kMaterialTypeBits = 16;
    static const uint kNestedPriorityBits = 4;
    static const uint kLobeTypeBits = 8;
    static const uint kSamplerIDBits = 8;
    static const uint kAlphaModeBits = 1;
    static const uint kAlphaThresholdBits = 16;
    static const uint kIoRBits = 16;

    static const uint kMaterialTypeOffset = 0;
    static const uint kNestedPriorityOffset = kMaterialTypeOffset + kMaterialTypeBits;
    static const uint kLobeTypeOffset = kNestedPriorityOffset + kNestedPriorityBits;
    static const uint kDoubleSidedFlagOffset = kLobeTypeOffset + kLobeTypeBits;
    static const uint kThinSurfaceFlagOffset = kDoubleSidedFlagOffset + 1;
    static const uint kEmissiveFlagOffset = kThinSurfaceFlagOffset + 1;
    static const uint kIsBasicMaterialFlagOffset = kEmissiveFlagOffset + 1;

    static const uint kAlphaThresholdOffset = 0;
    static const uint kAlphaModeOffset = kAlphaThresholdOffset + kAlphaThresholdBits;
    static const uint kSamplerIDOffset = kAlphaModeOffset + kAlphaModeBits;
    static const uint kEnableLightProfileOffset = kSamplerIDOffset + kSamplerIDBits;
    static const uint kDeltaSpecularFlagOffset = kEnableLightProfileOffset + 1;

    static const uint kIoROffset = 0;

    [mutating]
    void setMaterialType(MaterialType type) { packedData.x = PACK_BITS(kMaterialTypeBits, kMaterialTypeOffset, packedData.x, uint(type)); }
    MaterialType getMaterialType() { return MaterialType(EXTRACT_BITS(kMaterialTypeBits, kMaterialTypeOffset, packedData.x)); }

    [mutating]
    void setAlphaMode(AlphaMode mode) { packedData.y = PACK_BITS(kAlphaModeBits, kAlphaModeOffset, packedData.y, uint(mode)); }
    AlphaMode getAlphaMode() { return AlphaMode(EXTRACT_BITS(kAlphaModeBits, kAlphaModeOffset, packedData.y)); }

    [mutating]
    void setAlphaThreshold(float alphaThreshold)
    {
        uint encoded = uint(asuint16(float16_t(alphaThreshold)));
        packedData.y = PACK_BITS(kAlphaThresholdBits, kAlphaThresholdOffset, packedData.y, encoded);
    }
    float getAlphaThreshold()
    {
        uint encoded = EXTRACT_BITS(kAlphaThresholdBits, kAlphaThresholdOffset, packedData.y);
        return float(asfloat16(uint16_t(encoded)));
    }

    [mutating]
    void setNestedPriority(uint priority) { packedData.x = PACK_BITS(kNestedPriorityBits, kNestedPriorityOffset, packedData.x, priority); }
    uint getNestedPriority() { return EXTRACT_BITS(kNestedPriorityBits, kNestedPriorityOffset, packedData.x); }

    [mutating]
    void setActiveLobes(uint lobes) { packedData.x = PACK_BITS(kLobeTypeBits, kLobeTypeOffset, packedData.x, lobes); }
    uint getActiveLobes() { return EXTRACT_BITS(kLobeTypeBits, kLobeTypeOffset, packedData.x); }

    [mutating]
    void setDefaultTextureSamplerID(uint samplerID) { packedData.y = PACK_BITS(kSamplerIDBits, kSamplerIDOffset, packedData.y, samplerID); }
    uint getDefaultTextureSamplerID() { return EXTRACT_BITS(kSamplerIDBits, kSamplerIDOffset, packedData.y); }

    [mutating]
    void setDoubleSided(bool v) { packedData.x = PACK_BITS(1, kDoubleSidedFlagOffset, packedData.x, v ? 1u : 0u); }
    bool isDoubleSided() { return (packedData.x & (1u << kDoubleSidedFlagOffset)) != 0; }

    [mutating]
    void setThinSurface(bool v) { packedData.x = PACK_BITS(1, kThinSurfaceFlagOffset, packedData.x, v ? 1u : 0u); }
    bool isThinSurface() { return (packedData.x & (1u << kThinSurfaceFlagOffset)) != 0; }

    [mutating]
    void setEmissive(bool v) { packedData.x = PACK_BITS(1, kEmissiveFlagOffset, packedData.x, v ? 1u : 0u); }
    bool isEmissive() { return (packedData.x & (1u << kEmissiveFlagOffset)) != 0; }

    [mutating]
    void setIsBasicMaterial(bool v) { packedData.x = PACK_BITS(1, kIsBasicMaterialFlagOffset, packedData.x, v ? 1u : 0u); }
    bool isBasicMaterial() { return (packedData.x & (1u << kIsBasicMaterialFlagOffset)) != 0; }

    [mutating]
    void setEnableLightProfile(bool v) { packedData.y = PACK_BITS(1, kEnableLightProfileOffset, packedData.y, v ? 1u : 0u); }
    bool isLightProfileEnabled() { return (packedData.y & (1u << kEnableLightProfileOffset)) != 0; }

    [mutating]
    void setDeltaSpecular(bool v) { packedData.y = PACK_BITS(1, kDeltaSpecularFlagOffset, packedData.y, v ? 1u : 0u); }
    bool isDeltaSpecular() { return (packedData.y & (1u << kDeltaSpecularFlagOffset)) != 0; }

    [mutating]
    void setIoR(float ior)
    {
        uint encoded = uint(asuint16(float16_t(ior)));
        packedData.z = PACK_BITS(kIoRBits, kIoROffset, packedData.z, encoded);
    }
    float getIoR()
    {
        uint encoded = EXTRACT_BITS(kIoRBits, kIoROffset, packedData.z);
        return float(asfloat16(uint16_t(encoded)));
    }

    [mutating]
    void setAlphaTextureHandle(TextureHandle handle) { packedData.w = handle.packedData; }
    TextureHandle getAlphaTextureHandle() { TextureHandle handle; handle.packedData = packedData.w; return handle; }
};

// @export-cpp
struct MaterialPayload
{
    uint data[28];
};

// @export-cpp
struct MaterialDataBlob
{
    MaterialHeader header;
    MaterialPayload payload;
};
