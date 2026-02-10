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

#ifndef MATERIAL_SYSTEM_ENABLE_EXTRA_RESOURCES
#define MATERIAL_SYSTEM_ENABLE_EXTRA_RESOURCES 0
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
#if MATERIAL_SYSTEM_ENABLE_EXTRA_RESOURCES != 0
    ByteAddressBuffer materialBuffers[kMaterialBufferTableSize];
    Texture3D materialTextures3d[kMaterialTexture3dTableSize];
#endif

#if MATERIAL_SYSTEM_ENABLE_EXTRA_RESOURCES != 0 && MATERIAL_UDIM_INDIRECTION_ENABLED != 0
    StructuredBuffer<int> udimIndirection;
#endif

    [ForceInline]
    StandardMaterialData getMaterialData(uint materialID)
    {
        return materials[materialID];
    }

    [ForceInline]
    SamplerState getMaterialSampler(StandardMaterialData mtl)
    {
        uint samplerHandle = min(mtl.samplerHandle, kMaterialSamplerTableSize - 1);
        return materialSamplers[samplerHandle];
    }

    [ForceInline]
    float4 sampleMaterialTexture<L: ITextureSampler>(uint textureHandle, SamplerState sampler, float2 uv, L lod)
    {
        uint clampedHandle = min(textureHandle, kMaterialTextureTableSize - 1);
        return lod.sampleTexture(materialTextures[clampedHandle], sampler, uv);
    }

    [ForceInline]
    float4 sampleMaterialTexture(uint textureHandle, SamplerState sampler, float2 uv)
    {
        ImplicitLodTextureSampler lod;
        return sampleMaterialTexture(textureHandle, sampler, uv, lod);
    }
};
