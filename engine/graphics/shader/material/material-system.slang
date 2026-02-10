// MaterialSystem helpers shared by scene shaders.

import material.material_data;
import material.material_types;
import material.shading_data;
import material.texture_sampler;

// Descriptor table capacities - host-provided via defines, with defaults.
#ifndef MATERIAL_TEXTURE_TABLE_SIZE
#define MATERIAL_TEXTURE_TABLE_SIZE 128
#endif

#ifndef MATERIAL_SAMPLER_TABLE_SIZE
#define MATERIAL_SAMPLER_TABLE_SIZE 8
#endif

#ifndef MATERIAL_BUFFER_TABLE_SIZE
#define MATERIAL_BUFFER_TABLE_SIZE 16
#endif

#ifndef MATERIAL_TEXTURE3D_TABLE_SIZE
#define MATERIAL_TEXTURE3D_TABLE_SIZE 16
#endif

#ifndef MATERIAL_UDIM_INDIRECTION_ENABLED
#define MATERIAL_UDIM_INDIRECTION_ENABLED 0
#endif

static const uint kMaterialTextureTableSize = MATERIAL_TEXTURE_TABLE_SIZE;
static const uint kMaterialSamplerTableSize = MATERIAL_SAMPLER_TABLE_SIZE;
static const uint kMaterialBufferTableSize = MATERIAL_BUFFER_TABLE_SIZE;
static const uint kMaterialTexture3dTableSize = MATERIAL_TEXTURE3D_TABLE_SIZE;

struct MaterialSystemContext
{
    StructuredBuffer<StandardMaterialData> materials;
    Texture2D materialTextures[kMaterialTextureTableSize];
    SamplerState materialSamplers[kMaterialSamplerTableSize];
    ByteAddressBuffer materialBuffers[kMaterialBufferTableSize];
    Texture3D materialTextures3d[kMaterialTexture3dTableSize];

#if MATERIAL_UDIM_INDIRECTION_ENABLED != 0
    StructuredBuffer<int> udimIndirection;
#endif
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

float4 sampleMaterialTexture<L: ITextureSampler>(MaterialSystemContext ms, uint textureHandle, SamplerState sampler, float2 uv, L lod)
{
    // Clamp to valid range with overflow fallback to slot 0.
    uint clampedHandle = min(textureHandle, kMaterialTextureTableSize - 1);
    return lod.sampleTexture(ms.materialTextures[clampedHandle], sampler, uv);
}

float4 sampleMaterialTexture(MaterialSystemContext ms, uint textureHandle, SamplerState sampler, float2 uv)
{
    ImplicitLodTextureSampler lod;
    return sampleMaterialTexture(ms, textureHandle, sampler, uv, lod);
}
