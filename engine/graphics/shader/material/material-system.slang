// MaterialSystem helpers shared by scene shaders.

import material.material_data;

static const uint kMaterialTextureTableSize = 128;
static const uint kMaterialSamplerTableSize = 8;

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
    uint samplerHandle = min(mtl.samplerHandle, kMaterialSamplerTableSize - 1);
    return ms.materialSamplers[samplerHandle];
}

float4 sampleMaterialTexture(MaterialSystemContext ms, uint textureHandle, SamplerState sampler, float2 uv)
{
    uint clampedHandle = min(textureHandle, kMaterialTextureTableSize - 1);
    return ms.materialTextures[clampedHandle].Sample(sampler, uv);
}
