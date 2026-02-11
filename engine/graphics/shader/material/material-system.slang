// Shader-side material system aligned with Falcor MaterialSystem.slang.

import material.material_data;
import material.basic_material_data;
import material.material_types;
import material.texture_handle;
import material.texture_sampler;

#if !defined(MATERIAL_SYSTEM_TEXTURE_DESC_COUNT) || !defined(MATERIAL_SYSTEM_SAMPLER_DESC_COUNT) || !defined(MATERIAL_SYSTEM_BUFFER_DESC_COUNT)
#error "MaterialSystem defines are not set!"
#endif

#if !defined(MATERIAL_SYSTEM_TEXTURE_3D_DESC_COUNT) || !defined(MATERIAL_SYSTEM_UDIM_INDIRECTION_ENABLED) || !defined(MATERIAL_SYSTEM_USE_LIGHT_PROFILE)
#error "MaterialSystem optional defines are not set!"
#endif

struct MaterialSystem
{
    uint materialCount;
    StructuredBuffer<MaterialDataBlob> materialData;
    SamplerState materialSamplers[MATERIAL_SYSTEM_SAMPLER_DESC_COUNT];
    Texture2D<float4> materialTextures[MATERIAL_SYSTEM_TEXTURE_DESC_COUNT];
    ByteAddressBuffer materialBuffers[MATERIAL_SYSTEM_BUFFER_DESC_COUNT];
    Texture3D<float4> materialTextures3D[MATERIAL_SYSTEM_TEXTURE_3D_DESC_COUNT];
    StructuredBuffer<int> udimIndirection;

    uint getMaterialCount()
    {
        return materialCount;
    }

    MaterialType getMaterialType(const uint materialID)
    {
        return materialData[materialID].header.getMaterialType();
    }

    MaterialHeader getMaterialHeader(const uint materialID)
    {
        return materialData[materialID].header;
    }

    MaterialDataBlob getMaterialDataBlob(const uint materialID)
    {
        return materialData[materialID];
    }

    bool isBasicMaterial(const uint materialID)
    {
        return materialData[materialID].header.isBasicMaterial();
    }

    BasicMaterialData getBasicMaterialData(const uint materialID)
    {
        return reinterpret<BasicMaterialData, MaterialPayload>(materialData[materialID].payload);
    }

    SamplerState getTextureSampler(const uint samplerID)
    {
        return materialSamplers[samplerID];
    }

    SamplerState getDefaultTextureSampler(const uint materialID)
    {
        uint samplerID = getMaterialHeader(materialID).getDefaultTextureSamplerID();
        return getTextureSampler(samplerID);
    }

    ByteAddressBuffer getBuffer(const uint bufferID)
    {
        return materialBuffers[bufferID];
    }

    Texture3D<float4> getTexture3D(const uint textureID)
    {
        return materialTextures3D[textureID];
    }

    bool isDisplaced(const uint materialID)
    {
        if (isBasicMaterial(materialID))
        {
            BasicMaterialData md = getBasicMaterialData(materialID);
            return getResolvedTextureHandle(md.texDisplacementMap).getMode() == TextureHandle::Mode::Texture;
        }
        return false;
    }

    bool isEmissiveTextured(const uint materialID)
    {
        if (isBasicMaterial(materialID))
        {
            BasicMaterialData md = getBasicMaterialData(materialID);
            return getResolvedTextureHandle(md.texEmissive).getMode() == TextureHandle::Mode::Texture;
        }
        return false;
    }

    float3 evalEmissive(const uint materialID, const float2 uv)
    {
        if (isBasicMaterial(materialID))
        {
            BasicMaterialData md = getBasicMaterialData(materialID);
            SamplerState s = getDefaultTextureSampler(materialID);
            ExplicitLodTextureSampler lod = ExplicitLodTextureSampler(0.0f);
            return sampleTexture(md.texEmissive, s, uv, float4(md.emissive, 1.0f), lod).rgb * md.emissiveFactor;
        }
        return float3(0.0f);
    }

    float evalIoR(const uint materialID)
    {
        return getMaterialHeader(materialID).getIoR();
    }

    TextureHandle getResolvedTextureHandle(TextureHandle handle, inout float2 uv)
    {
#if MATERIAL_SYSTEM_UDIM_INDIRECTION_ENABLED == 0
        return handle;
#else
        if (!handle.getUdimEnabled())
            return handle;

        uint udimBase = handle.getTextureID();
        uint udimID = udimBase + uint(uv.x) + 10 * uint(uv.y);
        uv = frac(uv);

        int texID = -1;
        uint numStructs = 0;
        uint stride = 0;
        udimIndirection.GetDimensions(numStructs, stride);
        if (udimID < numStructs)
            texID = udimIndirection[udimID];

        TextureHandle result = handle;
        if (texID == -1)
        {
            result.setMode(TextureHandle::Mode::Uniform);
            return result;
        }

        result.setTextureID(uint(texID));
        result.setUdimEnabled(false);
        return result;
#endif
    }

    TextureHandle getResolvedTextureHandle(const TextureHandle handle)
    {
        float2 uv00 = float2(0.0f, 0.0f);
        return getResolvedTextureHandle(handle, uv00);
    }

    TextureInfo getTextureInfo(TextureHandle handle)
    {
        handle = getResolvedTextureHandle(handle);
        TextureInfo info = {};
        switch (handle.getMode())
        {
        case TextureHandle::Mode::Texture:
            materialTextures[handle.getTextureID()].GetDimensions(0, info.width, info.height, info.mipLevels);
            info.depth = 1;
            break;
        default:
            break;
        }
        return info;
    }

    float4 sampleTexture(const TextureHandle handle, SamplerState sampler, const float2 uv, const float lod)
    {
        ExplicitLodTextureSampler explicitLOD = ExplicitLodTextureSampler(lod);
        return sampleTexture(handle, sampler, uv, float4(0.0f), explicitLOD);
    }

    float4 sampleTexture<L: ITextureSampler>(TextureHandle handle, SamplerState sampler, float2 uv, const float4 uniformValue, L lod)
    {
        handle = getResolvedTextureHandle(handle, uv);
        switch (handle.getMode())
        {
        case TextureHandle::Mode::Uniform:
            return uniformValue;
        case TextureHandle::Mode::Texture:
            return lod.sampleTexture(materialTextures[handle.getTextureID()], sampler, uv);
        default:
            return float4(0.0f);
        }
    }

    float4 sampleTexture(TextureHandle handle, SamplerState sampler, float2 uv, const float4 uniformValue)
    {
        ImplicitLodTextureSampler lod;
        return sampleTexture(handle, sampler, uv, uniformValue, lod);
    }
};
