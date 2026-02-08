// Material data structures for GPU upload.
// These structs are the source of truth and generate matching C++ headers.

import material.material_types;

// @export-cpp
struct MaterialHeader
{
    uint materialType;      // MaterialType enum
    uint flags;             // MaterialFlags bitmask
    uint alphaMode;         // AlphaMode enum
    uint activeLobes;       // LobeType bitmask
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
};
