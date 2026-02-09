// Material data structures for GPU upload.
// These structs are the source of truth and generate matching C++ headers.

import material.material_types;

// Material ABI version. Increment when binary layout changes.
static const uint kMaterialAbiVersion = 1;

// @export-cpp
struct MaterialHeader
{
    uint abiVersion;        // Material ABI version (kMaterialAbiVersion)
    uint materialType;      // MaterialType enum
    uint flags;             // MaterialFlags bitmask
    uint alphaMode;         // AlphaMode enum
    uint activeLobes;       // LobeType bitmask
    uint reserved0;         // Reserved for forward-compatible header growth
    uint reserved1;         // Reserved for forward-compatible header growth
    uint reserved2;         // Reserved for forward-compatible header growth
};

// @export-cpp
struct StandardMaterialData
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

    uint baseColorTextureHandle;
    uint metallicRoughnessTextureHandle;
    uint normalTextureHandle;
    uint occlusionTextureHandle;
    uint emissiveTextureHandle;
    uint samplerHandle;
    uint bufferHandle;
    uint reservedDescriptorHandle;
};
