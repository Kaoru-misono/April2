// MaterialSystem helpers shared by scene shaders.

import material.material_data;

// Descriptor table capacities - host-provided via defines, with defaults.
#ifndef MATERIAL_TEXTURE_TABLE_SIZE
#define MATERIAL_TEXTURE_TABLE_SIZE 128
#endif

#ifndef MATERIAL_SAMPLER_TABLE_SIZE
#define MATERIAL_SAMPLER_TABLE_SIZE 8
#endif

static const uint kMaterialTextureTableSize = MATERIAL_TEXTURE_TABLE_SIZE;
static const uint kMaterialSamplerTableSize = MATERIAL_SAMPLER_TABLE_SIZE;

struct MaterialSystemContext
{
    StructuredBuffer<StandardMaterialData> materials;
    Texture2D materialTextures[kMaterialTextureTableSize];
    SamplerState materialSamplers[kMaterialSamplerTableSize];
};

StandardMaterialData getMaterial(MaterialSystemContext ms, uint materialID)
{
    return ms.materials[materialID];
}

SamplerState getMaterialSampler(MaterialSystemContext ms, StandardMaterialData mtl)
{
    // Clamp to valid range with overflow fallback to slot 0.
    uint samplerHandle = min(mtl.samplerHandle, kMaterialSamplerTableSize - 1);
    return ms.materialSamplers[samplerHandle];
}

float4 sampleMaterialTexture(MaterialSystemContext ms, uint textureHandle, SamplerState sampler, float2 uv)
{
    // Clamp to valid range with overflow fallback to slot 0.
    uint clampedHandle = min(textureHandle, kMaterialTextureTableSize - 1);
    return ms.materialTextures[clampedHandle].Sample(sampler, uv);
}
